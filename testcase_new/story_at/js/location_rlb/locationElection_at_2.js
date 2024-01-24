/***************************************************************************************************
 * @Description: cls 多节点单位置集 升主/降备
 * @ATCaseID: locationReelection_at_2
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/22/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含三个节点的复制组
 * 测试场景：
 *    测试多节点单位置集的升主/降备
 * 测试步骤：
 *    1. 给同一复制组的三个节点设置相同的位置 "A"，查看位置集内是否有主节点产生
 *    2. 停止一个备节点，查看位置集内是否有主节点
 *    3. 停止另一个备节点，查看位置集内是否有主节点
 *    4. 重启复制组，此时三个节点均启动，查看位置集内是否有主节点
 * 期望结果：
 *    步骤1-2：位置集有主节点
 *    步骤3：位置集没有主节点，当前的主节点降备
 *    步骤4：位置集有主节点
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "location_locationElection_at_2";

  // Step 0: get rg and node list
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName).slice(0, 3);

  // Step 1: set location for three nodes and check location primary
  setLocationForNodes(rg, nodeList, location);
  var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 2: stop a slave node, and check location primary
  var slaveList = getSlaveList(nodeList, primary1);
  var slaveNode = rg.getNode(slaveList[0].HostName, slaveList[0].svcname);
  slaveNode.stop();
  checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 3: stop another slave node, and check location primary
  var slaveNode = rg.getNode(slaveList[1].HostName, slaveList[1].svcname);
  slaveNode.stop();
  checkLocationHasNoPrimary(db, groupName, location, 34);

  // Restart replica group
  rg.start();
  checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Reset group info
  setLocationForNodes(rg, nodeList, "");
}
