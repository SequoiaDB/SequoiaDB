/******************************************************************************
 * @Description   : seqDB-33382:重复创建id索引后查看IndexCommitLSN
 * @Author        : liuli
 * @CreateTime    : 2023.09.15
 * @LastEditTime  : 2023.09.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_33382";
testConf.clName = COMMCLNAME + "_33382";
testConf.clOpt = { AutoIndexId: false };
testConf.useSrcGroup = true;

main( test );
function test ( testPara )
{
   var dbcl = testPara.testCL;

   // 插入数据
   insertBulkData( dbcl, 1000 );

   dbcl.createIdIndex();

   // 再次插入数据
   insertBulkData( dbcl, 1000 );

   // 重复创建id索引
   dbcl.createIdIndex();

   // 校验集合空间快照IndexCommitLSN主备节点保持一致
   checkIndexCommitLSNConsistent();

   // 删除id索引
   dbcl.dropIdIndex();

   // 重复删除id索引
   dbcl.dropIdIndex();

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