/************************************************************************
*@Description:  seqDB-6762:导入非utf-8格式的文件数据
*@Author:            2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6762";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/NOT_UTF-8";
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "a" --sparse true'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed failure: 1";
   var expImportedRecords = "Imported records: 0";
   var actParseRecords = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }

   // clean tmpRec
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var actCnt = 0;
   var expCnt = Number( cl.count() );
   if( actCnt !== expCnt )
   {
      throw new Error( "checkCLdata fail,[cl count]" +
         "[cnt:" + expCnt + "]" +
         "[cnt:" + actCnt + "]" );
   }
}