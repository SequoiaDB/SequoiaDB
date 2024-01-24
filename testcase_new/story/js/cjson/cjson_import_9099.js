/************************************************************************
*@Description:   seqDB-9099: 字段名/字段值中间存在多余逗号
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9099";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9099.json";
   var srcDatas = "{a:[1,2,,4],b:{test:{d:1,,c:2}}}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Syntax Error: extra \',\'"';
   var expLogInfo = 'Syntax Error: extra \',\'';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf ./*.rec' );
   removeTmpDir();

}