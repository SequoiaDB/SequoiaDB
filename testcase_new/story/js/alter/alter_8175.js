/******************************************************************************
@Description : 1. collection altered Compress, fail
@Modify list :
               2014-07-10 pusheng Ding  Init
               2019-10-21  luweikang modify
               2020-07-06 liyuanyue modify
******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var clName1 = "alter8175_1";
   var clName2 = "alter8175_2";
   var clName3 = "alter8175_3";
   var clName4 = "alter8175_4";
   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );

   var cl1 = commCreateCL( db, COMMCSNAME, clName1, { Compressed: false } );
   var cl2 = commCreateCL( db, COMMCSNAME, clName2, { ShardingKey: { id: 1 }, ShardingType: "hash", Compressed: false } );
   var cl3 = commCreateCL( db, COMMCSNAME, clName3, { ShardingKey: { id: 1 }, ShardingType: "range", Compressed: false } );
   var cl4 = commCreateCL( db, COMMCSNAME, clName4, { ShardingKey: { id: 1 }, ShardingType: "range", IsMainCL: true, Compressed: false } );

   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl1.alter( { "Compressed": true } );
   });
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl2.alter( { "Compressed": true } );
   });
   assert.tryThrow(SDB_ENGINE_NOT_SUPPORT, function() {
      cl3.alter( { "Compressed": true } );
   });
   cl4.alter( { "Compressed": true } );

   //check cl snapshot
   //checkcompressionType( clName1 );
   //checkcompressionType( clName2 );
   //checkcompressionType( clName3 );
   checkcompressionType( clName4 );

   commDropCL( db, COMMCSNAME, clName1 );
   commDropCL( db, COMMCSNAME, clName2 );
   commDropCL( db, COMMCSNAME, clName3 );
   commDropCL( db, COMMCSNAME, clName4 );
}

function checkcompressionType ( clName )
{
   var snap = db.snapshot( 8, { Name: COMMCSNAME + "." + clName } );
   var compressionType = snap.current().toObj()['CompressionType'];
   if( compressionType !== 0 )
   {
      throw new Error( "check compressionType, \nexpect: 0, \nbut found: " + compressionType );
   }
}
