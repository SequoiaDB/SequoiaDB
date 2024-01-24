/************************************************************************
*@Description:  seqDB-5468:导入数据时指定的数据类型为bool(boolean)，实际字段值为空
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5468";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5468.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,"string","1", "0"' + "\n"
      + '2,"string",""' + "\n"
      + '3,null,,' );
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
      + ' --type csv --fields "num int,type string,v1 bool,v2 bool"'
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

   //check failed records
   var rec = cmd.run( "ls " + tmpRec ).split( "\n" )[0];
   var actFailedNum = cmd.run( "cat " + rec ).split( "\n" ).length - 1;
   var expFailedNum = 1;
   if( expFailedNum !== actFailedNum )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[failedRecs:" + expFailedNum + "]" +
         "[failedRecs:" + actFailedNum + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 2;
   var expRecs = '[{"num":1,"type":"string","v1":true,"v2":false},{"num":3,"type":"null"}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}