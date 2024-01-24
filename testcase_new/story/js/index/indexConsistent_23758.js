/******************************************************************************
 * @Description   : seqDB-23758:主表异步创建/删除唯一索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23758";
   var mainCLName = "maincl_23758";
   var subCLName1 = "subcl_23758_1";
   var subCLName2 = "subcl_23758_2";
   var indexName = "index_23758";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   var docs = insertBulkData( maincl, 1000 );

   // 主表异步创建索引
   maincl.createIndexAsync( indexName, { a: 1, c: 1 }, { 'Unique': true, 'Enforced': true } );

   // 等待任务结束并校验
   waitTaskFinish( csName, mainCLName, "Create index" );
   checkIndexTask( "Create index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );

   // 主表插入索引重复数据
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      maincl.insert( { a: 5, c: 5 } );
   } );

   // 校验数据并查询访问计划
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { a: 5, c: 5 }, "ixscan", indexName );

   // 主表异步删除索引
   maincl.dropIndexAsync( indexName );

   // 等待任务结束并校验
   waitTaskFinish( csName, mainCLName, "Drop index" );
   checkIndexTask( "Drop index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, false );

   // 查询数据并校验访问计划
   actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { a: 5, c: 5 }, "ixscan", "$shard" );

   commDropCS( db, csName );
}