/************************************************************************
*@Description:   seqDB-5435:指定errorstop为true，即导入失败时中断导入（多线程解析和导入）
*@Author:        2016-7-14  huangxiaoni
************************************************************************/

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_5435_2";
   var cl = readyCL( csName, clName );

   var imprtFile = tmpFileDir + "5435_2.csv";
   var recsNum = 10000;
   var tmpSplice = 100;
   readyData( imprtFile, recsNum, tmpSplice );
   importData( csName, clName, imprtFile, recsNum );

   checkCLData( cl, recsNum );

   cleanCL( csName, clName );
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function readyData ( imprtFile, recsNum, tmpSplice )
{
   var str = "a int, b date";
   for( var i = 0; i < recsNum; i++ )
   {
      str = str + "\n" + i + ", 2020-10-10";
      // add invalid data
      if( i > 0 && i % tmpSplice === 0 )
      {
         str = str + "\n" + i + ", 2020-0-0";
      }
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
      + ' --type csv --headerline true --errorstop true -n 200 --parsers 30 -j 30'
      + ' --file ' + imprtFile;

   var rc = cmd.run( imprtOption );
   var rcObj = rc.split( "\n" );
   var actParseRecords = Number( rcObj[0].split( ":" )[1] );
   var actImportedRecords = Number( rcObj[4].split( ":" )[1] );
   if( actParseRecords >= recsNum || actImportedRecords >= recsNum )
   {
      throw new Error( "\n" + rc + "\nimport recsNum: " + recsNum + ", import not stop when error" );
   }
   cmd.run( "rm -rf " + tmpRec );

}

function checkCLData ( cl, recsNum )
{
   var cnt = cl.count();
   if( Number( cnt ) >= recsNum )
   {
      throw new Error( "cnt: " + cnt + ", expect cnt < " + recsNum );
   }
}