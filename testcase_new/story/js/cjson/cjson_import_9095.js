/************************************************************************
*@Description:   seqDB-9095: 字段名使用单引号
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9095";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9095.json";
   var srcDatas = "{'a':123,'b':'testo1'}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"a":123,"b":"testo1"}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}