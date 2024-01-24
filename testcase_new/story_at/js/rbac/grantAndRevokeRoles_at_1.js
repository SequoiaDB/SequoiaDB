/***************************************************************************************************
 * @Description: 为角色授予或者撤销继承角色
 * @ATCaseID: grantAndRevokeRoles_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test grant and revoke roles to/from role
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：
 * 测试步骤：
 *  1. 创建角色role1，role2，role3，role3继承role2，role2继承role1
 *  2. 撤销role2继承的角色role1，检查角色继承关系
 *  3. 授予role3继承的角色role1，检查角色继承关系
 *  
 * 期望结果：
 *  授予与撤销角色成功，角色继承关系正确
 **************************************************************************************************/
main(test);
function test() {
   prepareTestRoles(db);
   db.revokeRolesFromRole(test_role2_name, [test_role1_name]);
   checkRolesOfRoleByName(db, test_role2_name, [], []);
   checkRolesOfRoleByName(
      db,
      test_role3_name,
      [test_role2_name],
      [test_role2_name]
   );
   db.grantRolesToRole(test_role3_name, [test_role1_name]);
   checkRolesOfRoleByName(
      db,
      test_role3_name,
      [test_role1_name, test_role2_name],
      [test_role1_name, test_role2_name]
   );
   cleanTestRoles(db);
}
