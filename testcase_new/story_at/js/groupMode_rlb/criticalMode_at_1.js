/***************************************************************************************************
 * @Description: startCriticalMode 参数校验
 * @ATCaseID: criticalMode_at_1
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 4/03/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含5个节点的复制组
 * 测试场景：
 *    测试复制组使用不同类型的参数开启 criticalMode
 * 测试步骤：
 *    1. 使用不同类型的参数调用startCriticalMode()，查看结果
 * 期望结果：
 *    步骤1：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  commCheckBusinessStatus(db);

  // Option type in string, number, bool (it shoule be bson)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode("A");
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode(1);
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode(true);
  });

  // Location type in number, bool, bson (it shoule be string)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Location: { test: "test" } });
  });

  // NodeName type in number, bool, bson (it shoule be string)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ NodeName: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ NodeName: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ NodeName: { test: "test" } });
  });

  // Enforced type in string, number, bson (it shoule be bool)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Enforced: "true" });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Enforced: 0 });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ Enforced: { test: "test" } });
  });

  // MinKeepTime type in string, bool, bson (it shoule be number)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MinKeepTime: "0" });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MinKeepTime: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MinKeepTime: { test: "test" } });
  });

  // MaxKeepTime type in string, bool, bson (it shoule be number)
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MaxKeepTime: "0" });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MaxKeepTime: true });
  });
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.startCriticalMode({ MaxKeepTime: { test: "test" } });
  });
}
