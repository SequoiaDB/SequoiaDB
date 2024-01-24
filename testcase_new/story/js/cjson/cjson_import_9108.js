/************************************************************************
*@Description:   seqDB-9108:导入非法date类型数据:
                                 a、函数值不满足要求，如 {date1:SdbDate2015-06-05)}
                                 b、函数值格式不正确，如{date2:SdbDate("1433492413"}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9106";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9108.json";
   var srcDatas = "{date1:SdbDate2015-06-05)}\n{date2:SdbDate('1433492413'}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

