/************************************************************************
*@Description:  seqDB-6109:导入浮点型数据后再执行sdbexprt导出
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6109";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/autoToJudge.csv";
   var exprtFile = tmpFileDir + "sdbexprt_6109.csv";

   importData( csName, clName, imprtFile );
   exprtData( csName, clName, exprtFile );

   checkExprtFile( exprtFile );
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

function exprtData ( csName, clName, exprtFile )
{

   //remove export file
   cmd.run( "rm -rf " + exprtFile );

   //export operation
   var exportOption = installDir + 'bin/sdbexprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num,srcV2"'
      + ' --sort "{num:1}" --file ' + exprtFile;
   var rc = cmd.run( exportOption );

   //cat exprt file
   var fileInfo = cmd.run( "cat " + exprtFile );
}

function checkExprtFile ( exprtFile )
{

   var rcObj = cmd.run( "cat " + exprtFile ).split( "\n" );
   var actRC = JSON.stringify( rcObj );
   var expRC = '["num,srcV2","1,123","2,123","3,123","4,-123","5,123","6,-123","7,2147483648","8,123.1","9,0.123","10,9223372036854775808","11,true","12,false","13,\\"123\\"","14,\\"123a\\"","15,\\"true\\"","16,\\"false\\"","17,\\"null\\"","18,null",""]';

   if( actRC !== expRC )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[exprtFile data:" + expRC + "]" +
         "[exprtFile data:" + actRC + "]" );
   }

}