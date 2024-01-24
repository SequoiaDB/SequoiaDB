/******************************************************************************
 * @Description   : seqDB-32885:getRole接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.08.31
 * @LastEditTime  : 2023.08.31
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32885";
   var roleName1 = "role_32885_1";
   var roleName2 = "role_32885_2";

   cleanRole( roleName1 );

   var role = {
      Role: roleName1, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find"]
      }]
   };
   db.createRole( role );

   try
   {
      // 1、getRole指定rolename为合法参数：string类型
      var role = db.getRole( roleName1 );

      // 2、getRole指定rolename为非法参数：
      // a、非string类型，如：1、true、array
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( 1 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( [roleName1, roleName2] );
      } );

      // 3、getRole指定options为合法参数：符合预期的object类型
      var role = db.getRole( roleName1, { ShowPrivileges: true } );

      // 4、getRole指定options为非法参数：
      // a、非object类型，如："string"、1、true
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( roleName1, "string" );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( roleName1, 1 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.getRole( roleName1, true );
      } );

      // 5、getRole指定options.ShowPrivileges为合法参数：bool类型
      var role = db.getRole( roleName1, { ShowPrivileges: true } );

      // 6、getRole指定options.ShowPrivileges为非法参数：
      // a、非bool类型，如："string"、1、array
      // 会自动修正为参数默认值

      db.getRole( roleName1, { ShowPrivileges: "string" } );

      db.getRole( roleName1, { ShowPrivileges: 1 } );

      db.getRole( roleName1, { ShowPrivileges: [1, 2, 3] } );

   }
   finally
   {
      cleanRole( roleName1 );
   }

}
