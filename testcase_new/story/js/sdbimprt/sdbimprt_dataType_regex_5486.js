/************************************************************************
*@Description:    seqDB-5486:导入数据时指定的数据类型为regex，实际数据支持转换的数据类型
                    string  regex
                  seqDB-5487/seqDB-9614
*@Author:   2016-8-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5486";
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
      + ' --type csv --fields "num int,type string,v1 regex,v2 regex"'
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 11 } }, { v2: { $type: 1, $et: 11 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 36;
   var expRecs = '[{"num":1,"type":"int","v1":{"$regex":"-2147483648","$options":""},"v2":{"$regex":"2147483647","$options":""}},{"num":2,"type":"long","v1":{"$regex":"-9223372036854775808","$options":""},"v2":{"$regex":"9223372036854775807","$options":""}},{"num":3,"type":"double","v1":{"$regex":"-1.7E+308","$options":""},"v2":{"$regex":"1.7E+308","$options":""}},{"num":4,"type":"decimal","v1":{"$regex":"-9223372036854775809.001","$options":""},"v2":{"$regex":"9223372036854775808.001","$options":""}},{"num":5,"type":"number","v1":{"$regex":"-2147483649","$options":""},"v2":{"$regex":"2147483648","$options":""}},{"num":6,"type":"bool","v1":{"$regex":"true","$options":""},"v2":{"$regex":"false","$options":""}},{"num":7,"type":"string","v1":{"$regex":"str","$options":""},"v2":{"$regex":"       ","$options":""}},{"num":8,"type":"null","v1":{"$regex":"null","$options":""},"v2":{"$regex":"null","$options":""}},{"num":9,"type":"oid","v1":{"$regex":"5791b549b1f90a1171000016","$options":""},"v2":{"$regex":"abcdef01230123456789cdef","$options":""}},{"num":10,"type":"date","v1":{"$regex":"1900-01-01","$options":""},"v2":{"$regex":"9999-12-31","$options":""}},{"num":11,"type":"timestamp","v1":{"$regex":"1902-01-01-00.00.00.000000","$options":""},"v2":{"$regex":"2037-12-31-23.59.59.999999","$options":""}},{"num":12,"type":"binary","v1":{"$regex":"aGVsbG8gd29ybGQ=","$options":""},"v2":{"$regex":"adc=","$options":""}},{"num":13,"type":"regex","v1":{"$regex":"^a","$options":""},"v2":{"$regex":"a\\\\n","$options":""}},{"num":21,"type":"dateMS","v1":{"$regex":"-377705145943","$options":""},"v2":{"$regex":"253402271999","$options":""}},{"num":22,"type":"timestampMS","v1":{"$regex":"-2147414400000","$options":""},"v2":{"$regex":"2147443199000","$options":""}},{"num":31,"type":"intStr","v1":{"$regex":"-2147483648","$options":""},"v2":{"$regex":"2147483647","$options":""}},{"num":32,"type":"longStr","v1":{"$regex":"-9223372036854775808","$options":""},"v2":{"$regex":"9223372036854775807","$options":""}},{"num":33,"type":"doubleStr","v1":{"$regex":"-1.7E+308","$options":""},"v2":{"$regex":"1.7E+308","$options":""}},{"num":34,"type":"decimalStr","v1":{"$regex":"-9223372036854775809.001","$options":""},"v2":{"$regex":"9223372036854775808.001","$options":""}},{"num":35,"type":"numberStr","v1":{"$regex":"-2147483649","$options":""},"v2":{"$regex":"2147483648","$options":""}},{"num":36,"type":"boolStr","v1":{"$regex":"true","$options":""},"v2":{"$regex":"false","$options":""}},{"num":37,"type":"nullStr","v1":{"$regex":"null","$options":""},"v2":{"$regex":"null","$options":""}},{"num":38,"type":"oidStr","v1":{"$regex":"5791b549b1f90a1171000016","$options":""},"v2":{"$regex":"abcdef01230123456789cdef","$options":""}},{"num":39,"type":"dateStr","v1":{"$regex":"1900-01-01","$options":""},"v2":{"$regex":"9999-12-31","$options":""}},{"num":40,"type":"timestampStr","v1":{"$regex":"1902-01-01-00.00.00.000000","$options":""},"v2":{"$regex":"2037-12-31-23.59.59.999999","$options":""}},{"num":41,"type":"binaryStr","v1":{"$regex":"aGVsbG8gd29ybGQ=","$options":""},"v2":{"$regex":"adc=","$options":""}},{"num":42,"type":"regexStr","v1":{"$regex":"^a","$options":""},"v2":{"$regex":"a\\\\n","$options":""}},{"num":50,"type":"int","v1":{"$regex":"-2147483649","$options":""},"v2":{"$regex":"2147483648","$options":""}},{"num":51,"type":"long","v1":{"$regex":"-9223372036854775809","$options":""},"v2":{"$regex":"9223372036854775808","$options":""}},{"num":52,"type":"double","v1":{"$regex":"-1.8E+308","$options":""},"v2":{"$regex":"1.8E+308","$options":""}},{"num":53,"type":"oid","v1":{"$regex":"a  ","$options":""},"v2":{"$regex":"","$options":""}},{"num":54,"type":"date","v1":{"$regex":"1899-12-31","$options":""},"v2":{"$regex":"10000-01-01","$options":""}},{"num":55,"type":"timestamp","v1":{"$regex":"1901-12-31-59.59.59.999999","$options":""},"v2":{"$regex":"2038-01-01-00.00.00.000000","$options":""}},{"num":56,"type":"binary","v1":{"$regex":"abc","$options":""},"v2":{"$regex":"","$options":""}},{"num":57,"type":"dateMSStr","v1":{"$regex":"-377705145944","$options":""},"v2":{"$regex":"253402272000","$options":""}},{"num":58,"type":"timestampMSStr","v1":{"$regex":"-2147414400001","$options":""},"v2":{"$regex":"2147443199001","$options":""}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}