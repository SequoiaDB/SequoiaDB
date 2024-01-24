/******************************************************************************
 * @Description   : seqDB-23759:主表上创建索引，部分子表上存在相同索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23759";
   var mainCLName = "maincl_23759";
   var subCLName1 = "subcl_23759_1";
   var subCLName2 = "subcl_23759_2";
   var indexName1 = "index_23759_1";
   var indexName2 = "index_23759_2";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subcl1 = commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   // 子表创建索引
   subcl1.createIndex( indexName1, { b: 1 } );

   // 主表创建索引 a.索引名相同 索引定义不同
   assert.tryThrow( SDB_IXM_EXIST, function()
   {
      maincl.createIndex( indexName1, { c: 1 } );
   } );
   checkNoTask( "Create index", csName, mainCLName, indexName1 );
   checkIndexExist( db, csName, mainCLName, indexName1, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName1, false );

   // 主表创建索引 b.索引名相同 索引定义相同,校验任务
   maincl.createIndex( indexName1, { b: 1 } );
   checkIndexTask( "Create index", csName, mainCLName, indexName1, 0 );
   checkIndexExist( db, csName, mainCLName, indexName1, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName1, true );

   // 主表创建索引 c.索引名不同 索引定义相同
   assert.tryThrow( SDB_IXM_EXIST_COVERD_ONE, function()
   {
      maincl.createIndex( indexName2, { b: 1 } );
   } );
   checkNoTask( "Create index", csName, mainCLName, indexName2 );
   checkIndexExist( db, csName, mainCLName, indexName2, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName2, false );

   // 主表创建索引 d.索引名不同 索引定义部分相同
   maincl.createIndex( indexName2, { b: 1, c: 1 } );

   // 查看索引任务信息
   checkIndexTask( "Create index", csName, mainCLName, [indexName1, indexName2], 0 );
   checkIndexTask( "Create index", csName, subCLName1, [indexName1, indexName2], 0 );
   checkIndexTask( "Create index", csName, subCLName2, [indexName1, indexName2], 0 );
   checkIndexExist( db, csName, mainCLName, indexName2, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName2, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName2, true );

   commDropCS( db, csName );
}