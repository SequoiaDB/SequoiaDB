/******************************************************************************
 * @Description   : seqDB-32890 :: grantRolesToRole接口参数校验
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32890";
   var roleName1 = "role_32890_1";
   var roleName2 = "role_32890_2";

   cleanRole( roleName1 );
   cleanRole( roleName2 );

   var role1 = {
      Role: roleName1, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find", "remove"]
      }]
   };
   db.createRole( role1 );
   var role2 = {
      Role: roleName2, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["update", "insert"]
      }]
   };
   db.createRole( role2 );

   // grantRolesToRole指定rolename为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( 123, [roleName2] );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( true, [roleName2] );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( ['array'], [roleName2] );
   } );

   // grantRolesToRole指定roles为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( roleName1, "string" );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( roleName1, 123 );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( roleName1, { other: "string" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantRolesToRole( roleName1 );
   } );

   // grantRolesToRole指定rolename为合法参数
   db.grantRolesToRole( roleName1, [roleName2] );
   db.grantRolesToRole( roleName1, [] );

   cleanRole( roleName1 );
   cleanRole( roleName2 );

}
