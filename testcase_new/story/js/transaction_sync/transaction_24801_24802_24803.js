/******************************************************************************
 * @Description   : seqDB-24801:验证锁升级节点配置项 Transallowlockescalation
 *                : seqDB-24802:验证锁升级节点配置项 Transmaxlocknum
 *                : seqDB-24803:验证锁升级节点配置项 Transmaxlogspaceratio
 * @Author        : liuli
 * @CreateTime    : 2021.12.15
 * @LastEditTime  : 2022.08.30
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   try
   {
      // transallowlockescalation参数校验
      var config = { transallowlockescalation: false };
      db.updateConf( config );
      var actConfig = { transallowlockescalation: "FALSE" };
      checkSnapshot( db, actConfig );

      var config = { transallowlockescalation: true };
      db.updateConf( config );
      var actConfig = { transallowlockescalation: "TRUE" };
      checkSnapshot( db, actConfig );

      var config = { transallowlockescalation: 1 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var actConfig = { transallowlockescalation: "FALSE" };
      var config = { transallowlockescalation: 0 };
      db.updateConf( config );
      checkSnapshot( db, actConfig );

      var config = { transallowlockescalation: "true" };
      db.updateConf( config );
      var actConfig = { transallowlockescalation: "TRUE" };
      checkSnapshot( db, actConfig );

      // transmaxlocknum参数校验
      var config = { transmaxlocknum: -1 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlocknum: 0 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlocknum: 100 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlocknum: Math.pow( 2, 31 ) - 1 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlocknum: Math.pow( 2, 31 ) };
      db.updateConf( config );
      if( commIsArmArchitecture() )
      {
         var actConfig = { transmaxlocknum: Math.pow( 2, 31 ) - 1 };
      }
      else
      {
         var actConfig = { transmaxlocknum: -1 };
      }
      checkSnapshot( db, actConfig );

      var config = { transmaxlocknum: -2 };
      db.updateConf( config );
      var actConfig = { transmaxlocknum: -1 };
      checkSnapshot( db, actConfig );

      var config = { transmaxlocknum: true };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, actConfig );

      var config = { transmaxlocknum: "100" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, actConfig );

      var config = { transmaxlocknum: 100.1 };
      db.updateConf( config );
      var actConfig = { transmaxlocknum: 100 };
      checkSnapshot( db, actConfig );

      // transmaxlogspaceratio参数校验
      var config = { transmaxlogspaceratio: 1 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlogspaceratio: 10 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlogspaceratio: 50 };
      db.updateConf( config );
      checkSnapshot( db, config );

      var config = { transmaxlogspaceratio: 0 };
      db.updateConf( config );
      var actConfig = { transmaxlogspaceratio: 1 };
      checkSnapshot( db, actConfig );

      var config = { transmaxlogspaceratio: 51 };
      db.updateConf( config );
      var actConfig = { transmaxlogspaceratio: 50 };
      checkSnapshot( db, actConfig );

      var config = { transmaxlogspaceratio: 10.1 };
      db.updateConf( config );
      var actConfig = { transmaxlogspaceratio: 10 };
      checkSnapshot( db, actConfig );

      var config = { transmaxlogspaceratio: true };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, actConfig );

      var config = { transmaxlogspaceratio: "10" };
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( config );
      } );
      checkSnapshot( db, actConfig );
   } finally
   {
      db.deleteConf( { transallowlockescalation: "", transmaxlocknum: "", transmaxlogspaceratio: "" } );
   }
}

function checkSnapshot ( sdb, option )
{
   var cursor = sdb.snapshot( SDB_SNAP_CONFIGS, { role: "data" } );
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