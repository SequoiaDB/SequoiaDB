/************************************************************************
*@Description:   seqDB-9098: 字段值末尾存在多余逗号
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9098";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9098.json";
   var srcDatas = "{a:[1,2,3,],b:{a:{a:1,}}}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"a":[1,2,3],"b":{"a":{"a":1}}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}