/*******************************************************************************
*@Description:   seqDB-11549:csv原数据带转义字符和空格，指定trim参数导入数据 
*@Author:        2019-3-5  wangkexin
********************************************************************************/

var csvContent = '" ""Logicom Systems"" Ltd."' + "\n";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_11549";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "11549.csv";
   readyData( imprtFile );
   importData( csName, clName, imprtFile );

   checkCLData( cl );
   cleanCL( csName, clName );

}

function readyData ( imprtFile )
{

   var file = fileInit( imprtFile );
   file.write( csvContent );
   file.close();
}

function importData ( csName, clName, imprtFile )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //import operation
   var imprtOption = installDir + "bin/sdbimprt -s " + COORDHOSTNAME + " -p " + COORDSVCNAME
      + " -c " + csName + " -l " + clName
      + " --file " + imprtFile
      + " --type csv "
      + " -a '\"' "
      + " -e ',' "
      + " --fields 'gfmc string default \"\"'"
      + " --trim 'both'";
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: 1";
   var expImportedRecords = "Imported records: 1";
   var actParseRecords = rcObj[0];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expImportedRecords !== actImportedRecords )
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
   rc.close();

   var expCnt = 1;
   var expRecs = '[{"gfmc":"\\\"Logicom Systems\\\" Ltd."}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
}