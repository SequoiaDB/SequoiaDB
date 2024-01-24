/************************************************************************
*@Description:  seqDB-11185:导入date类型数据
*@Author:            2017-3-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_11185";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/date.json";
   var exprtFile = tmpFileDir + "sdbexprt_11185.csv";

   importData( csName, clName, imprtFile );
   checkCLData( cl );

   exprtData( csName, clName, exprtFile );
   checkExprtFile( csName, clName, exprtFile );

   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 53";
   var expParseFailure = "Parsed failure: 14";
   var expImportedRecords = "Imported records: 53";
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
   var expFailedNum = 29;  //include blank line
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

   var rc = cl.find( { $and: [{ b: { $type: 1, $et: 9 } }, { a: { $ne: 50 } }] }, { _id: { $include: 0 } } ).sort( { a: 1 } ); //except '{ a:50, b: SdbDate() }'
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 52;
   var expA15 = cmd.run( 'date -d@"-62135625957" +"%Y-%m-%d"' ).split( "\n" )[0];
   var expRecs = '[{"a":0,"b":{"$date":"1900-01-01"}},{"a":1,"b":{"$date":"1970-01-01"}},{"a":2,"b":{"$date":"9999-12-31"}},{"a":3,"b":{"$date":"0001-01-01"}},{"a":4,"b":{"$date":"1899-12-31"}},{"a":5,"b":{"$date":"1900-01-01"}},{"a":6,"b":{"$date":"9999-12-31"}},{"a":7,"b":{"$date":"0001-01-01"}},{"a":8,"b":{"$date":"1899-12-31"}},{"a":9,"b":{"$date":"1900-01-01"}},{"a":10,"b":{"$date":"9999-12-31"}},{"a":11,"b":{"$date":-9223372036854776000}},{"a":12,"b":{"$date":-9223372036854776000}},{"a":13,"b":{"$date":9223372036854776000}},{"a":14,"b":{"$date":9223372036854776000}},{"a":15,"b":{"$date":"' + expA15 + '"}},{"a":16,"b":{"$date":"1900-01-01"}},{"a":17,"b":{"$date":"9999-12-31"}},{"a":18,"b":{"$date":-9223372036854776000}},{"a":20,"b":{"$date":9223372036854776000}},{"a":22,"b":{"$date":"' + expA15 + '"}},{"a":23,"b":{"$date":"1900-01-01"}},{"a":24,"b":{"$date":"9999-12-31"}},{"a":25,"b":{"$date":-9223372036854776000}},{"a":27,"b":{"$date":9223372036854776000}},{"a":29,"b":{"$date":"' + expA15 + '"}},{"a":30,"b":{"$date":"1900-01-01"}},{"a":31,"b":{"$date":"9999-12-31"}},{"a":40,"b":{"$date":"1899-12-31"}},{"a":42,"b":{"$date":"0000-12-31"}},{"a":44,"b":{"$date":"0000-12-31"}},{"a":51,"b":{"$date":"1901-01-01"}},{"a":52,"b":{"$date":"9999-12-31"}},{"a":53,"b":{"$date":"0001-01-01"}},{"a":54,"b":{"$date":"1899-12-31"}},{"a":55,"b":{"$date":"1900-01-01"}},{"a":56,"b":{"$date":"9999-12-31"}},{"a":57,"b":{"$date":"0001-01-01"}},{"a":58,"b":{"$date":"1899-12-31"}},{"a":59,"b":{"$date":"1900-01-01"}},{"a":60,"b":{"$date":"9999-12-31"}},{"a":61,"b":{"$date":-9223372036854776000}},{"a":62,"b":{"$date":9223372036854776000}},{"a":63,"b":{"$date":"' + expA15 + '"}},{"a":64,"b":{"$date":"1900-01-01"}},{"a":65,"b":{"$date":"9999-12-31"}},{"a":66,"b":{"$date":-9223372036854776000}},{"a":68,"b":{"$date":9223372036854776000}},{"a":70,"b":{"$date":"' + expA15 + '"}},{"a":71,"b":{"$date":"1900-01-01"}},{"a":72,"b":{"$date":"9999-12-31"}},{"a":80,"b":{"$date":"1899-12-31"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]\n" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}

function exprtData ( csName, clName, exprtFile )
{

   //remove export file
   cmd.run( "rm -rf " + exprtFile );

   //export operation
   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json --fields "a,b"'
      + ' --sort "{a:1}" --file ' + exprtFile;
   var rc = cmd.run( exportOption );

   //cat exprt file
   var fileInfo = cmd.run( "cat " + exprtFile );
}

function checkExprtFile ( csName, clName, exprtFile )
{

   var rcObj = cmd.run( "cat " + exprtFile ).split( "\n" );
   var actRC = JSON.stringify( rcObj );

   var cl = db.getCS( csName ).getCL( clName );
   var b1 = cl.find( { a: 50 } ).current().toObj().b.$date; //a:50
   //var currentDate = cmd.run('date "+%Y-%m-%d"').split("\n")[0];
   var expA15 = cmd.run( 'date -d@"-62135625957" +"%Y-%m-%d"' ).split( "\n" )[0];

   var expRC = '["{ \\"a\\": 0, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 1, \\"b\\": { \\"$date\\": \\"1970-01-01\\" } }","{ \\"a\\": 2, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 3, \\"b\\": { \\"$date\\": \\"0001-01-01\\" } }","{ \\"a\\": 4, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }","{ \\"a\\": 5, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 6, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 7, \\"b\\": { \\"$date\\": \\"0001-01-01\\" } }","{ \\"a\\": 8, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }","{ \\"a\\": 9, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 10, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 11, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 12, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 13, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 14, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 15, \\"b\\": { \\"$date\\": \\"' + expA15 + '\\" } }","{ \\"a\\": 16, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 17, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 18, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 20, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 22, \\"b\\": { \\"$date\\": \\"' + expA15 + '\\" } }","{ \\"a\\": 23, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 24, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 25, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 27, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 29, \\"b\\": { \\"$date\\": \\"' + expA15 + '\\" } }","{ \\"a\\": 30, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 31, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 40, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }","{ \\"a\\": 42, \\"b\\": { \\"$date\\": \\"0000-12-31\\" } }","{ \\"a\\": 44, \\"b\\": { \\"$date\\": \\"0000-12-31\\" } }","{ \\"a\\": 50, \\"b\\": { \\"$date\\": \\"' + b1 + '\\" } }","{ \\"a\\": 51, \\"b\\": { \\"$date\\": \\"1901-01-01\\" } }","{ \\"a\\": 52, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 53, \\"b\\": { \\"$date\\": \\"0001-01-01\\" } }","{ \\"a\\": 54, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }","{ \\"a\\": 55, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 56, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 57, \\"b\\": { \\"$date\\": \\"0001-01-01\\" } }","{ \\"a\\": 58, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }","{ \\"a\\": 59, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 60, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 61, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 62, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 63, \\"b\\": { \\"$date\\": \\"' + expA15 + '\\" } }","{ \\"a\\": 64, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 65, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 66, \\"b\\": { \\"$date\\": -9223372036854775808 } }","{ \\"a\\": 68, \\"b\\": { \\"$date\\": 9223372036854775807 } }","{ \\"a\\": 70, \\"b\\": { \\"$date\\": \\"' + expA15 + '\\" } }","{ \\"a\\": 71, \\"b\\": { \\"$date\\": \\"1900-01-01\\" } }","{ \\"a\\": 72, \\"b\\": { \\"$date\\": \\"9999-12-31\\" } }","{ \\"a\\": 80, \\"b\\": { \\"$date\\": \\"1899-12-31\\" } }",""]';

   if( actRC !== expRC )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[exprtFile data:" + expRC + "]\n" +
         "[exprtFile data:" + actRC + "]" );
   }

}