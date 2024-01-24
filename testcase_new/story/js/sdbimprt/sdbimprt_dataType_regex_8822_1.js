/************************************************************************
*@Description:    seqDB-8822:导入正则表达式数据包含各种转义字符(csv)
*@Author:            2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_8822";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "8822.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,"/^(?:\\\\x22?\\\\x5C[\\\\x00-\\\\x7E]\\\\x22?)|(?:\\\\x22?[^\\\\x5C\\\\x22]\\\\x22?)|(?:\\\\x22?[^\\\\x5C\\\\x22]\\\\x22?)/"' + "\n"
      + '2,"^\\d+$"' + "\n"
      + '3,"\\f\\b\\r\\n"' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --fields "a int,b regex"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 3";
   var expImportedRecords = "Imported records: 3";
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

   var expCnt = 3;
   var expRecs = '[{"a":1,"b":{"$regex":"^(?:\\\\\\\\x22?\\\\\\\\x5C[\\\\\\\\x00-\\\\\\\\x7E]\\\\\\\\x22?)|(?:\\\\\\\\x22?[^\\\\\\\\x5C\\\\\\\\x22]\\\\\\\\x22?)|(?:\\\\\\\\x22?[^\\\\\\\\x5C\\\\\\\\x22]\\\\\\\\x22?)","$options":""}},{"a":2,"b":{"$regex":"^\\\\d+$","$options":""}},{"a":3,"b":{"$regex":"\\\\f\\\\b\\\\r\\\\n","$options":""}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}