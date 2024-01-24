/******************************************************************************
 * @Description   : seqDB-6663:创建复合键唯一索引，批量修改
 *                   检查CL集合属性及压缩率
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.02.23
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6663";
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

   // 创建索引并插入数据
   cl.createIndex( "idx", { INNER_NO: 1, SA_ACCT_NO: -1 }, true, true );
   insertRecs( cl, insertRecsNum );

   waitDictionary( db, csName, clName );

   // 批量修改数据
   var rc = cl.find( { $and: [{ INNER_NO: { $gte: 300000 } }, { SA_ACCT_NO: { $lt: 700000 } }] } )
      .update( { $set: { QRCODE_STRING: "need update by index" } } );
   while( rc.next() );

   // 检查结果，检查组内每个节点数据正确性
   checkLzwAttributeByDataNode( rgName, csName, clName, true );
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
            var recsCnt = null;
            if( i < 300000 || i >= 700000 )
            {
               // 检查更新前记录
               recsCnt = nodeCL.find( {
                  INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
                  QRCODE_STRING: "need update", SA_OP_ACCT_NO: "6217001820000548390"
               } ).count();
            }
            else
            {
               // 检查更新后记录
               recsCnt = nodeCL.find( {
                  INNER_NO: i, SA_ACCT_NO: i, EVT_ID: "lwy20120702" + i,
                  QRCODE_STRING: "need update by index", SA_OP_ACCT_NO: "6217001820000548390"
               } ).count();
            }
            assert.equal( recsCnt, 1, "i = " + i );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}