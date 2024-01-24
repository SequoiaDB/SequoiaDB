/***************************************************************************************************
 * @Description: cls 多节点单位置集使用 -9 信号停止节点
 * @ATCaseID: locationElection_at_6
 *
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 12/7/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含四个节点的复制组
 * 测试场景：
 *    测试多节点单位置集使用 -9 信号停止节点场景
 * 测试步骤：
 *    1. 给复制组的全部节点设置相同的位置 "A"，查看位置集内是否有主节点产生
 *    2. 使用 cmd 向位置集主节点发送 kill -9 的命令停止节点，查看位置集内是否有主节点产生
 *    3. 使用 cmd 向位置集所有备节点发送 kill -9 的命令停止节点，查看位置集内是否有主节点产生
 *    4. 使用 cmd 向位置集全部节点发送 kill -9 的命令停止节点，查看位置集内是否有主节点产生
 * 期望结果：
 *    步骤1-4：位置集有主节点
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "locationElection_at_6";

  // Step 0: get rg, nodeList
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);

  // Step 1: set location for two nodes and check location primary
  setLocationForNodes(rg, nodeList, location);
  var primary = checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 2: use kill -9 to stop location primary, and check new location primary
  var nodeName = primary.split(":");
  var primaryName = [{ HostName: nodeName[0], svcname: nodeName[1] }];
  killNodes(primaryName);
  var primary1 = checkAndGetLocationHasPrimary(db, groupName, location, 120);

  // Step 2: use kill -9 to stop location slave nodes, and check new location primary
  var slaveList = getSlaveList(nodeList, primary1);
  killNodes(slaveList);
  checkAndGetLocationHasPrimary(db, groupName, location, 120);

  // Step 3: use kill -9 to stop all location nodes, and check new location primary
  killNodes(nodeList);
  checkAndGetLocationHasPrimary(db, groupName, location, 120);

  // Reset group info
  setLocationForNodes(rg, nodeList, "");
}
