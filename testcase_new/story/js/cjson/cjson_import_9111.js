/************************************************************************
*@Description:   seqDB-9111:导入BinDate类型数据合法
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9111";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9109.json";
   var srcDatas = "{bin:BinData('aGVsbG8gd29ybGQ=', '1' )}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"bin":{"$binary":"aGVsbG8gd29ybGQ=","$type":"1"}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

