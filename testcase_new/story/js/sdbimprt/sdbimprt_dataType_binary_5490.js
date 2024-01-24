/************************************************************************
*@Description:    seqDB-5490:导入数据时指定的数据类型为binary，实际数据为不支持转换的数据类型
*@Author:            2016-8-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5490";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5490.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,""' + "\n"
      + '2,"a"' + "\n"
      + '3,"abc"' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "num int,v1 binary"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 0";
   var expParseFailure = "Parsed failure: 3";
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
   var expFailedNum = 3;
   if( expFailedNum !== actFailedNum )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[failedRecs:" + expFailedNum + "]" +
         "[failedRecs:" + actFailedNum + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
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