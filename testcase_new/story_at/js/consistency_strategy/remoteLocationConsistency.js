/***************************************************************************************************
 * @Description: remotelocationconsistency配置参数校验
 * @ATCaseID: remoteLocationConsistency
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 06/10/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *
 * 测试步骤：
 *    1. 更新配置
 *    2. 快照查看参数值实际结果和预期结果是否一致
 * 期望结果：
 *    报错-6
 *
 **************************************************************************************************/

main(test);
function test() {
  // 校验合法参数
  var config = { remotelocationconsistency: true };
  var expConfig = { remotelocationconsistency: "TRUE" };
  db.updateConf(config);
  checkSnapshot(db, expConfig);

  var config = { remotelocationconsistency: false };
  var expConfig = { remotelocationconsistency: "FALSE" };
  db.updateConf(config);
  checkSnapshot(db, expConfig);

  // 校验非法参数
  var config = { metacacheexpired: null };
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.updateConf(config);
  });
  checkSnapshot(db, expConfig);

  var config = { metacacheexpired: "" };
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.updateConf(config);
  });
  checkSnapshot(db, expConfig);

  var config = { metacacheexpired: -1 };
  assert.tryThrow(SDB_INVALIDARG, function () {
    db.updateConf(config);
  });
}

function checkSnapshot(sdb, option) {
  var cursor = sdb.snapshot(SDB_SNAP_CONFIGS, {}, option);
  while (cursor.next()) {
    var obj = cursor.current().toObj();
    for (var key in option) {
      assert.equal(obj[key], option[key]);
    }
  }
  cursor.close();
}
