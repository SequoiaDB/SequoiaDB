/*******************************************************************************
*@Description:   seqDB-12797: sdbimprt支持strictfieldnum配置 
*@Author:        2019-3-5  wangkexin
********************************************************************************/

var csvContent1 = 'test|123\ntest1|456' + "\n";
var csvContent2 = 'test|\ntest1|123\ntest2|456' + "\n";
var csvContent3 = 'test|123\ntest1|\ntest2|456' + "\n";
var csvContent4 = 'test|123\ntest1|456\ntest2|' + "\n";

main( test );

function test ()
{
   var csName = COMMCSNAME;
   var clName = COMMCLNAME + "_12797";
   var cl = readyCL( csName, clName );

   //准备数据文件
   var imprtFile1 = tmpFileDir + "12797a.csv";
   readyData( imprtFile1, csvContent1 );
   var imprtFile2 = tmpFileDir + "12797b.csv";
   readyData( imprtFile2, csvContent2 );
   var imprtFile3 = tmpFileDir + "12797c.csv";
   readyData( imprtFile3, csvContent3 );
   var imprtFile4 = tmpFileDir + "12797d.csv";
   readyData( imprtFile4, csvContent4 );

   //1. strictfieldnum=true，字段数与定义一致
   importData( csName, clName, imprtFile1, 2, 0, 2, true );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   //2. strictfieldnum=true，字段数与定义不一致，覆盖不一致出现位置为首部，中部以及尾部
   importData( csName, clName, imprtFile2, 2, 1, 2, true );
   var expRecs = '[{"yourname":"test1","yourid":123},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   importData( csName, clName, imprtFile3, 2, 1, 2, true );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   importData( csName, clName, imprtFile4, 2, 1, 2, true );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   //3. strictfieldnum=false，字段数与定义一致
   importData( csName, clName, imprtFile1, 2, 0, 2, false );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   //4. strictfieldnum=false，字段数与定义不一致，覆盖不一致出现位置为首部，中部以及尾部
   importData( csName, clName, imprtFile2, 3, 0, 3, false );
   var expRecs = '[{"yourname":"test"},{"yourname":"test1","yourid":123},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 3 );

   importData( csName, clName, imprtFile3, 3, 0, 3, false );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1"},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 3 );

   importData( csName, clName, imprtFile4, 3, 0, 3, false );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456},{"yourname":"test2"}]';
   checkCLData( cl, expRecs, 3 );

   //5. 不指定strictfieldnum，字段数与定义一致
   importData( csName, clName, imprtFile1, 2, 0, 2 );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456}]';
   checkCLData( cl, expRecs, 2 );

   //6. 不指定strictfieldnum，字段数与定义不一致，覆盖不一致出现位置为首部，中部以及尾部
   importData( csName, clName, imprtFile2, 3, 0, 3 );
   var expRecs = '[{"yourname":"test"},{"yourname":"test1","yourid":123},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 3 );

   importData( csName, clName, imprtFile3, 3, 0, 3 );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1"},{"yourname":"test2","yourid":456}]';
   checkCLData( cl, expRecs, 3 );

   importData( csName, clName, imprtFile4, 3, 0, 3 );
   var expRecs = '[{"yourname":"test","yourid":123},{"yourname":"test1","yourid":456},{"yourname":"test2"}]';
   checkCLData( cl, expRecs, 3 );

   cleanCL( csName, clName );
}

function readyData ( imprtFile, csvContent )
{

   var file = fileInit( imprtFile );
   file.write( csvContent );
   file.close();
}

function importData ( csName, clName, imprtFile, expParseRecordsNum, expParseFailureNum, expImportedRecordsNum, strictfieldnum )
{

   //remove rec file
   var tmpRec = csName + "_" + clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );

   //import operation
   if( typeof strictfieldnum == "undefined" )
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv '
         + ' --file ' + imprtFile
         + ' --fields "yourname string,yourid int"'
         + ' -e "|"';
   }
   else
   {
      var imprtOption = installDir + 'bin/sdbimprt -s ' + COORDHOSTNAME + ' -p ' + COORDSVCNAME
         + ' -c ' + csName + ' -l ' + clName
         + ' --type csv '
         + ' --file ' + imprtFile
         + ' --fields "yourname string,yourid int"'
         + ' -e "|"'
         + '	--strictfieldnum ' + strictfieldnum;
   }
   var rc = cmd.run( imprtOption );

   //check import results
   var rcObj = rc.split( "\n" );
   var expParseRecords = "Parsed records: " + expParseRecordsNum;
   var expParseFailure = "Parsed failure: " + expParseFailureNum;
   var expImportedRecords = "Imported records: " + expImportedRecordsNum;
   var actParseRecords = rcObj[0];
   var actParseFailure = rcObj[1];
   var actImportedRecords = rcObj[4];
   if( expParseRecords !== actParseRecords
      || expParseFailure !== actParseFailure
      || expImportedRecords !== actImportedRecords )
   {
      throw new Error( "importData fail,[sdbimprt results]" +
         "[" + expParseRecords + ", " + expImportedRecords + "]" +
         "[" + expParseFailure + ", " + actParseFailure + "]" +
         "[" + actParseRecords + ", " + actImportedRecords + "]" );
   }

   // clean tmpRec
   cmd.run( "rm -rf " + tmpRec );
}

function checkCLData ( cl, expRecs, expCnt )
{

   var rc = cl.find( {}, { _id: { $include: 0 } } ).sort( { "yourname": 1 } );//检视：cl中不止一条记录时建议查询使用排序，避免因为顺序问题随机失败1
   var recsArray = [];
   while( tmpRecs = rc.next() )
   {
      recsArray.push( tmpRecs.toObj() );
   }
   rc.close();

   var actCnt = recsArray.length;
   var actRecs = JSON.stringify( recsArray );
   if( actCnt !== expCnt || actRecs !== expRecs )
   {
      throw new Error( "checkCLdata fail,[find]" +
         "[cnt:" + expCnt + ", recs:" + expRecs + "]" +
         "[cnt:" + actCnt + ", recs:" + actRecs + "]" );
   }
   cl.truncate();
}