/******************************************************************************
 * @Description   : seqDB-23757:主表创建/删除普通索引
 * @Author        : liuli
 * @CreateTime    : 2021.08.26
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
// 指定NotArray为true
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23757b";
   var mainCLName = "maincl_23757b";
   var subCLName1 = "subcl_23757b_1";
   var subCLName2 = "subcl_23757b_2";
   var indexName = "idx23757b";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 插入数据
   var docs = [];
   for( var i = 0; i < 1500; i++ )
   {
      docs.push( { a: i, b: i, c: [i] } );
   }
   maincl.insert( docs );

   // 创建索引，索引字段非数组
   maincl.createIndex( indexName, { b: 1 }, { "NotArray": true } );

   // 校验索引并校验任务
   checkIndexTask( "Create index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );

   // 校验数据并查看访问计划
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { b: 5 }, "ixscan", indexName );

   // 删除索引后进行校验
   maincl.dropIndex( indexName );
   checkIndexTask( "Drop index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName2, indexName, 0 );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { b: 5 }, "tbscan" );

   // 创建索引，索引字段为数组
   assert.tryThrow( SDB_IXM_KEY_NOT_SUPPORT_ARRAY, function()
   {
      maincl.createIndex( indexName, { c: 1 }, { "NotArray": true } );
   } );

   // 校验结果
   checkIndexTaskResult( "Create index", csName, mainCLName, indexName, SDB_IXM_KEY_NOT_SUPPORT_ARRAY );
   checkIndexTaskResult( "Create index", csName, subCLName1, indexName, SDB_IXM_KEY_NOT_SUPPORT_ARRAY );
   checkIndexTaskResult( "Create index", csName, subCLName2, indexName, SDB_IXM_KEY_NOT_SUPPORT_ARRAY );
   checkIndexExist( db, csName, mainCLName, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, false );
   checkExplainByMaincl( maincl, { c: 5 }, "tbscan" );

   //清除环境
   commDropCS( db, csName );
}