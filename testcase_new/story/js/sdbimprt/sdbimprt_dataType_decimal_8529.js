/************************************************************************
*@Description:   seqDB-8529:导入csv文件时指定decimal类型但不指定精度
                    int 	long 	double 	decimal 	string 	null
                 seqDB-8532/seqDB-8536
*@Author:   2016-8-3  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_8529";
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
      + ' --type csv --fields "num int,type string,v1 decimal,v2 decimal"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 18";
   var expParseFailure = "Parsed failure: 18";
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
   var expFailedNum = 18;
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 100 } }, { v2: { $type: 1, $et: 100 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 17;  //skip a records: {"num":8,"type":"null","v1":null,"v2":null}
   var expRecs = '[{"num":1,"type":"int","v1":{"$decimal":"-2147483648"},"v2":{"$decimal":"2147483647"}},{"num":2,"type":"long","v1":{"$decimal":"-9223372036854775808"},"v2":{"$decimal":"9223372036854775807"}},{"num":3,"type":"double","v1":{"$decimal":"-170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},"v2":{"$decimal":"170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"num":4,"type":"decimal","v1":{"$decimal":"-9223372036854775809.001"},"v2":{"$decimal":"9223372036854775808.001"}},{"num":5,"type":"number","v1":{"$decimal":"-2147483649"},"v2":{"$decimal":"2147483648"}},{"num":21,"type":"dateMS","v1":{"$decimal":"-377705145943"},"v2":{"$decimal":"253402271999"}},{"num":22,"type":"timestampMS","v1":{"$decimal":"-2147414400000"},"v2":{"$decimal":"2147443199000"}},{"num":31,"type":"intStr","v1":{"$decimal":"-2147483648"},"v2":{"$decimal":"2147483647"}},{"num":32,"type":"longStr","v1":{"$decimal":"-9223372036854775808"},"v2":{"$decimal":"9223372036854775807"}},{"num":33,"type":"doubleStr","v1":{"$decimal":"-170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},"v2":{"$decimal":"170000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"num":34,"type":"decimalStr","v1":{"$decimal":"-9223372036854775809.001"},"v2":{"$decimal":"9223372036854775808.001"}},{"num":35,"type":"numberStr","v1":{"$decimal":"-2147483649"},"v2":{"$decimal":"2147483648"}},{"num":50,"type":"int","v1":{"$decimal":"-2147483649"},"v2":{"$decimal":"2147483648"}},{"num":51,"type":"long","v1":{"$decimal":"-9223372036854775809"},"v2":{"$decimal":"9223372036854775808"}},{"num":52,"type":"double","v1":{"$decimal":"-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},"v2":{"$decimal":"180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"num":57,"type":"dateMSStr","v1":{"$decimal":"-377705145944"},"v2":{"$decimal":"253402272000"}},{"num":58,"type":"timestampMSStr","v1":{"$decimal":"-2147414400001"},"v2":{"$decimal":"2147443199001"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}