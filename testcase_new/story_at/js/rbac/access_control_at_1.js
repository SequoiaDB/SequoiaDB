/***************************************************************************************************
 * @Description: 集合与集合空间命令的权限检查
 * @ATCaseID: access_control_at_1
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
 *    createIndex
 *    dropIndex
 *    count
 *    listIndexes
 *    getQueryMeta
 *    getCS
 *    getCL
 *    split
 *    listLobs
 *    truncate
 *    getDetail
 *    getCollectionStat
 *    getIndexStat
 *    alter(collection)
 *    createCL
 *    dropCL
 *    renameCL
 *    alter(collectionspace)
 *
 * 测试步骤：
 *  1. 创建数据库分区表
 *  2. 创建角色1和角色2，角色1一般包含获取集合空间、集合对象的权限，角色2包含待测试命令的所需权限。
 *  3. 角色1授予用户
 *  4. 执行命令，期望得到SDB_NO_PRIVILEGES
 *  5. 角色2授予用户
 *  6. 执行命令，期望成功
 * 期望结果：
 *  1. 角色1授予用户后，执行命令，期望得到SDB_NO_PRIVILEGES
 *  2. 角色2授予用户后，执行命令，期望成功
 **************************************************************************************************/
testConf.skipOneGroup = true;
testConf.csName = COMMCSNAME + "_rbac";
testConf.clName = COMMCLNAME + "_rbac";
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main(test);
function test(testPara) {
  ensurePrivilegeCheckEnabled(testPara, testAccessControl);
}
function testAccessControl(testPara) {
  db.getCS(testConf.csName)
    .getCL(testConf.clName)
    .alter({
      ShardingKey: { id: 1 },
      ShardingType: "hash",
      Partition: 4096,
    });
  db.getCS(testConf.csName)
    .getCL(testConf.clName)
    .createIndex("index_for_stat", { number: 1 });
  db.analyze({ CollectionSpace: testConf.csName });
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
          Resource: { cs: testConf.csName, cl: testConf.clName },
          Actions: ["createIndex", "dropIndex"],
        },
      ],
      function (user) {
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .createIndex("a", { a: 1 });
        user.getCS(testConf.csName).getCL(testConf.clName).dropIndex("a");
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
          Resource: { cs: testConf.csName, cl: testConf.clName },
          Actions: ["getDetail", "insert"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).getCL(testConf.clName).insert({ a: 1 });
        user.getCS(testConf.csName).getCL(testConf.clName).count();
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .count({ Collection: "a", CollectionSpace: "b" });
        user.getCS(testConf.csName).getCL(testConf.clName).listIndexes();
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .find()
          .getQueryMeta();
        user.getCS(testConf.csName).getCL(testConf.clName).listLobs();
        user.getCS(testConf.csName).getCL(testConf.clName).getDetail();
        user.getCS(testConf.csName).getCL(testConf.clName).getCollectionStat();
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .getIndexStat("index_for_stat");
      }
    );

    checkAccessControl(
      db,
      [],
      [
        {
          Resource: { cs: testConf.csName, cl: "" },
          Actions: ["testCS", "testCL"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).getCL(testConf.clName);
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
          Resource: { cs: testConf.csName, cl: testConf.clName },
          Actions: ["split"],
        },
      ],
      function (user) {
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .split(
            testPara.srcGroupName,
            testPara.dstGroupNames[0],
            { Partition: 2048 },
            { Partition: 4096 }
          );
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
          Resource: { cs: testConf.csName, cl: testConf.clName },
          Actions: ["truncate"],
        },
      ],
      function (user) {
        user.getCS(testConf.csName).getCL(testConf.clName).truncate();
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
          Resource: { cs: testConf.csName, cl: testConf.clName },
          Actions: ["alterCL"],
        },
      ],
      function (user) {
        user
          .getCS(testConf.csName)
          .getCL(testConf.clName)
          .alter({ ReplSize: -1 });
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
          Actions: ["createCL"],
        },
      ],
      function (user) {
        ignoreError(function () {
          db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac2");
        });
        user.getCS(testConf.csName).createCL(COMMCLNAME + "_rbac2");
        db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac2");
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
          Actions: ["dropCL"],
        },
      ],
      function (user) {
        ignoreError(function () {
          db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac2");
        });
        db.getCS(testConf.csName).createCL(COMMCLNAME + "_rbac2");
        user.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac2");
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
          Actions: ["renameCL"],
        },
      ],
      function (user) {
        ignoreError(function () {
          db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac2");
        });
        ignoreError(function () {
          db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac3");
        });
        db.getCS(testConf.csName).createCL(COMMCLNAME + "_rbac2");
        user
          .getCS(testConf.csName)
          .renameCL(COMMCLNAME + "_rbac2", COMMCLNAME + "_rbac3");
        db.getCS(testConf.csName).dropCL(COMMCLNAME + "_rbac3");
      }
    );
  } finally {
    cleanTestAccessControl(db);
  }
}
