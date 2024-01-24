/************************************************************************
*@Description:   seqDB-5444:指定并发数为1000（最大并发数）
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5444";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5444.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   for( i = 0; i < 2000; i++ )
   {
      file.write( String( i + ",test_" + i + "\n" ) );
   }
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields a,b'
      + ' --insertnum 2 --jobs 200'  //1000
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   sleep( 1500 );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 2000";
   var expImportedRecords = "Imported records: 2000";
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

   var expCnt = 2000;
   var expMinRecs = '{"a":0,"b":"test_0"}';
   var expMaxRecs = '{"a":1999,"b":"test_1999"}';
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