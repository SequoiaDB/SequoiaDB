/***************************************************************************************************
 * @Description: startCriticalMode 参数校验
 * @ATCaseID: criticalMode_at_2_error
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 4/03/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含7个节点的复制组，复制组只有3个节点启动
 * 测试场景：
 *    测试复制组使用 Location 参数开启 criticalMode
 * 测试步骤：
 *    1. 调用startCriticalMode()，使用 Location 参数开启，查看结果是否正确
 * 期望结果：
 *    步骤1：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_grpMode";
  var location1 = "criticalMode_at_2_A";
  var location2 = "criticalMode_at_2_B";
  commCheckBusinessStatus(db);

  // Get rg and nodes
  var rg = db.getRG(groupName);
  var nodeList = commGetGroupNodes(db, groupName);
  var replPrimary = rg.getMaster();
  var replPrimaryName = replPrimary.getHostName() + ":" + replPrimary.getServiceName();
  var slaveNodelList = getSlaveList(nodeList, replPrimaryName);

  // Stop 4 nodes
  stopNodes(rg, slaveNodelList.slice(2, 6));

  // Set Location for nodes
  setLocationForNodes(rg, slaveNodelList.slice(0, 1), location1);
  setLocationForNodes(rg, slaveNodelList.slice(1, 2), location2);

  // Check Location parameter
  var str = new Array(258).join("a");
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: str, MinKeepTime: 5, MaxKeepTime: 10 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: "", MinKeepTime: 5, MaxKeepTime: 10 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: "nonExistLocation", MinKeepTime: 5, MaxKeepTime: 10 });
  });

  // Start critical mode in location1
  rg.startCriticalMode({ Location: location1, MinKeepTime: 5, MaxKeepTime: 10 });
  checkCriticalLocationMode(db, groupName, location1);

  // Start critical mode in location2
  rg.startCriticalMode({ Location: location2, MinKeepTime: 5, MaxKeepTime: 10 });
  checkCriticalLocationMode(db, groupName, location2);

  // Start critical mode in location2 again
  rg.startCriticalMode({ Location: location2, MinKeepTime: 5, MaxKeepTime: 10 });
  checkCriticalLocationMode(db, groupName, location2);

  // Remove nodes' location, check if group has stopped critical mode
  // Use manual test for this part, because it will at least wait for 1 minute
  // setLocationForNodes(rg, slaveNodelList.slice(0, 2), "");
  // checkNormalGrpMode(db, groupName);

  // Start critical mode in node
  rg.startCriticalMode({ NodeName: replPrimaryName, MinKeepTime: 5, MaxKeepTime: 10 });
  checkCriticalNodeMode(db, groupName, replPrimaryName);

  // Stop Critical mode
  rg.stopCriticalMode();
  checkNormalGrpMode(db, groupName);

  // Reset group info
  setLocationForNodes(rg, nodeList, "");
  rg.stopCriticalMode();
  rg.start();
}
