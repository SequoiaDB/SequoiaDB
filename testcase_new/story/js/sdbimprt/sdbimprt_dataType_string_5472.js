/************************************************************************
*@Description:  seqDB-5472:导入数据时指定的数据类型为string，实际数据为支持转换的数据类型（all）
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5472";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/allDataType.csv";
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   //cat import file
   //var fileInfo = cmd.run( "cat "+ imprtFile );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string,v1 string,v2 string"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 36";
   var expParseFailure = "Parsed failure: 0";
   var expImportedRecords = "Imported records: 36";
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 2 } }, { v2: { $type: 1, $et: 2 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 36;
   var expRecs = '[{"num":1,"type":"int","v1":"-2147483648","v2":"2147483647"},{"num":2,"type":"long","v1":"-9223372036854775808","v2":"9223372036854775807"},{"num":3,"type":"double","v1":"-1.7E+308","v2":"1.7E+308"},{"num":4,"type":"decimal","v1":"-9223372036854775809.001","v2":"9223372036854775808.001"},{"num":5,"type":"number","v1":"-2147483649","v2":"2147483648"},{"num":6,"type":"bool","v1":"true","v2":"false"},{"num":7,"type":"string","v1":"str","v2":"       "},{"num":8,"type":"null","v1":"null","v2":"null"},{"num":9,"type":"oid","v1":"5791b549b1f90a1171000016","v2":"abcdef01230123456789cdef"},{"num":10,"type":"date","v1":"1900-01-01","v2":"9999-12-31"},{"num":11,"type":"timestamp","v1":"1902-01-01-00.00.00.000000","v2":"2037-12-31-23.59.59.999999"},{"num":12,"type":"binary","v1":"aGVsbG8gd29ybGQ=","v2":"adc="},{"num":13,"type":"regex","v1":"^a","v2":"a\\\\n"},{"num":21,"type":"dateMS","v1":"-377705145943","v2":"253402271999"},{"num":22,"type":"timestampMS","v1":"-2147414400000","v2":"2147443199000"},{"num":31,"type":"intStr","v1":"-2147483648","v2":"2147483647"},{"num":32,"type":"longStr","v1":"-9223372036854775808","v2":"9223372036854775807"},{"num":33,"type":"doubleStr","v1":"-1.7E+308","v2":"1.7E+308"},{"num":34,"type":"decimalStr","v1":"-9223372036854775809.001","v2":"9223372036854775808.001"},{"num":35,"type":"numberStr","v1":"-2147483649","v2":"2147483648"},{"num":36,"type":"boolStr","v1":"true","v2":"false"},{"num":37,"type":"nullStr","v1":"null","v2":"null"},{"num":38,"type":"oidStr","v1":"5791b549b1f90a1171000016","v2":"abcdef01230123456789cdef"},{"num":39,"type":"dateStr","v1":"1900-01-01","v2":"9999-12-31"},{"num":40,"type":"timestampStr","v1":"1902-01-01-00.00.00.000000","v2":"2037-12-31-23.59.59.999999"},{"num":41,"type":"binaryStr","v1":"aGVsbG8gd29ybGQ=","v2":"adc="},{"num":42,"type":"regexStr","v1":"^a","v2":"a\\\\n"},{"num":50,"type":"int","v1":"-2147483649","v2":"2147483648"},{"num":51,"type":"long","v1":"-9223372036854775809","v2":"9223372036854775808"},{"num":52,"type":"double","v1":"-1.8E+308","v2":"1.8E+308"},{"num":53,"type":"oid","v1":"a  ","v2":""},{"num":54,"type":"date","v1":"1899-12-31","v2":"10000-01-01"},{"num":55,"type":"timestamp","v1":"1901-12-31-59.59.59.999999","v2":"2038-01-01-00.00.00.000000"},{"num":56,"type":"binary","v1":"abc","v2":""},{"num":57,"type":"dateMSStr","v1":"-377705145944","v2":"253402272000"},{"num":58,"type":"timestampMSStr","v1":"-2147414400001","v2":"2147443199001"}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}