/************************************************************************
*@Description:   seqDB-9096: 字段值使用单引号
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9096";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9096.json";
   var srcDatas = "{a:'123','b':'test01'}\n{time:{ '$timestamp': '2012-01-01-13.14.26.124233'}}\n{reg:Regex('^W','i')}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"a":"123","b":"test01"},{"time":{"$timestamp":"2012-01-01-13.14.26.124233"}},{"reg":{"$regex":"^W","$options":"i"}}]';
   checkCLData( cl, expRecs );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}