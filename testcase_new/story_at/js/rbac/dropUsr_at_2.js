/***************************************************************************************************
 * @Description: 用户数大于1时，删除用户
 * @ATCaseID: dropUsr_at_1
 * @Author: Zhou Hongye
 * @TestlinkCase: 无
 * @Change    Activity:
 * Date       Who         Description
 * ========== =========== =========================================================
 * 08/04/2023 Zhou Hongye Test drop user
 **************************************************************************************************/

/*********************************************测试用例***********************************************
 * 环境准备：集群环境
 * 测试场景：存在仅一个_root用户和至少一个非_root用户时，可删除非_root用户，不可删除_root用户
 * 测试步骤：
 *  1. 集群中创建两个非_root用户
 *  2. 集群中创建_root用户
 *  3. 删除该_root用户，预期失败，报错 SDB_OPERATION_DENIED
 *  4. 分别删除两个非_root用户，预期删除成功
 * 期望结果：
 *  符合预期
 **************************************************************************************************/
main(test);
function test() {
   var user1 = "test_user1"
   var user2 = "test_user2"
   var user_root = "test_user_root"
   var pwd = "123";
   db.createUsr(user1, pwd);
   db.createUsr(user2, pwd);
   db.createUsr(user_root, pwd, { Roles: ["_root"] });
   assert.tryThrow( SDB_OPERATION_DENIED, function()
   {
      db.dropUsr(user_root, pwd );
   })
   db.dropUsr(user1, pwd);
   db.dropUsr(user2, pwd);
   db.dropUsr(user_root, pwd );
}