/*******************************************************************************
*@Description:   seqDB-11202:csv原数据为"a""",导入并导出
*@Author:        2019-3-5  wangkexin
********************************************************************************/
var key = "a";
var csvContent = key + "\n" + "\"a\"\"\"" + "\n";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_11202";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "11202.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   exportData( clName );
   cleanCL( csName, clName );
}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( csvContent );
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
      + ' --type csv '
      + ' --file ' + imprtFile
      + ' --headerline=true ';
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 1";
   var expImportedRecords = "Imported records: 1";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   rc.close();
   var expCnt = 1;
   var expRecs = '[{"a":"a\\\""}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}

function exportData ( clname )
{
   var csvfile = tmpFileDir + "sdbexprt11202.csv";
   cmd.run( "rm -rf " + csvfile );
   var command = installDir + "bin/sdbexprt" +
      " -s " + COORDHOSTNAME +
      " -p " + COORDSVCNAME +
      " -c " + COMMCSNAME +
      " -l " + clname +
      " --file " + csvfile +
      " --type csv" +
      " --fields " + key;
   var rc = cmd.run( command );

   checkFileContent( csvfile, csvContent );
}

function checkFileContent ( filename, expContent )
{
   var size = parseInt( File.stat( filename ).toObj().size );
   var file = new File( filename );
   var actContent = file.read( size );
   file.close();
   if( actContent !== expContent )
   {
      throw new Error( "checkFileContent fail" + "check " + filename + " content" + expContent, actContent );
   }
}