/******************************************************************************
*@Description : abnormal parameter verification for putLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/

main( test );

function test ()
{
   var clName = "testLob4462";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collection" );

   //test case:4462
   putLobWithFileNotExist( cl );
   //test case:4464
   putLobWithEmpty( cl );

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );

}

function putLobWithFileNotExist ( cl )
{
   var testLobFile = "/test4462.txt";
   assert.tryThrow( SDB_FNE, function()
   {
      cl.putLob( testLobFile );
   } );
}

function putLobWithEmpty ( cl )
{
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.putLob();
   } );
}

