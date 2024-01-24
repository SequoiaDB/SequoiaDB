/******************************************************************************
*@Description : abnormal parameter verification for deleteLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/

main( test );

function test ()
{
   var clName = "testLob4465";
   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collection" );

   //test case:4465
   deleteLobWithOidNotExist( cl );
   //test case:4467
   deleteLobWithEmpty( cl );

   commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );

}

function deleteLobWithOidNotExist ( cl )
{
   var lobOid = "5ce6016a97216ce21b5c982f";
   assert.tryThrow( SDB_FNE, function()
   {
      cl.deleteLob( lobOid );
   } );
}

function deleteLobWithEmpty ( cl )
{
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.deleteLob();
   } );
}

