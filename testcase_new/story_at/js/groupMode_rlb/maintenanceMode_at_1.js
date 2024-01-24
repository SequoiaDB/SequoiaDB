/***************************************************************************************************
 * @Description: startMaintenanceMode 参数校验
 * @ATCaseID: maintenanceMode_at_1.js
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
 *    测试复制组使用不同类型的参数开启 criticalMode
 * 测试步骤：
 *    1. 使用不同类型的参数调用startMaintenanceMode()，查看结果
 * 期望结果：
 *    步骤1：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  commCheckBusinessStatus(db);

  // Option type in string, number, bool (it shoule be bson)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode("A");
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode(1);
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode(true);
  });

  // Location type in number, bool, bson (it shoule be string)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ Location: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ Location: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ Location: { test: "test" } });
  });

  // NodeName type in number, bool, bson (it shoule be string)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ NodeName: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ NodeName: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ NodeName: { test: "test" } });
  });

  // MinKeepTime type in string, bool, bson (it shoule be number)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MinKeepTime: "A" });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MinKeepTime: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MinKeepTime: { test: "test" } });
  });

  // MaxKeepTime type in string, bool, bson (it shoule be number)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MaxKeepTime: "A" });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MaxKeepTime: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startMaintenanceMode({ MaxKeepTime: { test: "test" } });
  });
}
