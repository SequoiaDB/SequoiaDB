/************************************************************************
*@Description:     seqDB-5499:自定义日期格式导入日期格式数据（--datefmt）,
                              其中包含通配符（*）和特殊字符（任意UTF-8字符）
*@Author:   2016-7-29  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5499";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5499.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,D-M-Y,01-01-1900' + "\n"
      + '2,M-D-Y,12:31:9999' + "\n"
      + '3,D-Y-M,01*2014*01' );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var tmpRec = csName + "_" + clName + "*.rec";
   var datefmt = ["--datefmt DD-MM-YYYY",
      "--datefmt MM:DD:YYYY",
      "--datefmt DD*YYYY*MM"]
   for( i = 0; i < datefmt.length; i++ )
   {
      //remove rec file
      cmd.run( "rm -rf " + tmpRec );

      //import operation
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields "num int,desc string,v1 date" '
         + datefmt[i]
         + ' --file ' + imprtFile;
      var rc = cmd.run( imprtOption );

      //check import results
      var rcObj = rc.split( "\n" );
      var expParseRecords = "Parsed records: 1";
      var expParseFailure = "Parsed failure: 2";
      var expImportedRecords = "Imported records: 1";
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
      var actFailedNum = cmd.run( "cat -v " + rec ).split( "\n" ).length - 1;
      var expFailedNum = 2;
      if( expFailedNum !== actFailedNum )
      {
         throw new Error( "checkCLdata fail,[find]" +
            "[failedRecs:" + expFailedNum + "]" +
            "[failedRecs:" + actFailedNum + "]" );
      }
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( { v1: { $type: 1, $et: 1, $et: 9 } }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 3;
   var expRecs = '[{"num":1,"desc":"D-M-Y","v1":{"$date":"1900-01-01"}},{"num":2,"desc":"M-D-Y","v1":{"$date":"9999-12-31"}},{"num":3,"desc":"D-Y-M","v1":{"$date":"2014-01-01"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}