/******************************************************************************
 * @Description   : seqDB-23751:切分表事务中创建/删除索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_23751";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var indexName1 = "index_23751a";
   var indexName2 = "index_23751b";
   var cl = testPara.testCL;

   // 开启事务
   db.transBegin();

   // 插入数据
   insertBulkData( cl, 5000 );

   // 创建普通索引和唯一索引
   cl.createIndex( indexName1, { 'b': 1 } );
   cl.createIndex( indexName2, { 'a': 1 }, { "Unique": true, "Enforced": true } );

   // 插入重复数据
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.insert( { "a": 5 } );
   } );

   // 校验任务信息和索引一致性
   checkIndexTask( "Create index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, true );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, true );

   // 查询数据
   var actResult = cl.find();
   commCompareResults( actResult, [] );

   // 开启事务,执行数据操作
   db.transBegin();
   var docs = insertBulkData( cl, 5000 );

   // 删除索引
   cl.dropIndex( indexName1 );
   cl.dropIndex( indexName2 );

   // 提交事务
   db.transCommit();

   // 查询数据
   var actResult = cl.find().sort( { "a": 1 } );
   commCompareResults( actResult, docs );

   // 校验任务和主备节点索引被删除
   checkIndexTask( "Drop index", COMMCSNAME, testConf.clName, [indexName1, indexName2], 0 );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName1, false );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName2, false );
}