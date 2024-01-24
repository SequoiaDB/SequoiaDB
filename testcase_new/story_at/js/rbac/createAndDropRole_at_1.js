/***************************************************************************************************
 * @Description: 创建、删除角色
 * @ATCaseID: createAndDropRole_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test create and drop role
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：
 * 测试步骤：
 *  1. 创建角色role1，role2，role3，role3继承role2，role2继承role1
 *  2. 检查角色继承关系
 *  3. 删除角色
 * 期望结果：
 *  创建角色成功，角色继承关系正确，删除角色成功
 **************************************************************************************************/
main(test);
function test() {
  prepareTestRoles(db);
  checkRolesOfRoleByName(db, test_role1_name, [], []);
  checkRolesOfRoleByName(
    db,
    test_role2_name,
    [test_role1_name],
    [test_role1_name]
  );
  checkRolesOfRoleByName(
    db,
    test_role3_name,
    [test_role2_name],
    [test_role1_name, test_role2_name]
  );
  db.dropRole(test_role1_name);
  checkRolesOfRoleByName(db, test_role2_name, [], []);
  checkRolesOfRoleByName(
    db,
    test_role3_name,
    [test_role2_name],
    [test_role2_name]
  );
  db.dropRole(test_role2_name);
  checkRolesOfRoleByName(db, test_role3_name, [], []);
  cleanTestRoles(db);
}
