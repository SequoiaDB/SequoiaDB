/************************************************************************
*@Description:   seqDB-5435:指定errorstop为true，即导入失败时中断导入（单线程解析和导入）
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5435";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5435.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "at int,bt date\n1,2016-1-1\n2,2016-1-2\n3,2016-0-0\n4,2016-1-3" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --errorstop true -n 1 --parsers 1 -j 1'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 2";
   var expParseFailure = "Parsed failure: 1";
   var expImportedRecords = "Imported records: 2";
   var actParseRecords = rcObj[0];
   var actParseFailure = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }

   var rec = cmd.run( "ls " + tmpRec ).split( "\n" )[0];
   var failedRecs = cmd.run( "cat " + rec ).split( "\n" )[0];
   var expRecRecs = '3,2016-0-0';
   var actRecRecs = failedRecs;
   if( expRecRecs !== actRecRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[failedRecs:" + expRecRecs + "]" +
         "[failedRecs:" + actRecRecs + "]" );
   }

   cmd.run( "rm -rf " + tmpRec );

}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "at": 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 2;
   var expRecs = '[{"at":1,"bt":{"$date":"2016-01-01"}},{"at":2,"bt":{"$date":"2016-01-02"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}