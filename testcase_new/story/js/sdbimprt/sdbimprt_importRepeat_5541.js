/************************************************************************
*@Description:    seqDB-5541:重复导入
*@Author:           2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5541";
   var idxName = "idx_a";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5541.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "1,test\n"
      + "2,test" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var times = 2;
   for( i = 0; i < times; i++ )
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields "a int,b string"'
         + ' --file ' + imprtFile;
      var rc = cmd.run( imprtOption );
      var rcObj = rc.split( "\n" );

      var expParseRecords = "Parsed records: 2";
      var expImportedRecords = "Imported records: 2";
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
   var expRecs = '[{"a":1,"b":"test"},{"a":1,"b":"test"},{"a":2,"b":"test"},{"a":2,"b":"test"}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}