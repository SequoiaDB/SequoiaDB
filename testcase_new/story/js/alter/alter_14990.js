/************************************
*@Description: enableSharding批量修改属性
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14990
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14990";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 }, Compressed: true };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   cl.enableSharding( { ShardingKey: { b: 1 }, ShardingType: "range", EnsureShardingIndex: true } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { b: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "range" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );

   cl.enableSharding( { Partition: 2048, ShardingType: "hash", ShardingKey: { a: 1 }, EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 2048 );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", false );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.enableSharding( { ShardingKey: { b: 1 }, ShardingType: "range", Compressed: false } );
   } );

   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", "hash" );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "Compressed" );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}