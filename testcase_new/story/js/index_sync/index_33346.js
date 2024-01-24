/******************************************************************************
 * @Description   : seqDB-33346:创建唯一索引冲突后查看IndexCommitLSN
 * @Author        : liuli
 * @CreateTime    : 2023.09.12
 * @LastEditTime  : 2023.09.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_33346";
testConf.clName = COMMCLNAME + "_33346";
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "index_33346";
   var dbcl = testPara.testCL;
   var srcGroup = testPara.srcGroupName;

   // 插入数据
   insertBulkData( dbcl, 10000 );

   // 插入重复数据
   dbcl.insert( { "a": 1, "c": 1, "test": 1 } );

   // 创建字段存在重复的唯一索引
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.createIndex( indexName, { "a": 1, "c": 1 }, { "Unique": true } );
   } );

   // 校验集合空间快照IndexCommitLSN主备节点保持一致
   checkIndexCommitLSNConsistent();
}

function checkIndexCommitLSNConsistent ()
{
   var doTime = 0;
   var timeOut = 300;
   var objs = [];
   while( doTime <= timeOut )
   {
      objs = [];
      // 校验集合空间IndexCommitLSN主备节点保持一致
      var cursor = db.snapshot( SDB_SNAP_COLLECTIONS, { Name: testConf.csName + "." + testConf.clName, RawData: true },
         { "Details.IndexCommitLSN": 1, "Details.NodeName": 1 } );
      var indexCommitLSNs = [];
      while( cursor.next() )
      {
         var obj = cursor.current().toObj();
         objs.push( obj );
         indexCommitLSNs.push( obj["Details"][0]["IndexCommitLSN"] );
      }
      cursor.close();

      var indexCommitLSNConsistent = true;
      var firstElement = indexCommitLSNs[0];
      for( var i = 1; i < indexCommitLSNs.length; i++ )
      {
         if( indexCommitLSNs[i] !== firstElement )
         {
            indexCommitLSNConsistent = false;
         }
      }

      if( indexCommitLSNConsistent )
      {
         break;
      }

      doTime++;
      sleep( 1000 );
   }

   if( doTime > timeOut )
   {
      throw new Error( "IndexCommitLSN is not consistent ," + JSON.stringify( objs ) );
   }
}