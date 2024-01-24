/************************************************************************
*@Description:   seqDB-9120:导入数据超过double类型边界值，导入成功后显示为-Infinity和Infinity:
                                  a、小于-1.7E+308，如-1.8E+308 
                                  b、大于1.7E+308，如1.8E+308
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9120";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas
   var imprtFile = tmpFileDir + "9120.json";
   var srcDatas = "{a:-1.8E+308}\n{b:1.8E+308}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 0;
   var importRes = 2;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check the import result
   var expRecs = '[{"a":{"$decimal":"-180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}},{"b":{"$decimal":"180000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

