/************************************************************************
*@Description:  seqDB-8535:导入decimal类型的json文件
*@Author:           2016-8-3  huangxiaoni
************************************************************************/
testConf.csName = COMMCSNAME;
testConf.clName = COMMCLNAME + "_8535";

main( test );

function test ( testPara )
{
   var imprtFile = tmpFileDir + "8535.json";
   readyData( imprtFile );
   var rcResults = importData( COMMCSNAME, testConf.clName, imprtFile, "json" );
   checkImportRC( rcResults, 8, 8, 1 );

   var expRecs = [{ "a": 2, "b": { "$decimal": "MAX" } }, { "a": 3, "b": { "$decimal": "MIN" } }, { "a": 4, "b": { "$decimal": "MIN" } }, { "a": 5, "b": { "$decimal": "MAX" } }, { "a": 6, "b": { "$decimal": "NaN" } }, { "a": 7, "b": { "$decimal": "MAX" } }, { "a": 8, "b": { "$decimal": "MIN" } }, { "a": 9, "b": { "$decimal": "NaN" } }];
   var cursor = testPara.testCL.find().sort( { a: 1 } );
   commCompareResults( cursor, expRecs );

   // clean *.rec file
   var tmpRec = testConf.csName + "_" + testConf.clName + "*.rec";
   cmd.run( "rm -rf " + tmpRec );
}

function readyData ( imprtFile )
{
   var file = fileInit( imprtFile );
   file.write( '{ "a": 1, "b": { "$decimal": "1a1" } }' + "\n"
      + '{ "a": 2, "b": { "$decimal": "MAX" } }' + "\n"
      + '{ "a": 3, "b": { "$decimal": "MIN" } }' + "\n"
      + '{ "a": 4, "b": { "$decimal": "-INF" } }' + "\n"
      + '{ "a": 5, "b": { "$decimal": "INF" } }' + "\n"
      + '{ "a": 6, "b": { "$decimal": "NAN" } }' + "\n"
      + '{ "a": 7, "b": { "$decimal": "max" } }' + "\n"
      + '{ "a": 8, "b": { "$decimal": "min" } }' + "\n"
      + '{ "a": 9, "b": { "$decimal": "nan" } }' + "\n" );
   var fileInfo = cmd.run( "cat " + imprtFile );
   file.close();
}
