/************************************
*@Description: alter修改分区属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-15032
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_15032";

   var cl = commCreateCL( db, csName, clName, {}, true, false, "create CL in the begin" );
   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   cl.alter( { ShardingKey: { a: 1 }, ShardingType: "hash", Partition: 1024, EnsureShardingIndex: true, AutoSplit: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 1024 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false );

   cl.alter( { ShardingKey: { a: 1, b: 1 }, ShardingType: "range", EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1, b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", false );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.alter( { ShardingKey: { b: 1 }, ShardingType: "hash", IsMainCL: true, "AutoSplit": true, Compressed: false, lobPageSize: 8192 } );
   } );

   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1, b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "IsMainCL", undefined );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AutoSplit", false );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}