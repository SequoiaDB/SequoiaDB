/************************************************************************
*@Description:   seqDB-6388:指定autodate参数导入date类型数据
                    int 	long 	string 	date 	timestamp
*@Author:   2016-7-29  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6388";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/allDataType.csv";
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //cat import file
   //var fileInfo = cmd.run( "cat "+ imprtFile );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string,v1 autodate,v2 autodate"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 17";
   var expParseFailure = "Parsed failure: 19";
   var expImportedRecords = "Imported records: 17";
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
   var expFailedNum = 19;
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 9 } }, { v2: { $type: 1, $et: 9 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 16;
   var expRecs = '[{"num":1,"type":"int","v1":{"$date":"1969-12-07"},"v2":{"$date":"1970-01-26"}},{"num":2,"type":"long","v1":{"$date":-9223372036854776000},"v2":{"$date":9223372036854776000}},{"num":5,"type":"number","v1":{"$date":"1969-12-07"},"v2":{"$date":"1970-01-26"}},{"num":10,"type":"date","v1":{"$date":"1900-01-01"},"v2":{"$date":"9999-12-31"}},{"num":11,"type":"timestamp","v1":{"$date":"1902-01-01"},"v2":{"$date":"2037-12-31"}},{"num":21,"type":"dateMS","v1":{"$date":"1958-01-12"},"v2":{"$date":"1978-01-12"}},{"num":22,"type":"timestampMS","v1":{"$date":"1901-12-15"},"v2":{"$date":"2038-01-18"}},{"num":31,"type":"intStr","v1":{"$date":"1969-12-07"},"v2":{"$date":"1970-01-26"}},{"num":32,"type":"longStr","v1":{"$date":-9223372036854776000},"v2":{"$date":9223372036854776000}},{"num":35,"type":"numberStr","v1":{"$date":"1969-12-07"},"v2":{"$date":"1970-01-26"}},{"num":39,"type":"dateStr","v1":{"$date":"1900-01-01"},"v2":{"$date":"9999-12-31"}},{"num":40,"type":"timestampStr","v1":{"$date":"1902-01-01"},"v2":{"$date":"2037-12-31"}},{"num":50,"type":"int","v1":{"$date":"1969-12-07"},"v2":{"$date":"1970-01-26"}},{"num":55,"type":"timestamp","v1":{"$date":"1901-12-31"},"v2":{"$date":"2038-01-01"}},{"num":57,"type":"dateMSStr","v1":{"$date":"1958-01-12"},"v2":{"$date":"1978-01-12"}},{"num":58,"type":"timestampMSStr","v1":{"$date":"1901-12-15"},"v2":{"$date":"2038-01-18"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );

   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}