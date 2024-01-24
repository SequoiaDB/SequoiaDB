/******************************************************************************
 * @Description   : seqDB-6652:构建字典后，插入记录中大部分子串在字典中都能匹配到
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2023.02.08
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6652";
testConf.clOpt = { Compressed: true, CompressionType: "lzw", ReplSize: 0 };

main( test );
function test ( testPara )
{
   var rgName = testPara.srcGroupName;
   var csName = COMMCSNAME;
   var clName = testConf.clName;
   var cl = testPara.testCL;
   var number1 = 600000;
   var insertRecsNum = 1000000;
   var checkRecsNum = 10;

   // insert  
   insertRecs( cl, number1, insertRecsNum );

   // 等待字典构建
   waitDictionary( db, csName, clName );

   // 再次插入少量数据使数据压缩
   var insertRecsNum2 = 50000;
   insertRecs( cl, insertRecsNum2, insertRecsNum2 );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
   checkRecsByDataNode( rgName, csName, clName, number1, insertRecsNum + insertRecsNum2, checkRecsNum, insertRecsNum );
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
            total_exCount: i, exCount_id: i, tx_number: "testR" + i,
            tx_info: "hello/565bf18964f4f14fea84341b/image/20160101_1.png"
         } )
      };
      cl.insert( doc );
   }
}

function checkRecsByDataNode ( rgName, csName, clName, number1, insertRecsNum, checkRecsNum, insertRange )
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
            var i = parseInt( Math.random() * insertRange );

            if( i < number1 )
            {
               var cond = {
                  total_account: i, account_id: i, tx_number: "test" + i,
                  tx_info: "xzposs/565bf18944f4f14fea84341b/image/2016_1.png"
               };
            }
            else
            {
               var cond = {
                  total_exCount: i, exCount_id: i, tx_number: "testR" + i,
                  tx_info: "hello/565bf18964f4f14fea84341b/image/20160101_1.png"
               };
            }
            var recsCnt = nodeCL.find( cond ).count();
            if( recsCnt != 1 && recsCnt != 2 )
            {
               throw new Error( "expected result is 1 or 2, actual is " + recsCnt + " ,cond is :" + JSON.stringify( cond ) );
            }
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}