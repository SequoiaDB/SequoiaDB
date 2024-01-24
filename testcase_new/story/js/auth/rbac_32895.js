/******************************************************************************
 * @Description   : seqDB-32895:revokeRolesFromUser接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.09.01
 * @LastEditTime  : 2023.09.01
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32895";
   var roleName1 = "role_32895_1";
   var roleName2 = "role_32895_2";
   var userName = "user_32895";
   var passwd = "passwd_32895";

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

   db.createUsr( userName, passwd, { Roles: [roleName1, roleName2] } );


   try
   {
      //1、revokeRolesFromUser指定rolename为合法参数：string类型
      db.revokeRolesFromUser( userName, [roleName1] );
      db.revokeRolesFromUser( userName, [] );

      //2、revokeRolesFromUser指定rolename为非法参数：a、非string类型，如：true、array、1、""
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( true, [roleName1] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( 1, [roleName1] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( [], [roleName1] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( "", [roleName1] );
      } );

      //4、revokeRolesFromUser指定roles为非法参数：a、非array类型，如：1、true、"strine"
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( userName, true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( userName, 1 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( userName, "" );
      } );

      //4、revokeRolesFromUser指定roles为非法参数：不指定

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.revokeRolesFromUser( userName );
      } );

   } finally
   {
      cleanRole( roleName1 );
      cleanRole( roleName2 );
      db.dropUsr( userName, passwd );
   }
}
