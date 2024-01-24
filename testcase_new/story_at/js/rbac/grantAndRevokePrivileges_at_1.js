/***************************************************************************************************
 * @Description: 为角色授予或者撤销权限
 * @ATCaseID: grantAndRevokePrivileges_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test grant and revoke privileges to/from role
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：
 * 测试步骤：
 *  1. 创建角色role1，role2，role3，role3继承role2，role2继承role1
 *  2. 授予role2权限，检查角色权限
 *  3. 撤销role2权限，检查角色权限
 * 期望结果：
 *  授予撤销权限成功，角色权限正确
 **************************************************************************************************/
main(test);
function test() {
  prepareTestRoles(db);
  db.grantPrivilegesToRole(test_role2_name, [
    {
      Resource: { cs: "test_rbac2", cl: "test_rbac2" },
      Actions: ["find"],
    },
  ]);
  checkPrivilegesOfRoleByName(
    db,
    test_role2_name,
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["insert", "update"],
      },
      { Resource: { cs: "test_rbac2", cl: "test_rbac2" }, Actions: ["find"] },
    ],
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["find", "insert", "update"],
      },
      { Resource: { cs: "test_rbac2", cl: "test_rbac2" }, Actions: ["find"] },
    ]
  );
  db.revokePrivilegesFromRole(test_role2_name, [
    { Resource: { cs: "test_rbac", cl: "test_rbac" }, Actions: ["insert"] },
  ]);
  checkPrivilegesOfRoleByName(
    db,
    test_role2_name,
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["update"],
      },
      { Resource: { cs: "test_rbac2", cl: "test_rbac2" }, Actions: ["find"] },
    ],
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["find", "update"],
      },
      { Resource: { cs: "test_rbac2", cl: "test_rbac2" }, Actions: ["find"] },
    ]
  );
  cleanTestRoles(db);
}
