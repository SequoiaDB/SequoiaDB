/******************************************************************************
*@Description : the collection do autosplit in domain.test input normal record
*               and lob data into collection
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var putNum = 50;

   var partitionNum = 2048;
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var DOMCSNAME = CHANGEDPREFIX + "_domainCS";
   var domName = CHANGEDPREFIX + "_domName";
   var cmd = new Cmd();
   var clName = COMMCLNAME + "_13939";
   commDropCL( db, COMMCSNAME, clName, true, true );
   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile );
   // create domain
   try
   {
      var names = lobGetAllGroupNames( db );
      commDropDomain( db, domName );
      var domain = commCreateDomain( db, domName, names, { "AutoSplit": true } );
      var cs = lobCreateCS( db, DOMCSNAME, domName );

      // create collection
      var optionObj = {
         "ShardingKey": { "no": 1 }, "ShardingType": "hash", "ReplSize": 0,
         "Partition": partitionNum, "Compressed": true
      };
      var cl = commCreateCL( db, DOMCSNAME, clName, optionObj, true, true );
      lobInsertDoc( cl, putNum );
      var oids = lobPutLob( cl, testFile, putNum );
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true );
         var curMd5 = getMd5ForFile( testFile );
         assert.equal( originMd5, curMd5 );
      }
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( { "no": i } ).count();
         assert.equal( 1, count );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      commDropCL( db, COMMCSNAME, clName, true, true );
      commDropDomain( db, domName );

      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}

