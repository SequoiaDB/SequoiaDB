/************************************************************************
*@Description:  seqDB-5392:导入csv文件数据，文件编码为不带BOM的UTF-8格式_st.sdbimprt.001
*@Author:            2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5392";
   var cl = readyCL( csName, clName );

   var imprtFile = testCaseDir + "dataFile/UTF-8_NO-BOM.csv";
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
   var expParseRecords = "Parsed records: 2";
   var expImportedRecords = "Imported records: 2";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }
}

function checkCLData ( cl )
{

   var actCnt = 2;
   var expCnt = Number( cl.count() );
   if( actCnt !== expCnt )
   {
      throw new Error( "checkCLdata fail,[cl count]" +
         "[cnt:" + expCnt + "]" +
         "[cnt:" + actCnt + "]" );
   }
}