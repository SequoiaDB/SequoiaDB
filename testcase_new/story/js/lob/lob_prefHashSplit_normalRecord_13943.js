/******************************************************************************
*@Description : the collection do hash split befor input data.test input
*               normal record and lob data into collection
*@Modify list :
*               2014-12-18  xiaojun Hu  Init
******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var testFile = CHANGEDPREFIX + "lobTest.file";
   var getTestFile = CHANGEDPREFIX + "lobTestGet.file";
   var putNum = 50;
   var partitionNum = 2048;
   var names = lobGetAllGroupNames( db );
   if( 1 == names.length )
   {
      return;
   }

   lobGenerateFile( testFile ); // auto file
   var originMd5 = getMd5ForFile( testFile );

   // create collection
   var optionObj = {
      "ShardingKey": { "no": 1 }, "ShardingType": "hash", "ReplSize": 0,
      "Partition": partitionNum, "Compressed": true
   };
   var clName = COMMCLNAME + "_13943";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, optionObj, true, true );
   // collection do hash split before put data
   try
   {
      var FULLCLNAME = COMMCSNAME + "." + clName;
      var clRg = commGetCLGroups( db, FULLCLNAME );

      var cond = Math.floor( partitionNum / names.length );
      var loopCond = cond;
      for( var i = 0; i < names.length; ++i )
      {
         if( clRg[0] != names[i] )
         {
            var firstCond = { "Partition": loopCond - cond };
            var secondCond = { "Partition": loopCond };
            lobSplit( cl, clRg[0], names[i], firstCond, secondCond );
            loopCond += cond;
         }
      }


      lobInsertDoc( cl, putNum );
      var oids = lobPutLob( cl, testFile, putNum );
      for( var i = 0; i < oids.length; ++i )
      {
         cl.getLob( oids[i], getTestFile, true );
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
      // remove lobfile
      var cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
   }
}
