/* *****************************************************************************
@discretion: disable compression, then alter compressionType
@author��2018-4-26 wuyan  Init
***************************************************************************** */
var clName = CHANGEDPREFIX + "_alterclcompression_14974";

main( test );
function test ()
{
   if( true == commIsStandalone( db ) )
   {
      return;
   }
   //clean environment before test
   commDropCL( db, COMMCSNAME, clName, true, true, "drop CL in the beginning" );

   //create cl
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //enable compression
   var compressionType = "lzw";
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      dbcl.setAttributes( { CompressionType: compressionType } );
   });
   //checkAlterResult( clName, "AttributeDesc", "Compressed" );
   //checkAlterResult( clName, "CompressionTypeDesc", compressionType );

   //clean
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );
}
