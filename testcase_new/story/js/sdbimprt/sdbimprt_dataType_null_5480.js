/************************************************************************
*@Description:  seqDB-5480:导入数据时指定的数据类型为null，实际为支持的数据类型（all）
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5480";
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
      + ' --type csv --fields "num int,type string,v1 null,v2 null"'
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 10 } }, { v2: { $type: 1, $et: 10 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 36;
   var expRecs = '[{"num":1,"type":"int","v1":null,"v2":null},{"num":2,"type":"long","v1":null,"v2":null},{"num":3,"type":"double","v1":null,"v2":null},{"num":4,"type":"decimal","v1":null,"v2":null},{"num":5,"type":"number","v1":null,"v2":null},{"num":6,"type":"bool","v1":null,"v2":null},{"num":7,"type":"string","v1":null,"v2":null},{"num":8,"type":"null","v1":null,"v2":null},{"num":9,"type":"oid","v1":null,"v2":null},{"num":10,"type":"date","v1":null,"v2":null},{"num":11,"type":"timestamp","v1":null,"v2":null},{"num":12,"type":"binary","v1":null,"v2":null},{"num":13,"type":"regex","v1":null,"v2":null},{"num":21,"type":"dateMS","v1":null,"v2":null},{"num":22,"type":"timestampMS","v1":null,"v2":null},{"num":31,"type":"intStr","v1":null,"v2":null},{"num":32,"type":"longStr","v1":null,"v2":null},{"num":33,"type":"doubleStr","v1":null,"v2":null},{"num":34,"type":"decimalStr","v1":null,"v2":null},{"num":35,"type":"numberStr","v1":null,"v2":null},{"num":36,"type":"boolStr","v1":null,"v2":null},{"num":37,"type":"nullStr","v1":null,"v2":null},{"num":38,"type":"oidStr","v1":null,"v2":null},{"num":39,"type":"dateStr","v1":null,"v2":null},{"num":40,"type":"timestampStr","v1":null,"v2":null},{"num":41,"type":"binaryStr","v1":null,"v2":null},{"num":42,"type":"regexStr","v1":null,"v2":null},{"num":50,"type":"int","v1":null,"v2":null},{"num":51,"type":"long","v1":null,"v2":null},{"num":52,"type":"double","v1":null,"v2":null},{"num":53,"type":"oid","v1":null,"v2":null},{"num":54,"type":"date","v1":null,"v2":null},{"num":55,"type":"timestamp","v1":null,"v2":null},{"num":56,"type":"binary","v1":null,"v2":null},{"num":57,"type":"dateMSStr","v1":null,"v2":null},{"num":58,"type":"timestampMSStr","v1":null,"v2":null}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}