/************************************************************************
*@Description:    seqDB-5474:导入数据时指定的数据类型为timestamp，实际数据为不支持转换的数据类型
                  seqDB-5473
*@Author:   2016-7-29  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5474";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5474.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '1,str,"1902-01-01"' + "\n"
      + '2,date,1902-01-01' + "\n"
      + '3,invalid,2014-01-01-10.30' + "\n"
      + '4,invalid,2014-01-01-10' + "\n"
      + '5,date,"1902-01-01"' + "\n"
      + '6,invalid,""' + "\n"
      + '7,null,,' );
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
      + ' --type csv --fields "num int,type string,v1 timestamp"'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 6";
   var expParseFailure = "Parsed failure: 1";
   var expImportedRecords = "Imported records: 6";
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
   var expFailedNum = 1;
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

   var rc = cl.find( { v1: { $type: 1, $et: 17 } }, { _id: { $include: 0 } } ).sort( { num: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 5;
   var expRecs = '[{"num":1,"type":"str","v1":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"num":2,"type":"date","v1":{"$timestamp":"1902-01-01-00.00.00.000000"}},{"num":3,"type":"invalid","v1":{"$timestamp":"2014-01-01-10.30.00.000000"}},{"num":4,"type":"invalid","v1":{"$timestamp":"2014-01-01-10.00.00.000000"}},{"num":5,"type":"date","v1":{"$timestamp":"1902-01-01-00.00.00.000000"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}