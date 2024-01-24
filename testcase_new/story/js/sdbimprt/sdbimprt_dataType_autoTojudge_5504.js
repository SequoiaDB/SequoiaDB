/************************************************************************
*@Description:  seqDB-5504:导入数据覆盖所有CSV转BSON的数据类型的数据（参考信息中心“CSV类型自动判断”）
                    int 	long 	bool 	string 	null
                seqDB-6200
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5504";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/autoToJudge.csv";
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   //cat import file
   var fileInfo = cmd.run( "cat " + imprtFile );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,oriV1,type string,srcV2"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 18";
   var expParseFailure = "Parsed failure: 0";
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
}

function checkCLData ( cl )
{

   var rc = cl.find( { oriV1: { $field: "srcV2" } }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 18;  //skip a records: {"num":8,"type":"null","v1":null,"v2":null}
   var expRecs = '[{"num":1,"oriV1":123,"type":"int","srcV2":123},{"num":2,"oriV1":123,"type":"int","srcV2":123},{"num":3,"oriV1":123,"type":"int","srcV2":123},{"num":4,"oriV1":-123,"type":"int","srcV2":-123},{"num":5,"oriV1":123,"type":"int","srcV2":123},{"num":6,"oriV1":-123,"type":"int","srcV2":-123},{"num":7,"oriV1":2147483648,"type":"long","srcV2":2147483648},{"num":8,"oriV1":123.1,"type":"double","srcV2":123.1},{"num":9,"oriV1":0.123,"type":"double","srcV2":0.123},{"num":10,"oriV1":{"$decimal":"9223372036854775808"},"type":"decimal","srcV2":{"$decimal":"9223372036854775808"}},{"num":11,"oriV1":true,"type":"bool","srcV2":true},{"num":12,"oriV1":false,"type":"bool","srcV2":false},{"num":13,"oriV1":"123","type":"string","srcV2":"123"},{"num":14,"oriV1":"123a","type":"string","srcV2":"123a"},{"num":15,"oriV1":"true","type":"string","srcV2":"true"},{"num":16,"oriV1":"false","type":"string","srcV2":"false"},{"num":17,"oriV1":"null","type":"string","srcV2":"null"},{"num":18,"oriV1":null,"type":"null","srcV2":null}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}