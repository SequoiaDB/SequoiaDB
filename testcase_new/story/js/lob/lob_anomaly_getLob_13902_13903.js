/******************************************************************************
*@Description : anomaly test for getLob
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 1;
   var clName = COMMCLNAME + "_13902_13903";
   commDropCL( db, COMMCSNAME, clName, true, true );
   lobGenerateFile( testFile );
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
   // put Lob
   var oid = lobPutLob( cl, testFile, putNum ); //Array
   // get lob with no parmameter : getLob()[Test_Point_1]
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      cl.getLob();
   } );

   // get lob specify same file and don't set forced : true[Test_Point_2]
   cl.getLob( oid[0], getTestFile );
   assert.tryThrow( SDB_FE, function()
   {
      cl.getLob( oid[0], getTestFile );
   } );

   // delete lob
   try
   {
      // delete lobs
      for( var i = 0; i < oid.length; ++i )
      {
         cl.deleteLob( oid[i] );
      }
      commDropCL( db, COMMCSNAME, clName, true, true );
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      var cmd = new Cmd();
      // remove lobfile
      cmd.run( "rm -rf " + testFile );
      cmd.run( "rm -rf " + getTestFile );
   }
}
