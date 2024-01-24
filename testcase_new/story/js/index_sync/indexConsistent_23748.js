/******************************************************************************
 * @Description   : seqDB-23748:切分表创建唯一索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.02.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.clName = COMMCLNAME + "_23748";
testConf.clOpt = { ShardingKey: { "a": 1 }, ShardingType: "hash" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( testPara )
{
   var indexName = "index_23748";
   var srcGroup = testPara.srcGroupName;
   var dstGroup = testPara.dstGroupNames;
   var cl = testPara.testCL;

   // 插入数据,执行切分
   var docs = insertBulkData( cl, 1000 );
   cl.split( srcGroup, dstGroup[0], { Partition: 0 }, { Partition: 150 } );

   // 插入重复数据
   cl.insert( { "a": 1, "c": 1, "test": 1 } );

   // 创建字段存在重复的唯一索引
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.createIndex( indexName, { "a": 1, "c": 1 }, { "Unique": true, "Enforced": true } );
   } );

   // 校验创建索引失败任务
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, indexName, SDB_IXM_DUP_KEY );
   commCheckBusinessStatus( db, 180 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );

   // 删除重复记录，再次创建相同索引
   cl.remove( { "a": 1, "c": 1, "test": 1 } )
   cl.createIndex( indexName, { "a": 1, "c": 1 }, { "Unique": true, "Enforced": true } );

   // 检查任务和索引一致性
   checkIndexTaskResult( "Create index", COMMCSNAME, testConf.clName, indexName, 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, true );

   // 查询数据，查看访问计划
   var actResult = cl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplain( cl, { "a": 5, "c": 5 }, "ixscan", indexName );
}
