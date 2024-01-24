/***************************************************************************************************
 * @Description: cls 单节点单位置集 升主
 * @ATCaseID: locationElection_at_1
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/22/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含单个节点的复制组
 * 测试场景：
 *    测试单节点单位置集的升主
 * 测试步骤：
 *    1. 获取一个节点，并给该节点设置位置 "A"，查看该节点是否升主
 *    2. 重启该节点，查看节点是否升主
 *    3. 给该节点设置位置 "B"，查看该节点是否升主
 * 期望结果：
 *    步骤1-3：节点为当前位置集的主节点
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location1 = "location_locationElection_at_1_1";
  var location2 = "location_locationElection_at_1_2";

  // Step 0: get rg and a slave node
  var rg = db.getRG(groupName);
  var node = rg.getSlave();

  // Step 1: set location1 for node and check location primary in location1
  node.setLocation(location1);
  checkAndGetLocationHasPrimary(db, groupName, location1, 34);

  // Step 2: restart node and check location primary in location1
  node.stop();
  node.start();
  checkAndGetLocationHasPrimary(db, groupName, location1, 34);

  // Step 3: set location2 for node and check location primary in location2
  node.setLocation(location2);
  checkAndGetLocationHasPrimary(db, groupName, location2, 34);

  // Reset group info
  node.setLocation("");
}
