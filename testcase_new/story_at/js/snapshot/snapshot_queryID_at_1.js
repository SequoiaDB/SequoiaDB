/***************************************************************************************************
 * @Description: 验证消息 QueryID 的正确性
 * @ATCaseID: queryID_at_1
 * @Author: FangJiabin
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                   并在 Testlink 系统中标记本用例文件名）
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 11/19/2022 FangJiabin  Test the correctness of queryID in msg's GlobalID
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群环境即可
 * 测试场景：
 *    验证会话快照和查询快照 QueryID 字段的正确性（测试不考虑直连数据节点和编目节点的情况）
 * 测试步骤：
 *    1. 测试准备：
 *       (1) 插入测试数据；
 *       (2) 修改慢查询配置参数 mongroupmask 和 monslowquerythreshold，修改之前先保存旧参数用于还原环境。
 *           修改是为了能够记录接下来执行的查询操作；
 *    2. 获取最新的 QueryID：
 *       (1) 获取 QueryID.identifyID，它是由客户端所连接节点的 tid 和 nodeID。同一个连接下，该值是不变的；
 *       (2) 从查询快照中获取最新的 QueryID（假设最新的 QueryID = 1）；
 *    3. 执行 find 操作，执行查询快照，按 {QueryID:2} 条件过滤记录；
 *    4. 执行 find 操作，利用游标获取下一条记录，执行会话快照，按 {"$and":[{"Contexts.QueryID":4},{"Contexts.Type":"DATA"}]} 过滤记录；
 *    5. 获取协调节点的 hostname 和 svcname，重连节点，比较前后两个连接的 QueryID.identifyID；
 *    7. 清理环境：
 *       (1) 复原配置参数；
 * 期望结果：
 *    每个步骤都能成功执行，关键步骤执行结果说明如下：
 *    1. 第 3 步和第 4 步，预期结果是有记录返回，如果记录为空则抛异常；
 *    2. 第 5 步，预期结果是前后两个 QueryID.identifyID 不相同，如果相同则抛异常；
 **************************************************************************************************/

testConf.clName = COMMCLNAME + "_queryID";

var queryIDPrefixStr;
var sequence;
var oldConf;
var i = 0;

main(test);

function test(testPara) {
  var cl = testPara.testCL;
  var ret;
  var queryID;

  setUp(cl);

  getQueryID();

  cl.find();
  queryID = getNewQueryID();
  ret = db.snapshot(SDB_SNAP_QUERIES, { QueryID: queryID });
  if (!ret.next()) {
    tearDown();
    throw new Error("Invalid QueryID in query snapshot. It must be " + queryID);
  }

  var cursor = cl.find();
  cursor.next();

  queryID = getNewQueryID();
  ret = db.snapshot(SDB_SNAP_CONTEXTS, { "$and": [ { "Contexts.QueryID": queryID }, { "Contexts.Type": "DATA" } ] });
  if (!ret.next()) {
    tearDown();
    throw new Error("Invalid QueryID in contexts snapshot. It must be " + queryID);
  }

  var oldQueryIDPrefixStr = queryIDPrefixStr;
  var ret = commGetGroups(db, true, "SYSCoord", true, false, true)[0][1];
  var hostname = ret.HostName;
  var svcname = ret.svcname;
  db = new Sdb(hostname, svcname);

  getQueryID();

  if (oldQueryIDPrefixStr == queryIDPrefixStr) {
    tearDown();
    // The probability that oldQueryIDPrefixStr and queryIDPrefixStr are equal is very small
    throw new Error("The queryID in different connection must be different");
  }

  tearDown();
}

function getNewQueryID() {
  sequence++;
  var newQueryID = queryIDPrefixStr + numToHexStr(sequence, 8);
  println("new QueryID: " + newQueryID);
  return newQueryID;
}

function setUp(cl) {
  // record eg: { _id: 637836b3d9a5ffa6b824a653, a: 0, no: 0, b: 0, c: 0 }
  insertRecs(cl, 2000);

  // before updating config, we should save old config firstly
  var ret = db.snapshot(SDB_SNAP_CONFIGS, {}, { mongroupmask: 1, monslowquerythreshold: 1 });
  oldConf = ret.current().toObj();
  db.updateConf({ mongroupmask: "all:detail", monslowquerythreshold: 10 });
}

function tearDown() {
  // restore conf
  db.updateConf(oldConf);
}

function getQueryID() {
  var curQueryID = "";
  var identifyIDHexStr = "";
  var sequenceStr = "";
  var pattern = "";
  var source = "client_and_coord_session" + i++;

  db.setSessionAttr({ Source: source });
  var ret = db.list(
    SDB_LIST_SESSIONS,
    { $and: [{ Source: source }, { Type: "Agent" }] },
    { TID: 1 }
  );
  var tid = ret.current().toObj()["TID"];
  var nodeID = commGetGroups(db, true, "SYSCoord", true, false, true)[0][1].NodeID;

  identifyIDHexStr = numToHexStr(tid, 8) + numToHexStr(nodeID, 4);
  pattern = "0x" + identifyIDHexStr + ".*";

  var ret = db.snapshot(
    SDB_SNAP_QUERIES,
    new SdbSnapshotOption().cond({ QueryID: { $regex: pattern } }).sort({ QueryID: -1 })
  );
  curQueryID = ret.current().toObj()["QueryID"];

  if ("" == curQueryID) {
    tearDown();
    throw new Error("Query snapshot must have QueryID field");
  }

  println("current QueryID: " + curQueryID);

  sequenceStr = curQueryID.substr(14, 12);
  queryIDPrefixStr = "0x" + identifyIDHexStr + sequenceStr.substr(0, 4);
  sequence = parseInt(sequenceStr.substr(4, 8), 16);
}

function numToHexStr(num, totalStrlen) {
  var numHexStr = num.toString(16);
  if (totalStrlen < numHexStr.length) {
    tearDown();
    throw new Error("Invalid total str len");
  } else if (totalStrlen == numHexStr.length) {
    return numHexStr;
  } else {
    var ret = "";
    for (var i = 0; i < totalStrlen - numHexStr.length; i++) {
      ret += "0";
    }
    ret += numHexStr;
    return ret;
  }
}
