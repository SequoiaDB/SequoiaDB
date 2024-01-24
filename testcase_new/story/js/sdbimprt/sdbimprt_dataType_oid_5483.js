/************************************************************************
*@Description:   seqDB-5483:导入数据时指定的数据类型为oid，实际数据为大于24个字符的字符串
				 seqDB-5484/seqDB-5485/seqDB-5486
*@Author:           2016-8-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5483";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5483.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,null,null' + "\n"
      + '2,invalid,5791b549b1f90a11710000165' + "\n"
      + '3,invalid,5791b549b1f90a117100001' + "\n"
      + '4,invalid,5791b54%*$_-a1171000016' + "\n"
      + '5,invalid,5791b54      1171000016' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //import operation
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,type string,v1 oid"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 0";
   var expParseFailure = "Parsed failure: 5";
   var expImportedRecords = "Imported records: 0";
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
   var expFailedNum = 5;
   if( expFailedNum !== actFailedNum )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[failedRecs:" + expFailedNum + "]" +
         "[failedRecs:" + actFailedNum + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}