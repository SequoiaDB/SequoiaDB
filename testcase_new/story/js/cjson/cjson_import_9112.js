/************************************************************************
*@Description:   seqDB-9112:导入非法BinDate类型数据:
                                 a、缺少数据或类型，如bin:BinData("aGVsbG8gd29ybGQ=" )}、bin:BinData("1" )}
                                 b、函数值格式不正确，如{reg:Regex(’^W‘, "i"，"i")}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9112";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9112a.json";
   var srcDatas = "{bin:BinData('aGVsbG8gd29ybGQ=' )}\n{bin:BinData('1' )}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check {date1:SdbDate2015-06-05)} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Not enough arguments in call to function"';
   var expLogInfo = 'Not enough arguments in call to function';
   checkSdbimportLog( matchInfos, expLogInfo );

   //-------------test b：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9112b.json";
   var srcDatas = "{bin:BinData('aGVsbG8gd29ybGQ=11111', '1' )}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "The \'bin\' value binary format error"';
   var expLogInfo = 'The \'bin\' value binary format error';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

