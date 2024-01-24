/******************************************************************************
 * @Description   : seqDB-32887:updateRole接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32887";
   var roleName1 = "role_32887_1";
   var roleName2 = "role_32887_2";

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
      //1、updateRole指定rolename为合法参数：string类型
      var updateInfo = { Privileges: [{ Resource: { cs: dbname, cl: "" }, Actions: ["find", "insert"] }] };
      db.updateRole( roleName1, updateInfo );

      //2、updateRole指定rolename为非法参数： a、非string类型，如：true、array、1
      var updateInfo = { Privileges: [{ Resource: { cs: dbname, cl: "" }, Actions: ["find", "insert"] }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( true, updateInfo );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( 1, updateInfo );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( [roleName1, roleName2], updateInfo );
      } );

      //3、updateRole指定role为合法参数：符合格式的object类型
      var updateInfo = {
         Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "insert", "update"]
         }]
      };
      db.updateRole( roleName1, updateInfo );

      //4、updateRole指定role为非法参数： a、非object类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, "true" );
      } );

      //6、updateRole指定role.Privileges为非法参数： a、不包含Resource字段 b、不包含Actions字段 d、Resource指定内容不正确
      //            e、Actions指定内容不正确 f、role.Privileges指定非object类型
      var updateInfo = { Privileges: [{ Actions: ["find", "insert", "update"] }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, updateInfo );
      } );

      var updateInfo = { Privileges: [{ Resource: { cs: dbname, cl: "" } }] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, updateInfo );
      } );

      var updateInfo = {
         Privileges: [{
            Resource: { collectionspace: dbname, cl: "" },
            Actions: ["find", "insert", "update", "delete"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, updateInfo );
      } );

      var updateInfo = {
         Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "insert", "update", "test_action"]
         }]
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, updateInfo );
      } );

      var updateInfo = { Privileges: [100] };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName1, updateInfo );
      } );

      //8、updateRole指定role.Roles为非法参数： a、非array类型，如：10、true、"string"
      var updateInfo = {
         Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "insert", "update"]
         }], Roles: 10
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName2, updateInfo );
      } );

      var updateInfo = {
         Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "insert", "update",]
         }], Roles: true
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName2, updateInfo );
      } );

      var updateInfo = {
         Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "insert", "update",]
         }], Roles: "string"
      };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateRole( roleName2, updateInfo );
      } );

   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
   }
}
