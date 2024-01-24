/******************************************************************************
 * @Description   : seqDB-7524:记录未压缩，更新记录为压缩
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_7524";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var number1 = 700000;
   var insertRecsNum = 1000000;
   var checkRecsNum = 3;

   // insert
   insertRecs( cl, number1, insertRecsNum );

   // findAndUpdate
   var rc = cl.find( { INNER_NO: { $exists: 1 } } ).
      update( {
         $set: {
            total_account: insertRecsNum,
            tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
         }
      } );
   while( rc.next() );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, false );
   checkRecsByDataNode( rgName, csName, clName, number1, insertRecsNum, checkRecsNum );
}

function insertRecs ( cl, number1, insertRecsNum )
{
   for( k = 0; k < number1; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( {
            total_account: i, account_id: i, tx_number: "test" + i,
            tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
         } )
      };
      cl.insert( doc );
   }

   for( k = number1; k < insertRecsNum; k += 50000 )
   {
      var doc = [];
      for( i = 0 + k; i < 50000 + k; i++ )
      {
         doc.push( {
            INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
            IVC_NAME: "电子银行业务回单(付款)", OPEN_BRANCH_NAME: "中国民生银行福州闽江支行"
         } )
      };
      cl.insert( doc );
   }
}

function checkRecsByDataNode ( rgName, csName, clName, number1, insertRecsNum, checkRecsNum )
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
            var i = parseInt( Math.random() * insertRecsNum + 1 );
            if( i < number1 )
            {  // 检查更新前的记录
               var recsCnt = nodeCL.find(
                  {
                     total_account: i, account_id: i, tx_number: "test" + i,
                     tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
                  } ).count();
               var expctCnt = 1;
            }
            else
            {  // 检查更新后的记录
               var recsCnt = nodeCL.find(
                  {
                     total_account: insertRecsNum,
                     tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
                  } ).count();
               var expctCnt = insertRecsNum - number1;
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