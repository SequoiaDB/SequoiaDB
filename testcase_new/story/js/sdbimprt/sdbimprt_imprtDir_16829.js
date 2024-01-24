/*******************************************************************************
*@Description:   seqDB-16829:headerline=true 时导入目录下多个文件
*@Author:        2018-12-19  wangkexin
********************************************************************************/


main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_16829";
   var cl = readyCL( csName, clName );

   var imprtFile1 = tmpFileDir + "16829_1.csv";
   var imprtFile2 = tmpFileDir + "16829_2.csv";
   readyData( imprtFile1, imprtFile2 );
   importData( csName, clName );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile1, imprtFile2 )
{

   var file = fileInit( imprtFile1 );
   file.write( 'name, age, country\n"Jack",18,"China"\n"Mike",20,"USA"' );
   var fileInfo = cmd.run( "cat " + imprtFile1 );
   file.close();

   var file = fileInit( imprtFile2 );
   file.write( 'name, age, country\n"Jack1",181,"China1"\n"Mike1",201,"USA1"' );
   var fileInfo = cmd.run( "cat " + imprtFile2 );
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
      + ' --file ' + tmpFileDir
      + ' --headerline=true ';
   var rc = cmd.run( imprtOption );

   //check import results
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

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   //sort the result array by age
   recsArray.sort( function( obj1, obj2 )
   {
      return obj1.age - obj2.age;
   } )

   var expCnt = 4;
   var expRecs = '[{"name":"Jack","age":18,"country":"China"},{"name":"Mike","age":20,"country":"USA"},{"name":"Jack1","age":181,"country":"China1"},{"name":"Mike1","age":201,"country":"USA1"}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}
