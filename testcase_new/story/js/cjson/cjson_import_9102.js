/************************************************************************
*@Description:   seqDB-9102:导入数据中函数关键字不正确
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9102";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9102.json";
   var srcDatas = "{_id:aObjectId('55713f7953e6769804000001')}\n{id:ObjectIdbc()}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'aObjectId(\'55713f7953e6769804000001\')\' is not defined"';
   var expLogInfo = 'ReferenceError: \'aObjectId(\'55713f7953e6769804000001\')\' is not defined';
   checkSdbimportLog( matchInfos, expLogInfo );

   var matchInfos1 = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'ObjectIdbc()\' is not defined"';
   var expLogInfo1 = 'ReferenceError: \'ObjectIdbc()\' is not defined';
   checkSdbimportLog( matchInfos1, expLogInfo1 );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf ./*.rec' );
   removeTmpDir();
}


