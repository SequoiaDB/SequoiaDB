/************************************************************************
*@Description:    seqDB-5466:导入数据时指定的数据类型为bool(boolean)，实际数据为支持转换的数据类型
                    int 	long 	bool 	string 	null
                  seqDB-5467
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5466";
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
      + ' --type csv --fields "num int,type string,v1 bool,v2 bool"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 14";
   var expParseFailure = "Parsed failure: 22";
   var expImportedRecords = "Imported records: 14";
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
   var expFailedNum = 22;
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 8 } }, { v2: { $type: 1, $et: 8 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 13;  //skip a records: {"num":8,"type":"null","v1":null,"v2":null}
   var expRecs = '[{"num":1,"type":"int","v1":true,"v2":true},{"num":2,"type":"long","v1":true,"v2":true},{"num":5,"type":"number","v1":true,"v2":true},{"num":6,"type":"bool","v1":true,"v2":false},{"num":21,"type":"dateMS","v1":true,"v2":true},{"num":22,"type":"timestampMS","v1":true,"v2":true},{"num":31,"type":"intStr","v1":true,"v2":true},{"num":32,"type":"longStr","v1":true,"v2":true},{"num":35,"type":"numberStr","v1":true,"v2":true},{"num":36,"type":"boolStr","v1":true,"v2":false},{"num":50,"type":"int","v1":true,"v2":true},{"num":57,"type":"dateMSStr","v1":true,"v2":true},{"num":58,"type":"timestampMSStr","v1":true,"v2":true}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}