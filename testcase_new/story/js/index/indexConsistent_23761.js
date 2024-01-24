/******************************************************************************
 * @Description   : seqDB-23761:子表在多个组上，主表创建索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var csName = "cs_23761";
   var mainCLName = "maincl_23761";
   var subCLName1 = "subcl_23761_1";
   var subCLName2 = "subcl_23761_2";
   var indexName = "index_23761";

   var dataGroupNames = commGetDataGroupNames( db );

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   var subcl1 = commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", Group: dataGroupNames[0] } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 500 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 500 }, UpBound: { a: 1000 } } );

   // 主表插入数据
   var docs = insertBulkData( maincl, 1000 );

   // 子表1切分到两个组
   subcl1.split( dataGroupNames[0], dataGroupNames[1], 50 );

   // 主表创建索引
   maincl.createIndex( indexName, { c: 1 } );

   // 校验任务和索引一致性
   checkIndexTask( "Create index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, true );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, true );

   // 数据查询查看访问计划
   var actResult = maincl.find().sort( { a: 1 } ).hint( { "": indexName } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: 5 }, "ixscan", indexName );

   // 删除索引并校验
   maincl.dropIndex( indexName );
   checkIndexTask( "Drop index", csName, mainCLName, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName1, indexName, 0 );
   checkIndexTask( "Drop index", csName, subCLName2, indexName, 0 );
   checkIndexExist( db, csName, mainCLName, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName1, indexName, false );
   commCheckIndexConsistent( db, csName, subCLName2, indexName, false );

   // 数据查询查看访问计划
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { c: 5 }, "tbscan" );

   //清除环境
   commDropCS( db, csName );
}