/***************************************************************************************************
 * @Description: 集合与集合空间命令的权限检查
 * @ATCaseID: access_control_at_2
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/11/2023 Zhou Hongye Test access control of collection and collection space
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：有至少两个数据复制组的集群环境
 * 测试场景：
 *  测试命令如下：
 *    attachCL
 *    copyIndex
 *    detachCL
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

main(test);
function test(testPara) {
  ensurePrivilegeCheckEnabled(testPara, testAccessControl);
}

function testAccessControl(testPara) {
  db.getCS(testConf.csName).createCL("maincl", {
    IsMainCL: true,
    ShardingKey: { create_date: 1 },
    ShardingType: "range",
  });

  db.getCS(testConf.csName).createCL("year2018");
  db.getCS(testConf.csName).getCL("maincl").createIndex("IDIdx", { ID: 1 });

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
          Resource: { cs: testConf.csName, cl: "maincl" },
          Actions: ["attachCL", "find"],
        },
        {
          Resource: { cs: testConf.csName, cl: "year2018" },
          Actions: ["find", "update", "insert", "remove"],
        },
      ],
      function (user) {
        user
          .getCS(testConf.csName)
          .getCL("maincl")
          .attachCL(testConf.csName + ".year2018", {
            LowBound: { create_date: "201801" },
            UpBound: { create_date: "201901" },
          });
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
          Resource: { cs: testConf.csName, cl: "maincl" },
          Actions: ["copyIndex"],
        },
      ],
      function (user) {
        user
          .getCS(testConf.csName)
          .getCL("maincl")
          .copyIndex(testConf.csName + ".year2018");
        user.getCS(testConf.csName).getCL("maincl").copyIndex();
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
          Resource: { cs: testConf.csName, cl: "maincl" },
          Actions: ["detachCL"],
        },
      ],
      function (user) {
        ignoreError(function () {
          db.getCS(testConf.csName)
            .getCL("maincl")
            .detachCL(testConf.csName + ".year2018");
        });
        db.getCS(testConf.csName)
          .getCL("maincl")
          .attachCL(testConf.csName + ".year2018", {
            LowBound: { create_date: "201801" },
            UpBound: { create_date: "201901" },
          });
        user
          .getCS(testConf.csName)
          .getCL("maincl")
          .detachCL(testConf.csName + ".year2018");
      }
    );
  } finally {
    cleanTestAccessControl(db);
  }
}
