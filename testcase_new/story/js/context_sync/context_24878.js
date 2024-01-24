/******************************************************************************
 * @Description   : seqDB-24878:contexttimeout配置参数校验 
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.28
 * @LastEditTime  : 2022.03.15
 * @LastEditors   : Zhang Yanan
 ******************************************************************************/
main( test );
function test ()
{
   try
   {
      var snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 1440 );

      db.updateConf( { "contexttimeout": 0 } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 0 );

      db.updateConf( { "contexttimeout": Math.pow( 2, 31 ) - 1 } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, Math.pow( 2, 31 ) - 1 );

      db.updateConf( { "contexttimeout": 2000 } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 2000 );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( { "contexttimeout": true } );
      } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 2000 );

      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( { "contexttimeout": "100" } );
      } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 2000 );

      db.updateConf( { "contexttimeout": 0.5 } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 0 );

      db.updateConf( { "contexttimeout": -1 } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      checkSnashopt( snapshots, 0 );

      db.updateConf( { "contexttimeout": Math.pow( 2, 31 ) } );
      snapshots = db.snapshot( SDB_SNAP_CONFIGS );
      if( commIsArmArchitecture() == true )
      {
         checkSnashopt( snapshots, Math.pow( 2, 31 ) - 1 );
      } else
      {
         checkSnashopt( snapshots, 0 );
      }
   }
   finally
   {
      //恢复默认配置
      db.deleteConf( { "contexttimeout": 0 } );
   }
}

function checkSnashopt ( snapshots, timeout )
{
   snapshotsArray = snapshots.toArray();
   for( i = 0; i < snapshotsArray.length; i++ )
   {
      var snapshot = JSON.parse( snapshotsArray[i] );
      var snapshotTimeout = snapshot.contexttimeout;
      assert.equal( snapshotTimeout, timeout );
   }
}