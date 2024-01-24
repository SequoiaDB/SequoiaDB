/************************************************************************
*@Description:   seqDB-5430:字段名重复（包括首行字段名重复、指定的字段名重复）
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5430";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5430.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "a,a,btest" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //backup sdbimport.log and generate new sdbimport.log
   var rt = cmd.run( 'find ./ -maxdepth 1 -name "sdbimport.log"' );
   var newLogFile = '';
   if( rt !== '' )  //file is exist
   {
      var time = cmd.run( 'date "+%Y-%m-%d-%H:%M:%S"' ).split( "\n" )[0];
      newLogFile = 'sdbimport.log_' + time;
      cmd.run( 'mv ./sdbimport.log ./' + newLogFile );
   }

   //---------------------scene1-------------------------
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true'
      + ' --file ' + imprtFile;
   var rc ;

   try
   {
      rc = cmd.run( imprtOption ) ;
   }
   catch( e )
   {
   }

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expParseRecords = "Parsed records: 0";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actParseRecords = rcObj[1];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actParseRecords + ", " + actImportedRecords + "]" );
   }

   //check sdbimport.log
   var logInfo = cmd.run( 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Duplicate field name, name=a"' ).split( "\n" );
   var expV = 2;
   var actV = logInfo.length;
   if( expV !== actV )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expV + "]" +
         "[" + actV + "]" );
   }

   //---------------------scene2-------------------------
   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "b,b,ctest"'
      + ' --file ' + imprtFile;

   try
   {
      rc = cmd.run( imprtOption ) ;
   }
   catch( e )
   {
   }

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expParseRecords = "Parsed records: 0";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actParseRecords = rcObj[1];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expParseRecords !== actParseRecords
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actParseRecords + ", " + actImportedRecords + "]" );
   }

   //check sdbimport.log
   var logInfo = cmd.run( 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Duplicate field name, name=b"' ).split( "\n" );
   var expV = 2;
   var actV = logInfo.length;
   if( expV !== actV )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expV + "]" +
         "[" + actV + "]" );
   }

   // recover log
   if( rt !== '' )  //file is exist
   {
      cmd.run( 'mv ' + newLogFile + ' sdbimport.log' );
   }
}
