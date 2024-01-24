/************************************************************************
*@Description:   seqDB-13621 : 使用空格或制表符作为字段分隔符导入数据
*@Author:        2017-11-23  linsuqiang
************************************************************************/

main( test );

function test ()
{

   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_13621";
   var cl = readyCL( csName, clName );

   var file4Tab = tmpFileDir + "13621_tab.csv";
   readyData( file4Tab, "\t" );
   importData( csName, clName, file4Tab, "\t" );
   checkCLData( cl, "\t" );

   cl.truncate();

   var file4Space = tmpFileDir + "13621_space.csv";
   readyData( file4Space, " " );
   importData( csName, clName, file4Space, " " );
   checkCLData( cl, " " );

   cleanCL( csName, clName );

}

function readyData ( imprtFile, separator )
{

   var file = fileInit( imprtFile );
   var sep = separator;
   file.write( "_id" + sep + "a" + sep + "b" + sep + "c" + sep + "d" + sep + "\n" );
   file.write( "1" + sep + "1" + sep + "2" + sep + sep + "4" + sep + "\n" );
   file.write( "2" + sep + sep + sep + "3" + sep + "4" + sep + "\n" );
   file.write( "3" + sep + "1" + sep + "2" + sep + sep + sep + "\n" );
   file.write( "4" + sep + "1" + sep + sep + "3" + sep + sep + "\n" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}

function importData ( csName, clName, imprtFile, separator )
{

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv '
      + ' --headerline true '
      + ' -e "' + separator + '"'
      + ' --file ' + imprtFile;
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

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { _id: 1 } );
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }

   var expCnt = 4;
   var expRecs = '[{"a":1,"b":2,"d":4},\
{"c":3,"d":4},\
{"a":1,"b":2},\
{"a":1,"c":3}]';
   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }

}
