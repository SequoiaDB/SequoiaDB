/******************************************************************************
 * @Description   : seqDB-32889:revokePrivilegesFromRole接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32889";
   var roleName1 = "role_32889_1";
   var roleName2 = "role_32889_2";

   cleanRole( roleName1 );
   cleanRole( roleName2 );

   var role = {
      Role: roleName1, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find"]
      }]
   };
   db.createRole( role );
   var role = {
      Role: roleName2, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["insert", "update", "remove"]
      }]
   };
   db.createRole( role );


   try
   {
      //1、revokePrivilegesFromRole指定rolename为合法参数：string类型
      var revokeRole = [{ Resource: { cs: dbname, cl: "" }, Actions: ["find"] }];
      db.revokePrivilegesFromRole( roleName1, revokeRole );

      //2、revokePrivilegesFromRole指定rolename为非法参数：a、非string类型，如：true、array、1
      var revokeRole = [{ Resource: { cs: dbname, cl: "" }, Actions: ["find"] }];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( true, revokeRole );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( 1, revokeRole );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( [roleName1, roleName2], revokeRole );
      } );

      //3、revokePrivilegesFromRole指定role.privileges为合法参数：符合格式的array类型
      var revokeRole = [{ Resource: { cs: dbname, cl: "" }, Actions: ["find"] }];
      db.revokePrivilegesFromRole( roleName1, revokeRole );

      //4、revokePrivilegesFromRole指定privileges为非法参数：a、不包含Resource字段 b、不包含Actions字段 c、包含其他字段
      //d、Resource指定内容不正确 e、Actions指定内容不正确 f、role.Privileges指定非object类型
      var revokeRole = [{ Actions: ["find"] }];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( roleName1, revokeRole );
      } );

      var revokeRole = [{ Resource: { cs: dbname, cl: "" } }];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( roleName1, revokeRole );
      } );

      var revokeRole = [{ Resource: { collectionspace: dbname, cl: "" }, Actions: ["find"] }];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( roleName1, revokeRole );
      } );

      var revokeRole = [{ Resource: { cs: dbname, cl: "" }, Actions: ["find", "test"] }];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( roleName1, revokeRole );
      } );

      var revokeRole = [1];
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokePrivilegesFromRole( roleName1, revokeRole );
      } );

   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
   }
}
