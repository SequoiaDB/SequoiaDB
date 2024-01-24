/***************************************************************************************************
 * @Description: cls 多节点多位置集 升主
 * @ATCaseID: locationElection_at_4
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
 *    测试多节点多位置集的升主/降备
 * 测试步骤：
 *    1. 给同一复制组的其中两个节点设置相同的位置 "A"，另一个节点设置位置 "B"，查看两个位置集内是否有主节点产生
 *    2. 把 "A" 位置集的主节点设置成 "B"，查看两个位置集内是否有主节点
 *    3. 把 "B" 位置集的主节点设置成 ""，查看两个位置集内是否有主节点
 * 期望结果：
 *    步骤1-3：位置集有主节点
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location1 = "location_locationElection_at_4_1";
  var location2 = "location_locationElection_at_4_2";

  // Step 0: create a group contains three nodes
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName).slice(0, 3);

  // Step 1: set location1 for two nodes and check location primary in location1
  setLocationForNodes(rg, nodeList.slice(0, 2), location1);
  var primary1 = checkAndGetLocationHasPrimary(db, groupName, location1, 34);

  // Step 1: set location2 for another node and check location primary in location2
  setLocationForNodes(rg, nodeList.slice(2, 3), location2);
  checkAndGetLocationHasPrimary(db, groupName, location2, 34);

  // Step 2: set location2 for location1's primary node
  var nodeName = primary1.split(":");
  var node = rg.getNode(nodeName[0], nodeName[1]);
  node.setLocation(location2);
  // check location primary in location1
  checkAndGetLocationHasPrimary(db, groupName, location1, 34);
  // check location primary in location2
  var primary2 = checkAndGetLocationHasPrimary(db, groupName, location2, 1);

  // Step 3: set "" for location2's primary, and check location primary in location2
  var nodeName = primary2.split(":");
  var node = rg.getNode(nodeName[0], nodeName[1]);
  node.setLocation("");
  checkAndGetLocationHasPrimary(db, groupName, location2, 34);

  // Reset group info
  setLocationForNodes(rg, nodeList, "");
}
