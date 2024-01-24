/******************************************************************************
 * @Description   : seqDB-23743:分区表异步创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
// 不指定NotArray参数，索引字段覆盖数组和非数组
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23743a";
testConf.clOpt = { ShardingKey: { a: 1 }, AutoSplit: true };

main( test );
function test ( args )
{
   var indexName1 = "index_23743a_1";
   var indexName2 = "index_23743a_2";
   var clName = testConf.clName;
   var dbcl = args.testCL;

   // 插入数据
   var docs = [];
   for( var i = 0; i < 100; i++ )
   {
      docs.push( { a: i, b: i, c: [i] } );
   }
   dbcl.insert( docs );

   // 异步创建索引
   var taskid1 = dbcl.createIndexAsync( indexName1, { "b": 1 } );
   var taskid2 = dbcl.createIndexAsync( indexName2, { "c": 1 } );

   // 等待索引任务结束
   db.waitTasks( taskid1, taskid2 );

   // 校验索引任务信息
   checkIndexTask( "Create index", COMMCSNAME, clName, [indexName1, indexName2], 0 );

   // 检查索引一致性
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName2, true );

   // 校验数据并查看访问计划
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkExplain( dbcl, { "b": 5 }, "ixscan", indexName1 );
   checkExplain( dbcl, { "c": 5 }, "ixscan", indexName2 );

   // 异步删除索引
   var taskid1 = dbcl.dropIndexAsync( indexName1 );
   var taskid2 = dbcl.dropIndexAsync( indexName2 );

   // 等待任务结束
   db.waitTasks( taskid1, taskid2 );

   // 校验索引任务信息
   checkIndexTask( "Drop index", COMMCSNAME, clName, [indexName1, indexName2], 0 );

   // 检测索引校验数据并查看访问计划
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName2, false );
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkExplain( dbcl, { "b": 5 }, "tbscan" );
   checkExplain( dbcl, { "c": 5 }, "tbscan" );
}