/************************************************************************
*@Description:   seqDB-9093: json类型字段名和字段值为空
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9093";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9093.json";
   var srcDatas = "{:}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: invalid key"';
   var expLogInfo = 'Syntax Error: invalid key';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf ./*.rec' );
   removeTmpDir();
}