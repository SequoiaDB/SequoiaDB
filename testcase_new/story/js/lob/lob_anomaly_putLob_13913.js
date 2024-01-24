/******************************************************************************
*@Description : anomaly test that for putLob/listLob/deleteLob
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   //lobGenerateFile( testFile ); // auto file
   // create collection
   var clName = COMMCLNAME + "_13913";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
   // put lob with no lob file[Test_Point_1]
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.listLobs( testFile );
   } );

   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.putLob();
   } );

   // delete lob with no oid[Test_Point_2]
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.deleteLob();
   } );
   commDropCL( db, COMMCSNAME, clName, true, true );

}
