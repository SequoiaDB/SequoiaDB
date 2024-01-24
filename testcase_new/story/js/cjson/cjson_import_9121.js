/************************************************************************
*@Description:   seqDB-9121:导入数据为inf/-inf/Infinity/ -Infinity
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9121";
main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9121.json";
   var srcDatas = "{oldmin:-inf}\n{oldmax:inf}\n{min:-Infinity}\n{max:Infinity}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 0;
   var importRes = 4;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check the import result 
   var expRecs = '[{"oldmin":-Infinity},{"oldmax":Infinity},{"min":-Infinity},{"max":Infinity}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

