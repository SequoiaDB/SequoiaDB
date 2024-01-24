/************************************************************************
*@Description:   seqDB-8531:导入csv文件指定decimal类型且指定精度命令行验证
*@Author:           2016-8-3  huangxiaoni
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_8531";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "8531.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( "1" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   var decimalFmt = ["a decimal(1001,1)",
      "a decimal(10,11)",
      "a decimal(0,0)",
      "a decimal(10,10)"];
   for( i = 0; i < decimalFmt.length; i++ )
   {
      var rt = cmd.run( 'find ./ -maxdepth 1 -name "sdbimport.log"' );
      var newLogFile = '';
      if( rt !== '' )  //file is exist
      {
         var time = cmd.run( 'date "+%Y-%m-%d-%H:%M:%S"' ).split( "\n" )[0];
         newLogFile = 'sdbimport.log_' + time;
         cmd.run( 'mv ./sdbimport.log ./' + newLogFile );
      }


      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv --fields "' + decimalFmt[i]
         + '" --file ' + imprtFile;
      var rc ;

      try
      {
         rc = cmd.run( imprtOption );
      }
      catch( e )
      {
      }

      //check import results
      if( i < decimalFmt.length - 1 )
      {
         var logInfo = cmd.run( 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Invalid decimal type"' ).split( "\n" );
         var expV = 2;
         var actV = logInfo.length;
         if( expV !== actV )
         {
            throw new Error( "importData fail,[sdbimprt results]" +
               "[" + expV + "]" +
               "[" + actV + "]" );
         }

         if( rt !== '' )  //file is exist
         {
            cmd.run( 'mv ' + newLogFile + ' sdbimport.log' );
         }
      }
      else
      {
         var rcObj = rc.split( "\n" );
         var expParseRecords = "Parsed records: 0";
         var expParseFailure = "Parsed failure: 1";
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
      }
   }

   // clean tmpRec
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find();
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