/************************************************************************
*@Description:   seqDB-9116:导入非法MaxKey类型数据:
                                 a、缺少表达式或类型，如 {reg1:Regex(,)}、{reg2:Regex(,"i")}
                                 b、函数值格式不正确，如{reg:Regex(’^W‘, "i"，"i")}
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9116";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //-------------test a：--------------------------
   //import datas          
   var imprtFile = tmpFileDir + "9116.json";
   var srcDatas = "{key:MaxKey(123)}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 1;
   var importRes = 0;
   checkImportReturn( rcInfos, parseFail, importRes );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf *.rec' );
   removeTmpDir();
}

