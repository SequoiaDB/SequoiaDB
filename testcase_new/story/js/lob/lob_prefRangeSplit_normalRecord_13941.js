/******************************************************************************
*@Description : the collection do range split.test input normal record and
*               lob data into collection
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

   var names = lobGetAllGroupNames( db );
   if( 1 == names.length )
   {
      return;
   }

   lobGenerateFile( testFile ); // auto file
   // create collection
   var optionObj = {
      "ShardingKey": { "no": 1 }, "ShardingType": "range", "ReplSize": 0,
      "Compressed": true
   };
   var clName = COMMCLNAME + "_13941";
   commDropCL( db, COMMCSNAME, clName, true, true );
   var cl = commCreateCL( db, COMMCSNAME, clName, optionObj, true, true );
   // do range split collection before put data
   var FULLCLNAME = COMMCSNAME + "." + clName;
   var clRg = commGetCLGroups( db, FULLCLNAME );
   var cond = Math.floor( putNum / names.length );
   var loopCond = cond;
   for( var i = 0; i < names.length; ++i )
   {
      if( clRg[0] != names[i] )
      {
         var firstCond = { "no": ( loopCond - cond ) };
         var secondCond = { "no": loopCond };
         lobSplit( cl, clRg[0], names[i], firstCond, secondCond );
         loopCond += cond;
      }
   }
   lobInsertDoc( cl, putNum ); // will be OK
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var oids = lobPutLob( cl, testFile, putNum ); // will throw exception
   } )

   // get lob
   try
   {
      for( var i = 0; i < cl.count(); ++i )
      {
         var count = cl.find( { "no": i } ).count();
         assert.equal( 1, count );
      }

      if( typeof ( oids ) == "undefined" ) return;
      for( var i = 0; i < oids.length; ++i )// oid equal 0
      {
         cl.getLob( oids[i], getTestFile, true );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      // remove lobfile
      cmd = new Cmd();
      cmd.run( "rm -rf " + testFile );
      if( lobFileIsExist( getTestFile ) )
      {
         cmd.run( "rm -rf " + getTestFile );
      }
      commDropCL( db, COMMCSNAME, clName, true, true );
   }
}
