/******************************************************************************
*@Description : abnormal parameter verification for getLob().
*@Modify list :
*               2019-05-29  wuyan  Init
******************************************************************************/

main( test );

function test ()
{
   try
   {
      var clName = "testLob4468";
      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the beginning" );

      var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, false, "create collection" );
      var lobFilePath = WORKDIR + "/testlob4468";
      lobGenerateFile( lobFilePath );
      var lobOid = cl.putLob( lobFilePath );

      //test case:4468
      getLobWithOidNotExist( cl );
      //test case:4475
      getLobWithIllegalForced( cl, lobOid );
      //test case:4476
      getLobWithEmptyForced( cl, lobOid )

      commDropCL( db, COMMCSNAME, clName, true, true, "clear collection in the ending" );
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
      cmd.run( "rm -rf " + lobFilePath );
   }


}

function getLobWithOidNotExist ( cl )
{
   var getFilePath = WORKDIR + "/getlob4468";
   var lobOid = "5ce6016a97216ce21b5c982a";
   assert.tryThrow( SDB_FNE, function()
   {
      cl.getLob( lobOid, getFilePath );
   } );
}

function getLobWithIllegalForced ( cl, lobOid )
{
   var getFilePath = WORKDIR + "/getlob4475";
   var illegalForced = "test";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.getLob( lobOid, getFilePath, illegalForced );
   } );
}

function getLobWithEmptyForced ( cl, lobOid )
{
   var getFilePath = WORKDIR + "/getlob4476";
   var forced = null;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      cl.getLob( lobOid, getFilePath, null );
   } );
}

