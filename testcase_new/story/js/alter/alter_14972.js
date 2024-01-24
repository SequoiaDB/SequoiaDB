/************************************
*@Description: 修改压缩类型lzw为snappy
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14972
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
   var clName = CHANGEDPREFIX + "_14972";

   var options = { ShardingType: 'hash', ShardingKey: { a: 1 }, Compressed: true, CompressionType: 'lzw' };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl.setAttributes( { CompressionType: 'snappy' } );
   } );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionType", 0 );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionTypeDesc", "snappy" );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}
