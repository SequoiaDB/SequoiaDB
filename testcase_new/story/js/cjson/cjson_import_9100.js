/************************************************************************
*@Description:   seqDB-9100: 导入OID类型数据合法（支持两种格式）:ObjectId()、ObjectId（24字节16进制字符串））
*@Author:        2016-7-20  wuyan
************************************************************************/
var clName = COMMCLNAME + "_9100";

main( test );
function test ()
{
   var cl = readyCL( COMMCSNAME, clName );

   //import datas          
   var imprtFile = tmpFileDir + "9100.json";
   var srcDatas = "{_id:ObjectId('55713f7953e6769804000001')}\n{_id:ObjectId()}"
   importData( COMMCSNAME, clName, imprtFile, srcDatas );

   //check the import result    	
   checkCLDatas( cl );

   commDropCL( db, COMMCSNAME, clName );
   removeTmpDir();
}

function checkCLDatas ( cl, expRecs )
{
   var actIdCnt = cl.find( { "_id": { "$oid": "55713f7953e6769804000001" } } ).count();

   var rc = cl.find();
   var actCnt = cl.find().count();
   var expCnt = 2;
   var extIdCnt = 1;

   if( actIdCnt != extIdCnt )
   {
      throw new Error( "actIdCnt: " + actIdCnt + "\nextIdCnt: " + extIdCnt );
   }


   if( actCnt != expCnt )
   {
      throw new Error( "actCnt: " + actCnt + "\nexpCnt: " + expCnt );
   }
}