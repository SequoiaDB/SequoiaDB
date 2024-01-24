/******************************************************************************
 * @Description   : seqDB-23747:切分表异步创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_23747";
testConf.clOpt = { ShardingKey: { c: 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "index_23747";
   var srcGroup = testPara.srcGroupName;
   var dstGroup = testPara.dstGroupNames;
   var dbcl = testPara.testCL;

   var docs = insertBulkData( dbcl, 1000 );

   // 执行切分
   dbcl.split( srcGroup, dstGroup[0], { "c": 150 }, { "c": { "$maxKey": 1 } } );

   // 异步创建索引
   var taskid = dbcl.createIndexAsync( indexName, { "a": 1 } );
   db.waitTasks( taskid );

   // 校验任务和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );

   // 查询数据，查看访问计划源范围
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( dbcl, { "a": 5 }, "ixscan", indexName );

   // 查看访问计划目标范围
   checkExplain( dbcl, { "a": 105 }, "ixscan", indexName );

   // 异步删除索引
   var taskid = dbcl.dropIndexAsync( indexName );

   // 等待任务结束并校验
   db.waitTasks( taskid );
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );

   // 查看访问计划
   checkExplain( dbcl, { "a": 5 }, "tbscan" );
   checkExplain( dbcl, { "a": 105 }, "tbscan" );
}