/************************************
*@Description: 切分表修改ShardingKey字段, ShardingType, Partition, EnsureShardingIndex
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14940, seqDB-14950, seqDB-14957, seqDB-14978
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   //less two groups no split
   var allGroupName = getGroupName( db, true );
   if( 1 === allGroupName.length )
   {
      return;
   }
   var csName = COMMCSNAME;
   var clName = CHANGEDPREFIX + "_14940";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 } };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   for( i = 0; i < 5000; i++ )
   {
      cl.insert( { a: i, b: "sequoiadh test split cl alter option" } );
   }

   var splitGroup = getSplitGroup( db, csName, clName );
   cl.split( splitGroup.srcGroup, splitGroup.tarGroup, 50 );

   //test split cl alter ShardingKey
   clSetAttributes( cl, { ShardingKey: { 'b': 1 } } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingKey", { a: 1 } );

   clSetAttributes( cl, { ShardingType: 'range' } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "ShardingType", 'hash' );

   clSetAttributes( cl, { Partition: 8192 } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Partition", 4096 );

   clSetAttributes( cl, { EnsureShardingIndex: false } );
   checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "EnsureShardingIndex", true );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}