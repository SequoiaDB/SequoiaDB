/************************************************************************
*@Description:   seqDB-9101:导入OID类型数据中函数值存在空格
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9101";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9101.json";
   var srcDatas = "{_id:ObjectId('55713Z7953 6769804000001')}\n{_id:ObjectId('55713f7953e6769804000 ')}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Function ObjectId argument must be a hex string"';
   var expLogInfo = 'Function ObjectId argument must be a hex string';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf ./*.rec' );
   removeTmpDir();
}


