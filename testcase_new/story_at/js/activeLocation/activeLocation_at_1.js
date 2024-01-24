/***************************************************************************************************
 * @Description: setActiveLocation 参数校验
 * @ATCaseID: activeLocation_at_1
 * @Author: Chengxi Liao
 * @TestlinkCase: 无（由测试人员维护，在测试阶段如果有测试场景引用本和例，则在此处填写 Testlink 用例编号，
 *                    并在 Testlink 系统中标记本用例文件名）
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 2/28/2023 Chengxi Liao   Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：一个包含2个节点的复制组
 * 测试场景：
 *    测试复制组使用不同的参数设置 ActiveLocation
 * 测试步骤：
 *    1. 使用不同的参数调用setActiveLocation()，查看结果
 * 期望结果：
 *    步骤1：返回/报错信息正确
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "group_activeLocation_2";
  var str = new Array(258).join("a");
  commCheckBusinessStatus(db);

  /*********************************rg.setActiveLocation****************************/

  // Step 0: get rg
  var rg = db.getRG(groupName);

  // Step 1: use different argument in rg.setActiveLocation and check the result
  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation(1);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation(true);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation(null);
  });

  assert.tryThrow(SDB_OUT_OF_BOUND, function () {
    rg.setActiveLocation();
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation("nonExistLocation");
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    rg.setActiveLocation(str);
  });

  /*******************************domain.setActiveLocation****************************/

  var domainName = "domain_location";

  // Step 0: create domain
  var domain = db.createDomain(domainName, [groupName]);

  // Step 1: use different argument in domain.setActiveLocation and check the result
  assert.tryThrow(SDB_INVALIDARG, function () {
    domain.setActiveLocation(1);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    domain.setActiveLocation(true);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    domain.setActiveLocation(null);
  });

  assert.tryThrow(SDB_OUT_OF_BOUND, function () {
    domain.setActiveLocation();
  });

  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    domain.setActiveLocation("nonExistLocation");
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    domain.setActiveLocation(str);
  });

  // Remove domain
  db.dropDomain(domainName);

  /**********************************dc.setActiveLocation****************************/

  // Step 0: create domain
  var dc = db.getDC();

  // Step 1: use different argument in domain.setActiveLocation and check the result
  assert.tryThrow(SDB_INVALIDARG, function () {
    dc.setActiveLocation(1);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dc.setActiveLocation(true);
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dc.setActiveLocation(null);
  });

  assert.tryThrow(SDB_OUT_OF_BOUND, function () {
    dc.setActiveLocation();
  });

  assert.tryThrow(SDB_COORD_NOT_ALL_DONE, function () {
    dc.setActiveLocation("nonExistLocation");
  });

  assert.tryThrow(SDB_INVALIDARG, function () {
    dc.setActiveLocation(str);
  });
}
