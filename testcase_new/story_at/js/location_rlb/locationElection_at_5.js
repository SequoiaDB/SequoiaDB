/***************************************************************************************************
 * @Description: cls 多节点单位置集 LSN 心跳切主
 * @ATCaseID: locationElection_at_5
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/25/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含三个节点的复制组
 * 测试场景：
 *    测试多节点单位置集通过 LSN 切主
 * 测试步骤：
 *    1. 给同一复制组的其中两个备节点设置相同的位置 "A"，查看位置集内是否有主节点产生
 *    2. 给复制组的主节点设置位置 "A"，并检查位置集的主节点与复制组的主节点不同
 *    3. 向复制组中插入 50000 条数据，完成后检查位置集的主节点是否切到复制组的主节点上
 * 期望结果：
 *    步骤1：位置集有主节点
 *    步骤2：位置集的主节点不变，且不是复制组的主节点
 *    步骤3：位置集的主节点切换到复制组的主节点上
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "locationElection_at_5";
  var csName = "cs_locationElection_at_5";
  var clName = "cl_locationElection_at_5";

  // Step 0: get rg, nodeList and replPrimary
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();

  // Step 1: set location for two slave nodes and check location primary in location
  var locNodelList = getSlaveList(nodeList, replPrimaryName);
  setLocationForNodes(rg, locNodelList.slice(0, 2), location);
  var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 2: set location for replica group's primary node and check if the location primary has changed
  replPrimary.setLocation(location);
  checkNodeIsLocationPrimary(db, groupName, primary1, location, 34);

  // Step 3: insert 1000 records into group
  commDropCS(db, csName);
  var cs = commCreateCS(db, csName);
  var cl = cs.createCL(clName, { Group: groupName });
  var data = [];
  for (var i = 0; i < 50000; i++) {
    data.push({ a: i });
  }
  cl.insert(data);
  checkNodeIsLocationPrimary(db, groupName, replPrimaryName, location, 34);

  // Reset group info
  commDropCS(db, csName);
  setLocationForNodes(rg, locNodelList, "");
}
