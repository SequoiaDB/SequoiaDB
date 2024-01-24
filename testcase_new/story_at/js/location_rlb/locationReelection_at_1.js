/***************************************************************************************************
 * @Description: cls 单节点单位置集 重新选主的参数校验
 * @ATCaseID: locationReelection_at_1
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 11/28/2022 Chengxi Liao  Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含两个节点的复制组
 * 测试场景：
 *    测试单节点单位置集的重新选主和参数校验
 * 测试步骤：
 *    1. 获取一个节点，并给该节点设置位置 "A"，查看该节点是否升主
 *    2. 使用不同的参数调用rg.reelectLocation()，查看结果
 * 期望结果：
 *    步骤1：节点为当前位置集的主节点
 *    步骤2：返回/报错信息正确
 *
 * 说明：Location 选举周期的最长时间为 17s，使用两个选举周期来判断超时。
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_location_rlb";
  var location = "location_locatioReelection_at_1";

  // Step 0: get rg and node
  commCheckBusinessStatus(db);
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);

  // Step 1: set location for two node and check location primary in location
  setLocationForNodes(rg, nodeList.slice(0, 2), location);
  checkAndGetLocationHasPrimary(db, groupName, location, 34);

  // Step 2: use different argument in rg.reelectLocation and check the result
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.reelectLocation(1);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.reelectLocation(true);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.reelectLocation(null);
  });

  assert.tryThrow(SDB_OUT_OF_BOUND, function () {
    rg.reelectLocation();
  });

  // Node is in replica group but not in location
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.reelectLocation(location, nodeList[2].NodeID);
  });

  // Remove group info
  setLocationForNodes(rg, nodeList.slice(0, 2), "");
}
