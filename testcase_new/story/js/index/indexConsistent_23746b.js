/******************************************************************************
 * @Description   : seqDB-23746:切分表创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
// 创建索引指定NotArray为true
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_23743b";
testConf.clOpt = { ShardingKey: { b: 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( args )
{
   var clName = COMMCLNAME + "_23743b";
   var indexName = "index_23746b_1";
   var indexName2 = "index_23746b_2";
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;
   var dstGroup = args.dstGroupNames;

   // 插入数据,执行切分
   var docs = [];
   for( var i = 0; i < 5000; i++ )
   {
      docs.push( { "a": i, "b": i, c: [i] } );
   }
   dbcl.insert( docs );
   dbcl.split( srcGroup, dstGroup[0], { "b": 100 }, { "b": { "$maxKey": 1 } } );

   // 创建索引指定NotArray为true，索引字段数据为非数组
   dbcl.createIndex( indexName, { "a": 1 }, { "NotArray": true } );

   // 检查操作结果
   checkIndexTask( "Create index", COMMCSNAME, clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, true );

   // 查询数据原组范围并查看访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { "a": 5 }, "ixscan", indexName );

   // 查询数据目标组范围并查看访问计划
   checkExplain( dbcl, { "a": 105 }, "ixscan", indexName );

   // 删除索引
   dbcl.dropIndex( indexName );

   // 检查主备节点信息
   checkIndexTask( "Drop index", COMMCSNAME, clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, false );

   // 查询数据原组范围并查看访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { "a": 5 }, "tbscan" );

   // 查询数据目标组范围并查看访问计划
   checkExplain( dbcl, { "a": 105 }, "tbscan" );

   // 创建索引指定NotArray为true，索引字段数据为数组
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      dbcl.createIndex( indexName2, { "c": 1 }, { "NotArray": true } );
   } );

   // 校验任务和索引
   checkIndexTaskResult( "Create index", COMMCSNAME, clName, indexName2, SDB_IXM_KEY_NOT_SUPPORT_ARRAY );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName2, false );
}