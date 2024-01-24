/******************************************************************************
 * @Description   : seqDB-23764:指定索引名异步复制索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.12.10
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName = "cs_23764";
   var mainCLName = "maincl_23764";
   var subCLName1 = "subcl_23764_1";
   var subCLName2 = "subcl_23764_2";
   var subCLName3 = "subcl_23764_3";
   var subCLName4 = "subcl_23764_4";
   var indexName = "index_23764";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName3, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName4, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );

   //主表创建索引，此时子表1存在索引，子表2、3、4不存在索引
   maincl.createIndex( indexName, { c: 1 } );

   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName3, { LowBound: { a: 1000 }, UpBound: { a: 1500 } } );
   maincl.attachCL( csName + "." + subCLName4, { LowBound: { a: 1500 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 执行异步copy索引
   var taskid = maincl.copyIndexAsync( "", indexName );

   // 校验任务信息
   db.waitTasks( taskid );
   var subCLNames = [csName + "." + subCLName2, csName + "." + subCLName3, csName + "." + subCLName4];
   checkCopyTask( csName, mainCLName, indexName, subCLNames, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName3, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName4, indexName, 0 );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName3, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName4, indexName, true );

   // 数据查询并查看访问计划
   var num = parseInt( Math.random() * 2000 );
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: num }, "ixscan", indexName );

   commDropCS( db, csName );
}