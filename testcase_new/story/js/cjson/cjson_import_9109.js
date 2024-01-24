/************************************************************************
*@Description:   seqDB-9109:导入regex类型数据合法
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9109";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9109.json";
   var srcDatas = "{reg:Regex('^W', 'i')}\n{reg1:Regex('^张', 'i')}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"reg":{"$regex":"^W","$options":"i"}},{"reg1":{"$regex":"^张","$options":"i"}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

