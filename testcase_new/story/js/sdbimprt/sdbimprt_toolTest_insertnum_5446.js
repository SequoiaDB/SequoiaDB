/************************************************************************
*@Description:   seqDB-5446:指定批量导入insertnum为100000
                 seqDB-5449
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5446";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5446.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   for( i = 0; i < 101; i++ )
   {
      file.write( String( i + ",abc_\n-def;" ) );
   }
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a,b -r ";"'
      + ' --insertnum 100000'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 101";
   var expImportedRecords = "Imported records: 101";
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

   var expCnt = 101;
   var expMinRecs = '{"a":0,"b":"abc_\\n-def"}';
   var expMaxRecs = '{"a":100,"b":"abc_\\n-def"}';
   var actCnt = Number( cl.count() );
   var actMinRecs = JSON.stringify( cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } ).limit( 1 ).current().toObj() );
   var actMaxRecs = JSON.stringify( cl.find( {}, { _id: { $include: 0 } } ).sort( { a: -1 } ).limit( 1 ).current().toObj() );
   if( expCnt !== actCnt || expMinRecs !== actMinRecs || expMaxRecs !== actMaxRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", minRecs:" + expMinRecs + ", maxRecs" + expMaxRecs + "]" +
         "[cnt:" + actCnt + ", minRecs:" + actMinRecs + ", maxRecs" + actMaxRecs + "]" );
   }

}