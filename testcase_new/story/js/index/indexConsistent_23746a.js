/******************************************************************************
 * @Description   : seqDB-23746:切分表创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
// 创建索引不指定NotArray参数
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_23743a";
testConf.clOpt = { ShardingKey: { c: 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( args )
{
   var clName = COMMCLNAME + "_23743a";
   var indexName = "index_23746a";
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;
   var dstGroup = args.dstGroupNames;

   // 插入数据,执行切分
   var docs = insertBulkData( dbcl, 5000 );
   dbcl.split( srcGroup, dstGroup[0], { "c": 100 }, { "c": { "$maxKey": 1 } } );

   // 创建索引
   dbcl.createIndex( indexName, { 'a': 1 } );

   // 检查操作结果
   checkIndexTask( "Create index", COMMCSNAME, clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, true );

   // 查询数据原组范围并查看访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { 'a': 5 }, "ixscan", indexName );

   // 查询数据目标组范围并查看访问计划
   checkExplain( dbcl, { 'a': 105 }, "ixscan", indexName );

   // 删除索引
   dbcl.dropIndex( indexName );

   // 检查主备节点信息
   checkIndexTask( "Drop index", COMMCSNAME, clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, clName, indexName, false );

   // 查询数据原组范围并查看访问计划
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { 'a': 5 }, "tbscan" );

   // 查询数据目标组范围并查看访问计划
   checkExplain( dbcl, { 'a': 105 }, "tbscan" );
}