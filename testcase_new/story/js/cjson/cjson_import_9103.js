/************************************************************************
*@Description:   seqDB-9103:导入OID类型数据中函数值长度非法:小于24字节时，导入成功，自动在字符串前补0;超过24字节时导入失败 
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9103";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );
   cmd.run( 'rm -rf ./sdbimport.log' );

   //import datas          
   var imprtFile = tmpFileDir + "9103.json";
   var srcDatas = "{_id:ObjectId('55713f7953e6769804')}\n{_id:ObjectId('55713f7953e67698040000012')}"
   var rcInfos = importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result 
   var expRecs = '[{"_id":{"$oid":"00000055713f7953e6769804"}}]';
   checkClOidData( cl, expRecs );

   var parseFail = 1;
   var importRes = 1;
   checkImportReturn( rcInfos, parseFail, importRes );

   //check sdbimport.log 
   var matchInfos = 'find ./ -maxdepth 1 -name "sdbimport.log" |xargs grep "argument ObjectId must be a string of length 24"';
   var expLogInfo = 'argument ObjectId must be a string of length 24';
   checkSdbimportLog( matchInfos, expLogInfo );

   commDropCL( db, COMMCSNAME, clName );
   cmd.run( 'rm -rf ./*.rec' );
   removeTmpDir();
}

function checkClOidData ( cl, expRecs )
{
   var rc = cl.find();

   var recsArray = [];
   while( rc.next() )
   {
      recsArray.push( rc.current().toObj() );
   }
   //var expRecs = '[{"a":1},{"a":2}]';
   var actRecs = JSON.stringify( recsArray );
   if( actRecs !== expRecs )
   {
      throw new Error( "actRecs: " + actRecs + "\nexpRecs: " + expRecs );
   }
}
