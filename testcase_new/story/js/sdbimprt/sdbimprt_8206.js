/*******************************************************************************
*@Description : seqDB-8206:导入过程中数据额外增加一个字段验证 
*@Modify list : 2020-4-16  Zhao Xiaoni Init
*******************************************************************************/
testConf.clName = COMMCLNAME + "_8206";

main( test );

function test ( testPara )
{
   var importFile = tmpFileDir + "8206.json";
   cmd.run( "rm -rf " + importFile );
   var file = fileInit( importFile );
   file.write( "13434347996, test, 中文测试\n13434347997, test1, 中文测试1" );

   var type = "csv";
   var fields = "acc_no, context, text";
   importData( COMMCSNAME, testConf.clName, importFile, type, fields );

   var cursor = testPara.testCL.find().sort( { "acc_no": 1 } );
   expRecs = [{ "acc_no": 13434347996, "context": "test", "text": "中文测试" },
   { "acc_no": 13434347997, "context": "test1", "text": "中文测试1" }];
   commCompareResults( cursor, expRecs )

   cmd.run( "rm -rf " + importFile );
}
