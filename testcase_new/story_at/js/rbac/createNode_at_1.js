/***************************************************************************************************
 * @Description: create
 * @ATCaseID: createNode_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/11/2023 Zhou Hongye Test access control of create node
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：普通集群环境
 * 测试场景：
 *   createNode权限检查
 *
 * 测试步骤：
 *  1. 创建分区表
 *  2. 创建角色1和角色2，角色1一般包含获取集合空间、集合对象的权限，角色2包含待测试命令的所需权限。
 *  3. 角色1授予用户
 *  4. 执行命令，期望得到SDB_NO_PRIVILEGES
 *  5. 角色2授予用户
 *  6. 执行命令，期望成功
 * 期望结果：
 *  1. 角色1授予用户后，执行命令，期望得到SDB_NO_PRIVILEGES
 *  2. 角色2授予用户后，执行命令，期望成功
 **************************************************************************************************/

main(test);
function test(testPara) {
  ensurePrivilegeCheckEnabled(testPara, testAccessControl);
}
function testAccessControl(testPara) {
  try {
    checkAccessControl(
      db,
      [
        {
          Resource: { Cluster: true },
          Actions: ["createRG"],
        },
      ],
      [
        {
          Resource: { Cluster: true },
          Actions: ["createNode"],
        },
      ],
      function (user) {
        var rg_name = "test_rg_at_1";
        var host = System.getHostName();
        var svc = parseInt(RSRVPORTBEGIN) + 10;
        var dbpath = RSRVNODEDIR + "data/" + svc;
        try {
          var rg = user.createRG(rg_name);
          rg.createNode(host, svc, dbpath);
        } finally {
          try {
            var rg = db.getRG(rg_name);
            rg.removeNode(host, svc);
          } catch (e) {}
          db.removeRG(rg_name);
        }
      }
    );
  } finally {
    cleanTestAccessControl(db);
  }
}
