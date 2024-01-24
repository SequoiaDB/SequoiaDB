/***************************************************************************************************
 * @Description: 日志回滚时，保存回滚的日志
 * @ATCaseID: consultrollbackLlogon_at_2
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
 *    1. 构造cosult日志回滚场景
 *    2. 查看生成的回滚数据文件
 *    3. sdbreplay回放回滚文件
 * 期望结果：
 *    回放文件成功
 *
 **************************************************************************************************/

testConf.skipStandAlone = true;

main(test);
function test() {
  var groupName = "save_rollback_log_rlb";
  var csName = CHANGEDPREFIX + "save_rollback_log_rlb_cs";
  var clName = CHANGEDPREFIX + "save_rollback_log_rlb_cl";

  commCreateCS(db, csName);
  var cl = commCreateCL(db, csName, clName, { Group: groupName });
  for (i = 0; i < 1000; i++) {
    cl.insert({ a: i });
  }
  var rg = db.getRG(groupName);
  var primary = rg.getMaster();
  var nodes = commGetGroupNodes(db, groupName);
  nodes = getSlaveList(nodes, primary);

  var node1 = rg.getNode(nodes[0].HostName, nodes[0].svcname);
  var node2 = rg.getNode(nodes[1].HostName, nodes[1].svcname);

  // 构造consult回滚场景
  node1.setLocation("a");
  node1.stop();
  for (i = 0; i < 1000; i++) {
    cl.insert({ a: i });
  }
  primary.stop();
  node2.stop();
  node1.start();

  var succeed = false;
  while (!succeed) {
    try {
      rg.startCriticalMode({ Location: "a", MinKeepTime: 1, MaxKeepTime: 5 });
      succeed = true;
    } catch (e) {
      if (e != SDB_TIMEOUT) {
        throw new Error(e);
      }
    }
  }
  primary.start();
  node2.start();

  // 回放文件，并检查是否回放成功
  var cmd = new Cmd();
  execSdbReplay(cmd, groupName, "archive", COORDHOSTNAME, COORDSVCNAME);
  assert.equal(cl.count(), 2000);

  commDropCS(db, csName);
}
