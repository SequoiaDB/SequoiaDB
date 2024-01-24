/************************************************************************
*@Description:   seqDB-22775:--parsers并发解析任务数
*@Author:        2020-9-25  huangxiaoni
************************************************************************/
// CI-2133 该用例占用内存太大，注释掉不执行
// main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_22775";
   var cl = readyCL( csName, clName );
   var recsNum = 10000;
   var expRecs = new Array();

   var imprtFile = tmpFileDir + "22775.csv";
   readyData( imprtFile, recsNum, expRecs );
   importData( csName, clName, imprtFile, recsNum );

   checkCLData( cl, expRecs );
   cleanCL( csName, clName );
}

function readyData ( imprtFile, recsNum, expRecs )
{
   var str = "a int";
   for( var i = 0; i < recsNum; i++ )
   {
      str = str + "\n" + i;

      expRecs.push( { "a": i } );
   }
   var file = fileInit( imprtFile );
   file.write( str );
   file.close();
}

function importData ( csName, clName, imprtFile, recsNum )
{
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
      + ' -c ' + csName + ' -l ' + clName
      + ' --type csv --headerline true --errorstop false -n 3 -j 1000 --parsers 1000'
      + ' --file ' + imprtFile;
   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: " + recsNum;
   var expParseFailure = "Parsed failure: 0";
   var expImportedRecords = "Imported records: " + recsNum;
   var actParseRecords = rcObj[0];
   var actParseFailure = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "[" + expParseRecords + ", " + expParseFailure + ", " + expImportedRecords + "]" + "\n"
         + "[" + actParseRecords + ", " + actParseFailure + ", " + actImportedRecords + "]" );
   }
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl, expRecs )
{
   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "a": 1 } );
   var recsArray = new Array();
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   if( JSON.stringify( recsArray ) !== JSON.stringify( expRecs ) )
   {
      throw new Error( "expRecs: " + JSON.stringify( expRecs ) + ", actRecs: " + JSON.stringify( recsArray ) );
   }
}