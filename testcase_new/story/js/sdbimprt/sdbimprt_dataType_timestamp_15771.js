/*******************************************************************************
*@Description:   seqDB-15771:fields的timestamp类型支持自定义格式
*@Author:        2018-9-10  wangkexin
********************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_15771";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "15771.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( '_id,a,b,c,d,e,f\n1,"2014-01-01-12.30.20", ,"2012.01.03","2018?09!07@01\\02*|*03","2000-01","1968-09 10:12.13"\n2,"2014-01-01","2008","2012.01.15","2018?09","2000-01","1971-12 10:12.13.14"' );
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
      + ' --type csv '
      + ' --fields=\'_id int, a timestamp, b timestamp("YYYY") default 2018, c timestamp("YYYY.MM.DD") , d timestamp("YYYY?MM!DD@HH\\mm*|*ss") ,e timestamp("YYYY-MM"), f timestamp("YYYY-MM DD:HH.mm")\' '
      + ' --headerline true '
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 2";
   var expParseFailure = "Parsed failure: 0";
   var expImportedRecords = "Imported records: 2";
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

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "_id": 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 2;
   var expRecs = '[{"a":{"$timestamp":"2014-01-01-12.30.20.000000"},"b":{"$timestamp":"2018-01-01-00.00.00.000000"},"c":{"$timestamp":"2012-01-03-00.00.00.000000"},"d":{"$timestamp":"2018-09-07-01.02.03.000000"},"e":{"$timestamp":"2000-01-01-00.00.00.000000"},"f":{"$timestamp":"1968-09-10-12.13.00.000000"}},'
      + '{"a":{"$timestamp":"2014-01-01-00.00.00.000000"},"b":{"$timestamp":"2008-01-01-00.00.00.000000"},"c":{"$timestamp":"2012-01-15-00.00.00.000000"},"d":{"$timestamp":"2018-09-01-00.00.00.000000"},"e":{"$timestamp":"2000-01-01-00.00.00.000000"},"f":{"$timestamp":"1971-12-10-12.13.00.000000"}}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}
