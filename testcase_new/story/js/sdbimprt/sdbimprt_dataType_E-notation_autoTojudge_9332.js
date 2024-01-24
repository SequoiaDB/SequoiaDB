/************************************************************************
*@Description:  seqDB-9332:原数据为科学计数法，自动判断类型导入
*@Author:   2016-8-18  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_9332";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/E-notation.csv";
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
      + ' --type csv --fields "num,type,v1,v2"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 13";
   var expImportedRecords = "Imported records: 13";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
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

   var expCnt = 13;
   var expRecs = '[{"num":1,"type":"int","v1":-10,"v2":10},{"num":2,"type":"decimal","v1":-2147483648,"v2":2147483647},{"num":3,"type":"decimal","v1":{"$decimal":"-9223372036854775808"},"v2":{"$decimal":"9223372036854775807"}},{"num":4,"type":"double","v1":-1.7e+308,"v2":1.7e+308},{"num":5,"type":"decimal","v1":{"$decimal":"-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"},"v2":{"$decimal":"180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"num":6,"type":"double","v1":-1000000090.00006,"v2":1000000090.00006},{"num":7,"type":"decimal","v1":{"$decimal":"-10000000100.00001"},"v2":{"$decimal":"10000000100.00001"}},{"num":8,"type":"decimal","v1":-0.000007,"v2":0.000007},{"num":9,"type":"string","v1":"1e+1+1","v2":"1e-1-1"},{"num":10,"type":"decimal","v1":-1.7e-308,"v2":1.7e-308},{"num":11,"type":"double","v1":19.9714,"v2":0.01211},{"num":12,"type":"int","v1":1200,"v2":1},{"num":13,"type":"int","v1":-1,"v2":1}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}