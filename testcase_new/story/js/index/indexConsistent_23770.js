/******************************************************************************
 * @Description   : seqDB-23770:重复复制相同索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23770";
   var mainCLName = "maincl_23770";
   var subCLName1 = "subcl_23770_1";
   var subCLName2 = "subcl_23770_2";
   var subCLName3 = "subcl_23770_3";
   var subCLName4 = "subcl_23770_4";
   var indexName1 = "index_23770_1";
   var indexName2 = "index_23770_2";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   maincl.createIndex( indexName1, { c: 1 } )
   var subcl1 = commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var subcl2 = commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var subcl3 = commCreateCL( db, csName, subCLName3, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   var subcl4 = commCreateCL( db, csName, subCLName4, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName3, { LowBound: { a: 1000 }, UpBound: { a: 1500 } } );
   maincl.attachCL( csName + "." + subCLName4, { LowBound: { a: 1500 }, UpBound: { a: 2000 } } );

   // a.索引名相同，索引定义不相同
   subcl4.createIndex( indexName1, { b: 1 } );
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      maincl.copyIndex( "", indexName1 );
   } );
   checkNoTask( "Copy index", csName, mainCLName );
   maincl.detachCL( csName + "." + subCLName4 );

   // b.索引名不同，索引定义相同
   subcl3.createIndex( indexName2, { c: 1 } );
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      maincl.copyIndex( "", indexName1 );
   } );
   checkNoTask( "Copy index", csName, mainCLName );
   maincl.detachCL( csName + "." + subCLName3 );

   // c.索引名不同，索引定义部分相同
   subcl2.createIndex( indexName2, { a: 1, c: 1 } );
   // d.索引名和索引定义都相同
   subcl1.createIndex( indexName1, { c: 1 } );
   maincl.copyIndex();

   //4.检查任务信息,检查一致性
   checkCopyTask( csName, mainCLName, indexName1, csName + "." + subCLName2, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexName1, 0 );
   checkIndexTask( "Create index", csName, subCLName2, [indexName1, indexName2], 0 );
   checkIndexExist( db, csName, mainCLName, indexName1, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName1, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName1, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName2, true );

   // 主表中插入数据
   var docs = insertBulkData( maincl, 1000 );

   // 查询数据
   var actResult = maincl.find().sort( { a: 1 } ).hint( { "": indexName1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: 5 }, "ixscan", indexName1 );

   // 清除环境
   commDropCS( db, csName );
}