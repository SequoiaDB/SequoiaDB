/************************************************************************
*@Description:    seqDB-5432:字段名非法参数校验
*@Author:   2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5432";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5432.csv";
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true'
      + ' --file ' + imprtFile;
   //------------------scene:1--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'$a'\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );

   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:2--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'a.b\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );

   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:3--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "''\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );

   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   //------------------scene:4--------------------------------
   cmd.run( "rm -f " + imprtFile );
   var file = fileInit( imprtFile );
   file.write( "'a\t'\n1" );
   var fileInfo = cmd.run( "cat " + imprtFile );

   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expError = "Failed to parse fields";
   var expImportedRecords = "Imported records: 0";
   var actError = rcObj[0];
   var actImportedRecords = rcObj[5];
   if( expError !== actError || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expError + ", " + expImportedRecords + "]" +
         "[" + actError + ", " + actImportedRecords + "]" );
   }

   file.close();
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
