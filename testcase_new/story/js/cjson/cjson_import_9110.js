/************************************************************************
*@Description:   seqDB-9110:导入非法regex类型数据:
                                 a、缺少表达式或类型，如 {reg1:Regex(,)}、{reg2:Regex(,"i")}
                                 b、函数值格式不正确，如{reg:Regex(’^W‘, "i"，"i")}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9110";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9110a.json";
   var srcDatas = "{reg1:Regex(,)}\n{reg2:Regex(,'i')}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check {date1:SdbDate2015-06-05)} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'\' is not defined"';
   var expLogInfo = 'ReferenceError: \'\' is not defined';
   checkSdbimportLog( matchInfos, expLogInfo );

   //-------------test b：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9110b.json";
   var srcDatas = "{reg:Regex('^W','i','i')}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'\' is not defined"';
   var expLogInfo = 'ReferenceError: \'\' is not defined';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

