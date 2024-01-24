/************************************************************************
*@Description:   seqDB-9092:  json类型字段值为空
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9092";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );
   cmd.run( 'rm -rf ./*.rec' );

   //import datas          
   var imprtFile = tmpFileDir + "9092.json";
   var srcDatas = "{a:}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'\' is not defined"';
   var expLogInfo = "ReferenceError: \'\' is not defined";
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}