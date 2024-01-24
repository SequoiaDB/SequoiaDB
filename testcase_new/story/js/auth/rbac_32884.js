/******************************************************************************
 * @Description   : seqDB-32884 :: dropRole接口参数校验 
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.04
 * @LastEditTime  : 2023.09.04
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32884";
   var roleName = "role_32884";

   cleanRole( roleName );

   try
   {
      var role = {
         Role: roleName, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "remove"]
         }]
      };
      db.createRole( role );

      // dropRole指定role为合法参数
      assert.tryThrow( SDB_AUTH_ROLE_NOT_EXIST, function()
      {
         db.dropRole( "abc" )
      } );

      assert.tryThrow( SDB_AUTH_ROLE_NOT_EXIST, function()
      {
         db.dropRole( "@#$%" )
      } );

      // dropRole指定role为非法参数
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.dropRole( true )
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.dropRole( 123 )
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.dropRole( ['array'] )
      } );

   } finally
   {
      cleanRole( roleName );
   }
}
