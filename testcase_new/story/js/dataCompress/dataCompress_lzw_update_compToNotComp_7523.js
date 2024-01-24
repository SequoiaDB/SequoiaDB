/******************************************************************************
 * @Description   : seqDB-7523:记录已压缩，更新记录为不压缩
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_7523";
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
   insertRecs( cl, insertRecsNum );

   // findAndUpdate
   var rc = cl.find( { INNER_NO: { $gte: 300000 } } ).
      update( { $replace: { "电子银行业务回单(付款)": "中国民生银行福州闽江支行" } } );
   while( rc.next() );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, false );
   checkRecsByDataNode( rgName, csName, clName, insertRecsNum, checkRecsNum );
}

function insertRecs ( cl, insertRecsNum )
{

   for( k = 0; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( {
            INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
            QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390"
         } )
      };
      cl.insert( doc );
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
            if( i < 300000 )
            {  // 检查更新前的记录
               var recsCnt = nodeCL.find( {
                  INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
                  QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390"
               } ).count();
               var expctCnt = 1;
            }
            else
            {  // 检查更新后的记录
               var recsCnt = nodeCL.find( { "电子银行业务回单(付款)": "中国民生银行福州闽江支行" } ).count();
               var expctCnt = insertRecsNum - 300000;
            }
            assert.equal( recsCnt, expctCnt );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}