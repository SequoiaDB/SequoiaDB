/******************************************************************************
 * @Description   : seqDB-23744:分区表创建唯一索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.02.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.csName = COMMCSNAME + "_23744";
testConf.clName = COMMCLNAME + "_23744";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true };

main( test );
function test ( testPara )
{
   var csName = testConf.csName;
   var clName = testConf.clName;
   var indexName = "index_23744_1";
   var cl = testPara.testCL;

   //1.插入数据
   var docs = insertBulkData( cl, 100 );
   cl.insert( { "a": 1, "c": 1, "test": 1 } );

   //2.创建唯一索引，存在重复记录创建失败
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      cl.createIndex( indexName, { "a": 1, "c": 1 }, { "Unique": true, "Enforced": true } );
   } );

   //3.检查任务和索引
   checkIndexTaskResult( "Create index", csName, clName, indexName, SDB_IXM_DUP_KEY );
   commCheckBusinessStatus( db, 180 );
   commCheckIndexConsistent( db, csName, clName, indexName, false );

   //4.删除重复记录
   cl.remove( { "a": 1, "c": 1, "test": 1 } )

   //5.再次创建相同索引
   cl.createIndex( indexName, { "a": 1, "c": 1 }, { "Unique": true, "Enforced": true } );

   //6.查看索引任务信息
   checkIndexTaskResult( "Create index", csName, clName, indexName, 0 );
   commCheckIndexConsistent( db, csName, clName, indexName, true );

   //7.查询数据，查看访问计划
   var actResult = cl.find().sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( actResult, docs );
   checkExplain( cl, { "a": 5, "c": 5 }, "ixscan", indexName );
}