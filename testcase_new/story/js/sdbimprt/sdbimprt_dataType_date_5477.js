/************************************************************************
*@Description:    seqDB-5477:导入数据时指定的数据类型为date，实际数据为不支持转换的数据类型
                  seqDB-5476
*@Author:   2016-7-29  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5477";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5477.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,str,"1901-01-01-00.00.00.000000"' + "\n"
      + '2,timestamp,9999-12-31-00.00.00.000000' + "\n"
      + '3,invalid,2014-01-01-10.30' + "\n"
      + '4,invalid,2014-01' + "\n"
      + '5,invalid,""' + "\n"
      + '6,null,,' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string,v1 date"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 5";
   var expParseFailure = "Parsed failure: 1";
   var expImportedRecords = "Imported records: 5";
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
   var actFailedNum = cmd.run( "cat -v " + rec ).split( "\n" ).length - 1;
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

   var rc = cl.find( { v1: { $type: 1, $et: 9 } }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 4;  //skip a records: {"num":6,"type":"null","v1":null,"v2":null}
   var expRecs = '[{"num":1,"type":"str","v1":{"$date":"1901-01-01"}},{"num":2,"type":"timestamp","v1":{"$date":"9999-12-31"}},{"num":3,"type":"invalid","v1":{"$date":"2014-01-01"}},{"num":4,"type":"invalid","v1":{"$date":"2014-01-01"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}