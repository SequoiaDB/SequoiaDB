/***************************************************************************************************
 * @Description: 集合与集合空间命令的权限检查
 * @ATCaseID: listCollections_at_1.js
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/22/2023 Zhou Hongye Test access control of list collections
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：普通集群环境
 * 测试场景：
 *  测试listCollections的权限检查
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
testConf.csName = COMMCSNAME + "_rbac";
testConf.clName = COMMCSNAME + "_rbac";

main(test);
function test(testPara) {
  ensurePrivilegeCheckEnabled(db, testAccessControl);
}
function testAccessControl(testPara) {
  db.getCS(testConf.csName).getCL(testConf.clName);
  try {
    checkAccessControl(
      db,
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["testCS", "testCL"],
        },
      ],
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["find"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).listCollections();
      }
    );

    checkAccessControl(
      db,
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["testCS", "testCL"],
        },
      ],
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["getDetail"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).listCollections();
      }
    );

    checkAccessControl(
      db,
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["testCS", "testCL"],
        },
      ],
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["listCollections"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).listCollections();
      }
    );

    checkAccessControl(
      db,
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["testCS", "testCL"],
        },
      ],
      [
        {
          Resource: { Cluster: true },
          Actions: ["list"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).listCollections();
      }
    );
  } finally {
    cleanTestAccessControl(db);
  }
}
