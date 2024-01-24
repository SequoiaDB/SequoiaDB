/******************************************************************************
*@Description : test function:
*               db.collectionspace.collection.getLob( < oid >, < file >, [forced] )
*               db.collectionspace.collection.putLob( < lobfile > )
*               db.collectionspace.collection.listLobs()
*               db.collectionspace.collection.deleteLob( < oid > )
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/

main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "_lobTest.file";
   var getTestFile = CHANGEDPREFIX + "_lobTestGet.file";
   var putNum = 10;
   var cmd = new Cmd();
   var oids = [];

   lobGenerateFile( testFile ); // auto file
   // cmd.run( "cat " + testFile ); 
   // create collection
   var clName = COMMCLNAME + "_13938";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cl = commCreateCL( db, COMMCSNAME, clName, {}, true, true );
   var md5Arr = cmd.run( "md5sum " + testFile ).split( " " );
   var md5 = md5Arr[0];
   // put Lob
   for( var i = 0; i < putNum; ++i )
   {
      oids.push( cl.putLob( testFile ) );
   }
   // verify
   var cursor = cl.listLobs().toArray();
   assert.equal( putNum, cursor.length );
   // get lob
   try
   {
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true );
         md5Arr = cmd.run( "md5sum " + getTestFile ).split( " " );
         getMd5 = md5Arr[0];
         assert.equal( getMd5, md5 );
      }
      // delete lobs
      for( var i = 0; i < oids.length; ++i )
      {
         cl.deleteLob( oids[i] );
      }
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ); 
      //cmd.run( "rm -rf " + getTestFile ); 
   }
   catch( e )
   {
      // remove lobfile
      //cmd.run( "rm -rf " + testFile ); 
      throw e;
   }
   finally
   {
      cmd.run( "rm -rf " + testFile );
      cmd.run( "rm -rf " + getTestFile );
      commDropCL( db, COMMCSNAME, clName, true, true );
   }
}
