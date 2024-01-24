/************************************************************************
*@Description:  seqDB-11186:导入timestamp类型数据
*@Author:            2017-3-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_11186";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/timestamp.json";
   var exprtFile = tmpFileDir + "sdbexprt_11186.csv";

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
   var expParseRecords = "Parsed records: 18";
   var expParseFailure = "Parsed failure: 14";
   var expImportedRecords = "Imported records: 18";
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
   var expFailedNum = 14 * 2;  //include blank line, actual 13 records
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

   var rc = cl.find( { $and: [{ b: { $type: 1, $et: 17 } }, { a: { $ne: 5 } }, { a: { $ne: 20 } }, { a: { $ne: 25 } }] }, { _id: { $include: 0 } } ).sort( { a: 1 } ); //except '{ a:20, b: Timestamp() }'
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   //a:3 a:23
   var localtime1 = turnLocaltime( '1901-12-31T15:54:03.000Z', '%Y-%m-%d-%H.%M.%S.000000' );
   //a:27
   var localtime2 = cmd.run( 'date -d@"-2145945600" "+%Y-%m-%d-%H.%M.%S.000000"' ).split( "\n" )[0];
   //a:29
   var localtime3 = cmd.run( 'date -d@"-2147483648" "+%Y-%m-%d-%H.%M.%S.000000"' ).split( "\n" )[0];

   var expCnt = 15;
   var expRecs = '[{"a":0,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":1,"b":{"$timestamp":"1970-01-01-00.00.00.000000"}},{"a":2,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"a":3,"b":{"$timestamp":"' + localtime1 + '"}},{"a":4,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":6,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":21,"b":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"a":22,"b":{"$timestamp":"2037-12-31-23.59.59.999999"}},{"a":23,"b":{"$timestamp":"' + localtime1 + '"}},{"a":24,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":26,"b":{"$timestamp":"2037-12-31-23.59.59.999000"}},{"a":27,"b":{"$timestamp":"' + localtime2 + '"}},{"a":28,"b":{"$timestamp":"2037-12-31-23.59.59.000000"}},{"a":29,"b":{"$timestamp":"' + localtime3 + '"}},{"a":30,"b":{"$timestamp":"2038-01-19-11.14.07.000000"}}]';
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
   var b3 = cl.find( { a: 3 } ).current().toObj().b.$timestamp;  //a:3,a:23
   var b5 = cl.find( { a: 5 } ).current().toObj().b.$timestamp;  //a:5,b:25
   var b20 = cl.find( { a: 20 } ).current().toObj().b.$timestamp;
   var b29 = cmd.run( "date -d@'-2147483648' +%Y-%m-%d-%H.%M.%S.000000" ).split( "\n" )[0]

   var expRC = '["{ \\"a\\": 0, \\"b\\": { \\"$timestamp\\": \\"1902-01-01-00.00.00.000000\\" } }","{ \\"a\\": 1, \\"b\\": { \\"$timestamp\\": \\"1970-01-01-00.00.00.000000\\" } }","{ \\"a\\": 2, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999999\\" } }","{ \\"a\\": 3, \\"b\\": { \\"$timestamp\\": \\"' + b3 + '\\" } }","{ \\"a\\": 4, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999000\\" } }","{ \\"a\\": 5, \\"b\\": { \\"$timestamp\\": \\"' + b5 + '\\" } }","{ \\"a\\": 6, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999000\\" } }","{ \\"a\\": 20, \\"b\\": { \\"$timestamp\\": \\"' + b20 + '\\" } }","{ \\"a\\": 21, \\"b\\": { \\"$timestamp\\": \\"1902-01-01-00.00.00.000000\\" } }","{ \\"a\\": 22, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999999\\" } }","{ \\"a\\": 23, \\"b\\": { \\"$timestamp\\": \\"' + b3 + '\\" } }","{ \\"a\\": 24, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999000\\" } }","{ \\"a\\": 25, \\"b\\": { \\"$timestamp\\": \\"' + b5 + '\\" } }","{ \\"a\\": 26, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.999000\\" } }","{ \\"a\\": 27, \\"b\\": { \\"$timestamp\\": \\"' + b5 + '\\" } }","{ \\"a\\": 28, \\"b\\": { \\"$timestamp\\": \\"2037-12-31-23.59.59.000000\\" } }","{ \\"a\\": 29, \\"b\\": { \\"$timestamp\\": \\"' + b29 + '\\" } }","{ \\"a\\": 30, \\"b\\": { \\"$timestamp\\": \\"2038-01-19-11.14.07.000000\\" } }",""]';

   if( actRC !== expRC )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[exprtFile data:" + expRC + "]\n" +
         "[exprtFile data:" + actRC + "]" );
   }

}