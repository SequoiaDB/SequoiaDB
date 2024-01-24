/************************************************************************
*@Description: seqDB-9626:导入JSON文件数据，文件内容为非json格式
*@Author: 2019-11-26 Zhao xiaoni Init
************************************************************************/

testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_9626";

main( test );

function test ( testPara )
{
   var imprtFile = tmpFileDir + "9626.json";
   readyData( imprtFile );
   var rcResults = importData( testConf.csName, testConf.clName, imprtFile, "json" );
   checkImportRC( rcResults, 0, 0, 1 );
   checkCLData( testPara.testCL, 0, "[]" );

   // clean *.rec file
   var tmpRec = testConf.csName + "_" + testConf.clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function readyData ( imprtFile )
{
   var file = fileInit( imprtFile );
   file.write( "abc\n" );
   file.write( "[1, 2, 3]\n" );
   file.write( "123" );
   file.close();
}
