/************************************************************************
*@Description:   seqDB-5401:指定目录导入数据，目录包含多个json和csv格式文件
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5401";
   var cl = readyCL( csName, clName );

   var imprtFile1 = tmpFileDir + "5401_1.json";
   var imprtFile2 = tmpFileDir + "5401_2.json";
   readyData( imprtFile1, imprtFile2 );
   importData( csName, clName );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile1, imprtFile2 )
{

   var file = fileInit( imprtFile1 );
   file.write( "{a:1}\n{a:2}" );
   var fileInfo = cmd.run( "cat " + imprtFile1 );
   file.close();

   var file = fileInit( imprtFile2 );
   file.write( "{a:3}\n{a:4}" );
   var fileInfo = cmd.run( "cat " + imprtFile2 );
   file.close();

}

function importData ( csName, clName )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type json'
      + ' --file ' + tmpFileDir;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 4";
   var expImportedRecords = "Imported records: 4";
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

   var expCnt = 4;
   var expRecs = '[{"a":1},{"a":2},{"a":3},{"a":4}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}