/************************************************************************
*@Description:   seqDB-9118:导入非法NumberLong类型数据:
                                 a、函数值不满足要求，如{number:NumberLong(abc123)} 
                                 b、函数值格式不正确，如{number:NumberLong(123,234)}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9118";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9118.json";
   var srcDatas = "{number:NumberLong(abc123)}\n{number:NumberLong(123,234)}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check {number:NumberLong(abc123)} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "ReferenceError: \'abc123\' is not defined"';
   var expLogInfo = 'ReferenceError: \'abc123\' is not defined';
   checkSdbimportLog( matchInfos, expLogInfo );

   //check {number:NumberLong(123,234)} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "Failed to parse the No. 1 argument"';
   var expLogInfo = 'Failed to parse the No. 1 argument';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

