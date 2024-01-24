/******************************************************************************
 * @Description   : seqDB-32891:revokeRolesFromRole接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32891";
   var roleName1 = "role_32891_1";
   var roleName2 = "role_32891_2";

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
      }], Roles: [roleName1]
   };
   db.createRole( role );


   try
   {
      //1、revokeRolesFromRole指定rolename为合法参数：string类型
      db.revokeRolesFromRole( roleName2, [roleName1] );
      db.revokeRolesFromRole( roleName2, [] );

      //2、revokeRolesFromRole指定rolename为非法参数：a、非string类型，如：true、array、1、""
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( true, [roleName1] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( 1, [roleName1] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( [], [roleName1] );
      } );

      //4、revokeRolesFromRole指定roles为非法参数：a、非array类型，如：1、true、"strine" b、指定空的array
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( roleName2, true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( roleName2, 1 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( roleName2, "strine" );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromRole( roleName2 );
      } );

   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
   }
}
