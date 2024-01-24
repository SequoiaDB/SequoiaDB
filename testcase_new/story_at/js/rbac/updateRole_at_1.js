/***************************************************************************************************
 * @Description: 更新角色
 * @ATCaseID: updateRole_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test update role
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：
 * 测试步骤：
 *  1. 创建角色role1，role2，role3，role3继承role2，role2继承role1
 *  2. 更新角色role2，使其不再继承role1，并更新其权限
 *  3. 检查角色继承关系和权限
 * 期望结果：
 *  更新角色成功，角色继承关系与权限正确
 **************************************************************************************************/
main(test);
function test() {
  prepareTestRoles(db);
  var new_role2 = {
    Privileges: [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["insert"],
      },
    ],
    Roles: [],
  };
  db.updateRole(test_role2_name, new_role2);
  checkRolesOfRole(JSON.parse(db.getRole(test_role2_name)), [], []);
  checkPrivilegesOfRole(
    JSON.parse(db.getRole(test_role2_name, { ShowPrivileges: true })),
    [{ Resource: { cs: "test_rbac", cl: "test_rbac" }, Actions: ["insert"] }],
    [{ Resource: { cs: "test_rbac", cl: "test_rbac" }, Actions: ["insert"] }]
  );
  checkRolesOfRole(
    JSON.parse(db.getRole(test_role3_name)),
    [test_role2_name],
    [test_role2_name]
  );
  checkPrivilegesOfRole(
    JSON.parse(db.getRole(test_role3_name, { ShowPrivileges: true })),
    [{ Resource: { cs: "test_rbac", cl: "test_rbac" }, Actions: ["remove"] }],
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["insert", "remove"],
      },
    ]
  );

  var new_role3 = {
    Roles: [test_role1_name],
  };
  db.updateRole(test_role3_name, new_role3);
  checkRolesOfRole(
    JSON.parse(db.getRole(test_role3_name)),
    [test_role1_name],
    [test_role1_name]
  );
  checkPrivilegesOfRole(
    JSON.parse(db.getRole(test_role3_name, { ShowPrivileges: true })),
    [{ Resource: { cs: "test_rbac", cl: "test_rbac" }, Actions: ["remove"] }],
    [
      {
        Resource: { cs: "test_rbac", cl: "test_rbac" },
        Actions: ["find", "remove"],
      },
    ]
  );
  cleanTestRoles(db);
}
