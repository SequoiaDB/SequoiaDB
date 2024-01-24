/***************************************************************************************************
 * @Description: startMaintenanceMode 参数校验
 * @ATCaseID: maintenanceMode_at_2.js
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 7/12/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含5个节点的复制组
 * 测试场景：
 *    测试在已经开启 Critical 模式的复制组上开启 Maintenance 模式
 * 测试步骤：
 *    1. 在复制组中开启 Critical 模式
 *    2. 在复制组开启 Maintenance 模式
 * 期望结果：
 *    步骤2：返回/报错信息正确
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

  // Start Critical Mode in replica primary
  rg.startCriticalMode({ NodeName: replPrimaryName, MinKeepTime: 5, MaxKeepTime: 10 });
  checkCriticalNodeMode(db, groupName, replPrimaryName);

  // Start Maintenance Mode in slave node
  assert.tryThrow(SDB_OPERATION_CONFLICT, function () {
    rg.startMaintenanceMode({ NodeName: slaveNodelList[0], MinKeepTime: 5, MaxKeepTime: 10 });
  });

  // Clear environment
  rg.stopCriticalMode();
  rg.stopMaintenanceMode();
  checkNormalGrpMode(db, groupName);
}
