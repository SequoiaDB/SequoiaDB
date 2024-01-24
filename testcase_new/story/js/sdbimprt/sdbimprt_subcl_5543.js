/************************************************************************
*@Description:   seqDB-5543:导入的数据不在子表的区间范围内
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var csName = COMMCSNAME;
   var mainclName = COMMCLNAME + "_5543_mainCL";
   var subclName = COMMCLNAME + "_5543_subCL";
   //create mainCL
   var opt1 = { ShardingKey: { a: 1 }, IsMainCL: true };
   var mainCL = readyCL( csName, mainclName, opt1, "[mainCL]" );
   //create subCL
   var opt2 = { ShardingKey: { a: 1 }, ShardingType: "hash", ReplSize: 0 };
   readyCL( csName, subclName, opt2, "[subCL]" );
   //attach cl
   var options = { LowBound: { "a": 1 }, UpBound: { "a": 10 } };
   mainCL.attachCL( csName + "." + subclName, options );

   var imprtFile = tmpFileDir + "5543.csv";
   readyData( imprtFile );
   importData( csName, mainclName, imprtFile );

   checkCLData( mainCL );
   cleanCL( csName, subclName );
   cleanCL( csName, mainclName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "a int\n1\n6\n9\n0\n10" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --sharding true -n 1 --parsers 1 -j 1'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 5";
   var expImportedRecords = "Imported records: 3";
   var expImportFailure = "Imported failure: 2";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   var actImportFailure = rcObj[5];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords
      || expImportFailure !== actImportFailure )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + ", " + expImportFailure + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + ", " + actImportFailure + "]" );
   }

   var rec = cmd.run( "ls " + tmpRec ).split( "\n" )[0];
   var tmpRecs = cmd.run( "cut -c 49-56 " + rec ).split( "\n" );
   var failedRecs = String( [tmpRecs[0], tmpRecs[1]] );
   var expRecRecs = ' "a": 0 , "a": 10';
   var actRecRecs = failedRecs;
   if( expRecRecs !== actRecRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[failedRecs:" + expRecRecs + "]" +
         "[failedRecs:" + actRecRecs + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );

}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 3;
   var expRecs = '[{"a":1},{"a":6},{"a":9}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}
