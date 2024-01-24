/************************************
*@Description: 重复开启关闭压缩
*@author:      luweikang
*@createdate:  2018.4.25
*@testlinkCase:seqDB-14967
**************************************/

main( test );
function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }
   var csName = COMMCSNAME;
   var clName = "cl14967";

   var options = { Compressed: false };
   var cl = commCreateCL( db, csName, clName, options, true, false, "create CL in the begin" );

   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl.setAttributes( { CompressionType: 'snappy' } )
   } );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionType", 0 );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionTypeDesc", "snappy" );

   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl.setAttributes( { Compressed: false } )
   } );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "Attribute", 0 );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "AttributeDesc", "" );

   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl.setAttributes( { Compressed: true } )
   } );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionType", 1 );
   //checkSnapshot( db, SDB_SNAP_CATALOG, csName, clName, "CompressionTypeDesc", "lzw" );

   commDropCL( db, csName, clName, true, false, "clean cl" );
}
