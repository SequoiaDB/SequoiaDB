/******************************************************************************
 * @Description   : seqDB-32888 :: grantPrivilegesToRole接口参数校验
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32888";
   var roleName = "role_32888";

   cleanRole( roleName );

   var role = {
      Role: roleName, Privileges: [{
         Resource: { cs: dbname, cl: "" },
         Actions: ["find", "remove"]
      }]
   };
   db.createRole( role );

   // grantPrivilegesToRole指定rolename和privileges为合法参数
   var privileges = [{ Resource: { cs: dbname, cl: "" }, Actions: ["update"] }]
   db.grantPrivilegesToRole( roleName, privileges );

   // grantPrivilegesToRole指定rolename为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( 123, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( false, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( ['array'], privileges );
   } );

   // grantPrivilegesToRole指定privileges为非法参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var privileges = [{ Actions: ["update"] }]
      db.grantPrivilegesToRole( roleName, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var privileges = [{ Resource: { cs: dbname, cl: "" } }]
      db.grantPrivilegesToRole( roleName, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var privileges = [{ Resource: { cs: 123, cl: 123 }, Actions: ["update"] }]
      db.grantPrivilegesToRole( roleName, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var privileges = [{ Resource: { cs: dbname, cl: "" }, Actions: true }]
      db.grantPrivilegesToRole( roleName, privileges );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( roleName, 123 );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( roleName, true );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( roleName, "string" );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.grantPrivilegesToRole( roleName, ['array'] );
   } );

   cleanRole( roleName );
}
