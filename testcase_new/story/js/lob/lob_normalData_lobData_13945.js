/******************************************************************************
*@Description : test same collection input nomal record and lob data
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 50;

   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile );
   var clName = COMMCLNAME + "_13945";
   commDropCL( db, COMMCSNAME, clName, true, true );
   // create collection
   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
   // put normal data and put lob
   try
   {
      oid = lobPutLob( cl, testFile, putNum );
      lobInsertDoc( cl, putNum );

      for( var i = 0; i < oid.length; ++i )
      {
         cl.getLob( oid[i], getTestFile, true );
         var curMd5 = getMd5ForFile( getTestFile );
         assert.equal( originMd5, curMd5 );
      }
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( { "no": i } ).count();
         assert.equal( 1, count );
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
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}
