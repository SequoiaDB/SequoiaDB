/******************************************************************************
 * @Description   : seqDB-23771:连续创建/删除相同索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2022.02.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23771";
   var mainCLName = "maincl_23771";
   var subCLName1 = "subcl_23771_1";
   var subCLName2 = "subcl_23771_2";
   var indexName = "index_23771";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   maincl.createIndex( indexName, { c: 1 } );
   var docs = insertBulkData( maincl, 1000 );
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );

   // 删除索引再次创建相同索引
   maincl.dropIndex( indexName );
   checkIndexTask( "Drop index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, false );

   maincl.createIndex( indexName, { c: 1 } );
   checkIndexTask( "Create index", csName, mainCLName, [indexName, indexName], 0 );
   checkIndexTask( "Create index", csName, subCLName1, [indexName, indexName], 0 );
   checkIndexTask( "Create index", csName, subCLName2, [indexName, indexName], 0 );
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );

   //6.查询数据
   var actResult = maincl.find().sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: 5 }, "ixscan", indexName );

   //清除环境
   commDropCS( db, csName );
}