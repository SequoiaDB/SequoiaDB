/************************************************************************
*@Description:    seqDB-5429:指定首行作为字段名，字段值为空
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5429";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5429.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "a,b" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );

   var expParseRecords = "Parsed records: 0";
   var expImportedRecords = "Imported records: 0";
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { a: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 0;
   var actCnt = recsArray.length;
   if( actCnt !== expCnt )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + "]" +
         "[cnt:" + actCnt + "]" );
   }

}