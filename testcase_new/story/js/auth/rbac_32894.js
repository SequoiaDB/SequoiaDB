/******************************************************************************
 * @Description   : seqDB-32894 :: grantRolesToUser接口参数校验
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dbname = "db_32894";
   var roleName = "role_32894";
   var name = "user_32894";
   var password = "password_32894";

   cleanRole( roleName );

   try
   {
      db.dropUsr( name, password );
   }
   catch( e )
   {
      if( e != SDB_AUTH_USER_NOT_EXIST )
      {
         throw e;
      }
   }

   try
   {
      var role = {
         Role: roleName, Privileges: [{
            Resource: { cs: dbname, cl: "" },
            Actions: ["find", "remove"]
         }]
      };
      db.createRole( role );
      db.createUsr( name, password );

      // grantRolesToUser指定username为非法参数
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( 123, [roleName] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( true, [roleName] );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( ['array'], [roleName] );
      } );

      // grantRolesToUser指定roles为非法参数
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( name, "string" );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( name, true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( name, 123 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.grantRolesToUser( name, { other: 1 } );
      } );

      // grantRolesToUser指定username和roles为合法参数
      db.grantRolesToUser( name, [roleName] );

   } finally
   {
      cleanRole( roleName );

      try
      {
         db.dropUsr( name, password );
      }
      catch( e )
      {
         if( e != SDB_AUTH_USER_NOT_EXIST )
         {
            throw e;
         }
      }
   }
}
