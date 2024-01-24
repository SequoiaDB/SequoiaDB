/******************************************************************************
 * @Description   : seqDB-6657:remove带条件删除所有数据并再次插入数据
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.07
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6659";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var insertRecsNum = 800000;
   var checkRecsNum = 3;

   // insert  
   insertRecs2( cl, insertRecsNum );
   checkLzwAttributeByDataNode( rgName, csName, clName, true );

   // truncate
   cl.truncate();
   var recsCnt = cl.count();
   assert.equal( recsCnt, 0 );
   checkLzwAttributeByDataNodeAfterTruncate( rgName, csName, clName );

   // insert again
   insertRecs2( cl, insertRecsNum );

   waitDictionary( db, csName, clName );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum, checkRecsNum );
}

function checkLzwAttributeByDataNodeAfterTruncate ( rgName, csName, clName )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );
         var clInfo = nodeDB.snapshot( 4, { Name: csName + "." + clName } ).toArray();
         var details = JSON.parse( clInfo[0] ).Details[0];
         assert.equal( details.Attribute, "Compressed", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.CompressionType, "lzw", "clInfo = " + JSON.stringify( clInfo ) );
         assert.equal( details.DictionaryCreated, false, "clInfo = " + JSON.stringify( clInfo ) );
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}

function checkRecsByDataNode ( rgName, csName, clName, insertRecsNum, checkRecsNum )
{
   var rc = db.exec( "select NodeName from $SNAPSHOT_SYSTEM where GroupName='" + rgName + "'" );
   while( rc.next() )
   {
      var nodeName = rc.current().toObj()["NodeName"];
      var nodeDB = null;
      try
      {
         nodeDB = new Sdb( nodeName );
         var nodeCL = nodeDB.getCS( csName ).getCL( clName );
         // 检查数据总数
         var recsCnt = nodeCL.count();
         assert.equal( recsCnt, insertRecsNum );
         // 随机检查n条记录正确性
         for( j = 0; j < checkRecsNum; j++ )
         {
            var i = parseInt( Math.random() * insertRecsNum );
            var recsCnt = nodeCL.find( {
               INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
               IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行"
            } ).count();
            assert.equal( recsCnt, 1 );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}