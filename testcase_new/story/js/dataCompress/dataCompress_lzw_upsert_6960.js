/******************************************************************************
 * @Description   : seqDB-6960:upsert指定更新符删除已压缩记录中字段
 * @Author        : XiaoNi Huang
 * @CreateTime    : 2016.03.23
 * @LastEditTime  : 2021.05.20
 * @LastEditors   : XiaoNi Huang
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = CHANGEDPREFIX + "_cl_6960";
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

   // 插入并更新数据
   insertRecs( cl, insertRecsNum );
   cl.upsert( { $unset: { dtest: "abcdefg890abcdefg890abcdefg890" } } );

   // 等待字典构建
   waitDictionary( db, csName, clName );

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
         doc.push( { atest: i, btest: i, ctest: "test" + i, dtest: "abcdefg890abcdefg890abcdefg890" } )
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
            // 检查更新前的记录
            var recsCnt1 = nodeCL.find( {
               atest: i, btest: i, ctest: "test" + i,
               dtest: "abcdefg890abcdefg890abcdefg890"
            } ).count();
            assert.equal( recsCnt1, 0 );
            // 检查更新后的记录
            var recsCnt2 = nodeCL.find( { atest: i, btest: i, ctest: "test" + i } ).count();
            assert.equal( recsCnt2, 1 );
         }
      }
      finally 
      {
         if( nodeDB != null ) nodeDB.close();
      }
   }
}