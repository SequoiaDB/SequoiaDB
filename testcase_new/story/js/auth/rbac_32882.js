/******************************************************************************
 * @Description   : seqDB-32882 :: invalidateUserCache接口参数校验 
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.04
 * @LastEditTime  : 2023.09.04
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var name = "user_32882";
   var password = "password_32882";


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
      db.createUsr( name, password, { Roles: ['_root'] } );
      var rootDB = new Sdb( COORDHOSTNAME, COORDSVCNAME, name, password );

      // username指定合法参数类型
      rootDB.invalidateUserCache();
      rootDB.invalidateUserCache( "" );
      rootDB.invalidateUserCache( "notExistUser" );

      // username指定非法参数类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rootDB.invalidateUserCache( true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rootDB.invalidateUserCache( ['array'] );
      } );


      // options指定合法参数类型
      rootDB.invalidateUserCache( name, { Group: commGetDataGroupNames( db )[0] } );
      rootDB.invalidateUserCache( "", { Group: commGetDataGroupNames( db )[0] } );
      rootDB.invalidateUserCache( name, { Group: "" } );
      rootDB.invalidateUserCache( "", { Group: "" } );

      // options为非法参数类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rootDB.invalidateUserCache( name, 123 );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rootDB.invalidateUserCache( "", true );
      } );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rootDB.invalidateUserCache( name, "string" );
      } );

   } finally
   {
      try
      {
         rootDB.dropUsr( name, password );
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
