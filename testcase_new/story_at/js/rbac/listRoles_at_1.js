/***************************************************************************************************
 * @Description: 列出所有角色
 * @ATCaseID: listRoles_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test list roles
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：
 * 测试步骤：
 *  1. 创建角色role1，role2，role3，role3继承role2，role2继承role1
 *  2. 列出所有角色，检查角色继承关系
 * 期望结果：
 *  列出角色成功，角色继承关系正确
 **************************************************************************************************/

main(test);
function test() {
   prepareTestRoles(db);
   var cursor = db.listRoles();
   var roles_map = {};
   while( cursor.next() )
   {
      var role = JSON.parse(cursor.current());
      roles_map[role.Role] = role;
   }
   checkRolesOfRole(roles_map[test_role1_name], [], []);
   checkRolesOfRole(roles_map[test_role2_name], [test_role1_name], [test_role1_name]);
   checkRolesOfRole(roles_map[test_role3_name], [test_role2_name], [test_role1_name, test_role2_name]);
   cleanTestRoles(db);
}