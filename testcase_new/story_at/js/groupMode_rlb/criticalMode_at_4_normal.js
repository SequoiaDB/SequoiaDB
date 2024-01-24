/***************************************************************************************************
 * @Description: startCriticalMode 参数校验
 * @ATCaseID: criticalMode_at_4
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 4/042023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含7个节点的复制组
 * 测试场景：
 *    测试复制组使用 Enforced 参数开启 criticalMode
 * 测试步骤：
 *    1. 调用startCriticalMode()，使用 Enforced 参数开启，查看结果是否正确
 * 期望结果：
 *    步骤1：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_grpMode";
  commCheckBusinessStatus(db);

  // Get rg and nodes
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
  var slaveNodelList = getSlaveList(nodeList, replPrimaryName);

  // Start critical mode in node0
  var nodeName0 = slaveNodelList[0].HostName + ":" + slaveNodelList[0].svcname;
  rg.startCriticalMode({ NodeName: nodeName0, MinKeepTime: 5, MaxKeepTime: 10, Enforced: true });
  checkCriticalNodeMode(db, groupName, nodeName0);

  // Start critical mode in node1
  var nodeName1 = slaveNodelList[1].HostName + ":" + slaveNodelList[1].svcname;
  rg.startCriticalMode({ NodeName: nodeName1, MinKeepTime: 5, MaxKeepTime: 10, Enforced: true });
  checkCriticalNodeMode(db, groupName, nodeName1);

  // Start critical mode in node1 again
  var nodeName1 = slaveNodelList[1].HostName + ":" + slaveNodelList[1].svcname;
  rg.startCriticalMode({ NodeName: nodeName1, MinKeepTime: 5, MaxKeepTime: 10, Enforced: true });
  checkCriticalNodeMode(db, groupName, nodeName1);

  // Start critical mode in location
  var location = "criticalMode_at_4";
  setLocationForNodes(rg, slaveNodelList.slice(2, 4), location);
  rg.startCriticalMode({ Location: location, MinKeepTime: 5, MaxKeepTime: 10, Enforced: true });
  checkCriticalLocationMode(db, groupName, location);

  // Insert a record and check lsn version
  var csName = "cs_criticalMode_at_4";
  var clName = "cl_criticalMode_at_4";
  commDropCS(db, csName);
  var cs = commCreateCS(db, csName);
  var cl = cs.createCL(clName, { Group: groupName });
  cl.insert({ a: "test" });

  var lsnVersion = getLSNVersion(db, groupName);
  assert.equal(lsnVersion > 10, true, "LSN version is " + lsnVersion);

  // Stop Critical mode
  rg.stopCriticalMode();
  checkNormalGrpMode(db, groupName);

  // Reset group info
  commDropCS(db, csName);
  setLocationForNodes(rg, nodeList, "");
  rg.stopCriticalMode();
}
