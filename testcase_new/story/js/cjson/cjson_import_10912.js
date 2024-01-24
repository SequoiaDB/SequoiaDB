/************************************************************************
*@Description:   seqDB-10912:导入NumberLong类型超过最大值
*@Author:        2017-1-5  wuyan
************************************************************************/
var clName = COMMCLNAME + "_10912";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "10912.json";
   var srcDatas = "{a:{'$numberLong':'-9223372036854775809'}}\n{c:{'$numberLong':'9223372036854775808'}}\n"
      + "{b:{'$numberLong':'-9223372036854775808'}}\n{d:{'$numberLong':'9223372036854775807'}}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the Return Infos of the import datas
   var parseFail = 2;
   var importRes = 2;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check the import result 
   var expRecs = '[{"b":{"$numberLong":"-9223372036854775808"}},{"d":{"$numberLong":"9223372036854775807"}}]';
   checkCLData( cl, expRecs );

   //check {a:{'$numberLong':'-9223372036854775809'}} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "the \'-9223372036854775809\' is out of the range of numberLong"';
   var expLogInfo = 'Failed to read numberLong, the \'-9223372036854775809\' is out of the range of numberLong';
   checkSdbimportLog( matchInfos, expLogInfo );

   //check {c:{'$numberLong':'9223372036854775808'}} error of sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "the \'9223372036854775808\' is out of the range of numberLong"';
   var expLogInfo = 'Failed to read numberLong, the \'9223372036854775808\' is out of the range of numberLong';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

