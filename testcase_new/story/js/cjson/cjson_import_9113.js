/************************************************************************
*@Description:   seqDB-9113:导入MinKey类型数据合法
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9113";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9113.json";
   var srcDatas = "{key:MinKey()}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"key":{"$minKey":1}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

