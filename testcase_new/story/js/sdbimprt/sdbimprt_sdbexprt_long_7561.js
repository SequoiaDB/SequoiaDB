/************************************************************************
*@Description:  seqDB-7561:导入导出numberLong
*@Author:           2016-8-3  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_7561";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "7561.csv";
   var exprtFile = tmpFileDir + "sdbexprt_7561.csv";

   readyData( imprtFile );
   importData( csName, clName, imprtFile );
   checkCLData( cl );

   exprtData( csName, clName, exprtFile );
   checkExprtFile( exprtFile );

   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "1,long,-2147483649,2147483648\n"
      + "2,long,-9223372036854775808,9223372036854775807" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string, v1 long, v2 long"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );

   //check import results
   var expParseRecords = "Parsed records: 2";
   var expParseFailure = "Parsed failure: 0";
   var expImportedRecords = "Imported records: 2";
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

   var rc = cl.find( { $and: [{ v1: { $type: 1, $et: 18 } }, { v2: { $type: 1, $et: 18 } }] }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 2;
   var expRecs = '[{"num":1,"type":"long","v1":-2147483649,"v2":2147483648},{"num":2,"type":"long","v1":{"$numberLong":"-9223372036854775808"},"v2":{"$numberLong":"9223372036854775807"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
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
      + ' --type csv --fields "num,type,v1,v2"'
      + ' --sort "{num:1}" --file ' + exprtFile;
   var rc = cmd.run( exportOption );

   //cat exprt file
   var fileInfo = cmd.run( "cat " + exprtFile );
}

function checkExprtFile ( exprtFile )
{

   var rcObj = cmd.run( "cat " + exprtFile ).split( "\n" );
   var actRC = JSON.stringify( rcObj );
   var expRC = '["num,type,v1,v2","1,\\"long\\",-2147483649,2147483648","2,\\"long\\",-9223372036854775808,9223372036854775807",""]';

   if( actRC !== expRC )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[exprtFile data:" + expRC + "]" +
         "[exprtFile data:" + actRC + "]" );
   }

}