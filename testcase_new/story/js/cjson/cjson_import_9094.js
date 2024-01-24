/************************************************************************
*@Description:   seqDB-9094:  json类型符号不完整 验证情况如下：
                                 a、缺少}或{
                                 b、数组缺少[或]，如{a：[1，2，3} 
                                 c、字符串缺少左“或右”,如{a:"test01} 
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9094";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //---------------------test a：loss “}”-------------------------
   //import datas          
   var imprtFile1 = tmpFileDir + "9094a.json";
   var srcDatas1 = "{a:'test'"
   var rcInfos1 = importData( COMMCSNAME, clName, imprtFile1, srcDatas1 );

   //check the Return Infos of the import datas
   var parseFail1 = 1;
   var importRes1 = 0;
   checkImportReturn( rcInfos1, parseFail1, importRes1 );

   //check sdbimport.log 
   var matchInfos1 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: JSON key \'a\' missing \',\' or \'\}\'"';
   var expLogInfo1 = 'Syntax Error: JSON key \'a\' missing \',\' or \'\}\'';
   checkSdbimportLog( matchInfos1, expLogInfo1 );

   //---------------------test a：loss “{” -------------------------
   //import datas          
   var imprtFile2 = tmpFileDir + "9094a2.json";
   var srcDatas2 = "b:'test'}"
   var rcInfos2 = importData( COMMCSNAME, clName, imprtFile2, srcDatas2 );

   //check the Return Infos of the import datas
   var parseFail2 = 1;
   var importRes2 = 0;
   checkImportReturn( rcInfos2, parseFail2, importRes2 );

   //check sdbimport.log 
   var matchInfos2 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: JSON is missing \'{\'"';
   var expLogInfo2 = 'Syntax Error: JSON is missing \'{\'';
   checkSdbimportLog( matchInfos2, expLogInfo2 );


   //---------------------test b:array loss “]”-------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9094b.json";
   var srcDatas = "{a:[1,2,3}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: JSON array missing \',\' or \']\'"';
   var expLogInfo = 'Syntax Error: JSON array missing \',\' or \']\'';
   checkSdbimportLog( matchInfos, expLogInfo );

   //---------------------test b:array loss “[” -------------------------
   //import datas          
   var imprtFile3 = tmpFileDir + "9094b2.json";
   var srcDatas3 = "{a:1,2,3]}"
   var rcInfos3 = importData( COMMCSNAME, clName, imprtFile3, srcDatas3 );

   //check the Return Infos of the import datas
   var parseFail3 = 1;
   var importRes3 = 0;
   checkImportReturn( rcInfos3, parseFail3, importRes3 );

   //check sdbimport.log 
   var matchInfos3 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: \'2,3]}\' missing \':\'"';
   var expLogInfo3 = 'Syntax Error: \'2,3]}\' missing \':\'';
   checkSdbimportLog( matchInfos3, expLogInfo3 );

   //---------------------test c:string loss “ -------------------------
   //import datas          
   var imprtFile4 = tmpFileDir + "9094c.json";
   var srcDatas4 = "{a:test0a'}"
   var rcInfos4 = importData( COMMCSNAME, clName, imprtFile4, srcDatas4 );

   //check the Return Infos of the import datas
   var parseFail4 = 1;
   var importRes4 = 0;
   checkImportReturn( rcInfos4, parseFail4, importRes4 );

   //check sdbimport.log 
   var matchInfos4 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'test0a\'\' is not defined"';
   var expLogInfo4 = 'ReferenceError: \'test0a\'\' is not defined';
   checkSdbimportLog( matchInfos4, expLogInfo4 );

   //---------------------test c:string loss " -------------------------
   //import datas          
   var imprtFile5 = tmpFileDir + "9094c2.json";
   var srcDatas5 = "{a:'test0a}"
   var rcInfos5 = importData( COMMCSNAME, clName, imprtFile5, srcDatas5 );

   //check the Return Infos of the import datas
   var parseFail5 = 1;
   var importRes5 = 0;
   checkImportReturn( rcInfos5, parseFail5, importRes5 );

   //check sdbimport.log 
   var matchInfos5 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: JSON string is missing \'"';
   var expLogInfo5 = 'Syntax Error: JSON string is missing \'';
   checkSdbimportLog( matchInfos5, expLogInfo5 );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}