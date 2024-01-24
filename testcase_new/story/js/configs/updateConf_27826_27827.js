/******************************************************************************
 * @Description   : seqDB-27826:metacacheexpired配置参数验证 
 *                : seqDB-27827:metacachelwm配置参数验证 
 * @Author        : liuli
 * @CreateTime    : 2022.09.23
 * @LastEditTime  : 2022.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      // metacacheexpired校验默认配置
      var expConfig = { metacacheexpired: 30 };
      checkSnapshot( db, expConfig );

      // metacacheexpired校验合法参数
      var config = { metacacheexpired: 0 };
      var expConfig = { metacacheexpired: 0 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacacheexpired: 43200 };
      var expConfig = { metacacheexpired: 43200 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacacheexpired: 1 };
      var expConfig = { metacacheexpired: 1 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacacheexpired: 200.715 };
      var expConfig = { metacacheexpired: 200 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      // 超范围值自动修正
      var config = { metacacheexpired: 50000 };
      var expConfig = { metacacheexpired: 43200 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      // metacacheexpired校验非法参数
      var config = { metacacheexpired: "TRUE" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );

      var config = { metacacheexpired: "" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );

      var config = { metacacheexpired: -1 };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );

      // metacachelwm校验默认配置
      var expConfig = { metacachelwm: 512 };
      checkSnapshot( db, expConfig );

      // metacachelwm校验合法参数
      var config = { metacachelwm: 0 };
      var expConfig = { metacachelwm: 0 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacachelwm: 10240 };
      var expConfig = { metacachelwm: 10240 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacachelwm: 1 };
      var expConfig = { metacachelwm: 1 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      var config = { metacachelwm: 200.715 };
      var expConfig = { metacachelwm: 200 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      // 超范围值自动修正
      var config = { metacachelwm: 20480 };
      var expConfig = { metacachelwm: 10240 };
      db.updateConf( config );
      checkSnapshot( db, expConfig );

      //metacachelwm校验非法参数
      var config = { metacachelwm: "TRUE" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );

      var config = { metacachelwm: "" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );

      var config = { metacachelwm: -1 };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, expConfig );
   }
   finally
   {
      db.deleteConf( { metacacheexpired: "", metacachelwm: "" } );
   }
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, {}, option );
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      for( var key in option )
      {
         assert.equal( obj[key], option[key] );
      }
   }
   cursor.close();
}