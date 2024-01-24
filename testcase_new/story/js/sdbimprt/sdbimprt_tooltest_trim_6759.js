/************************************************************************
*@Description:   seqDB-6759:sdbimprt支持配置trim参数忽略空格
                 seqDB-5505/seqDB-6203
*@Author:            2016-8-1  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_6759";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "6759.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1," "' + "\n"
      + "2,,\n"
      + '3,"  atest  "\n'
      + '4,"　　　  全角字符空格　　　  "' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var trimType = ["--trim no",
      "--trim right",
      "--trim left",
      "--trim both"]
   for( i = 0; i < trimType.length; i++ )
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields "num int,v1 string" --ignorenull true '
         + trimType[i]
         + ' --file ' + imprtFile;
      var rc = cmd.run( imprtOption );

      //check import results
      var rcObj = rc.split( "\n" );
      var expParseRecords = "Parsed records: 4";
      var expParseFailure = "Parsed failure: 0";
      var expImportedRecords = "Imported records: 4";
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
   }
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 16;
   var expRecs = '[{"num":1,"v1":" "},{"num":1,"v1":""},{"num":1,"v1":""},{"num":1,"v1":""},{"num":2},{"num":2},{"num":2},{"num":2},{"num":3,"v1":"  atest  "},{"num":3,"v1":"  atest"},{"num":3,"v1":"atest  "},{"num":3,"v1":"atest"},{"num":4,"v1":"　　　  全角字符空格　　　  "},{"num":4,"v1":"　　　  全角字符空格"},{"num":4,"v1":"全角字符空格　　　  "},{"num":4,"v1":"全角字符空格"}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}