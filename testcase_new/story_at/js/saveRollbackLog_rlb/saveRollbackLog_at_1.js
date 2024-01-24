/***************************************************************************************************
 * @Description: consultrollbacklogon配置参数校验
 * @ATCaseID: consultrollbacklogon
 * @Author: Huang Youquan
 * @TestlinkCase: 无
 * @Change Activity:
 * Date       Who           Description
 * ========== ============= =========================================================
 * 07/07/2023 Huang Youquan    Init
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：正常集群
 * 测试场景：
 *
 * 测试步骤：
 *    1. 更新配置
 *    2. 快照查看参数值实际结果和预期结果是否一致
 * 期望结果：
 *    参数可以配置
 *
 **************************************************************************************************/

main(test);
function test() {
  // 校验合法参数
  var config = { consultrollbacklogon: false };
  var expConfig = { consultrollbacklogon: "FALSE" };
  db.updateConf(config);
  checkSnapshot(db, expConfig);

  var config = { consultrollbacklogon: true };
  var expConfig = { consultrollbacklogon: "TRUE" };
  db.updateConf(config);
  checkSnapshot(db, expConfig);

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
