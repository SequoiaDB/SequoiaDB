/******************************************************************************
 * @Description   : seqDB-23757:主表创建/删除普通索引
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.22
 * @LastEditors   : liuli
 ******************************************************************************/
// 创建索引不指定NotArray
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23757a";
   var mainCLName = "maincl_23757a";
   var subCLName1 = "subcl_23757a_1";
   var subCLName2 = "subcl_23757a_2";
   var idxName = "index_23757a";

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 主表创建索引
   maincl.createIndex( idxName, { a: 1, c: 1 } );

   // 校验索引任务信息
   checkIndexTask( "Create index", csName, mainCLName, idxName, 0 );
   checkIndexTask( "Create index", csName, subCLName1, idxName, 0 );
   checkIndexTask( "Create index", csName, subCLName2, idxName, 0 );

   // 校验索引一致性
   checkIndexExist( db, csName, mainCLName, idxName, true );
   commCheckIndexConsistent( db, csName, subCLName1, idxName, true );
   commCheckIndexConsistent( db, csName, subCLName2, idxName, true );

   // 插入数据
   var docs = insertBulkData( maincl, 2000 );

   // 校验数据并查看访问计划
   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { a: 5, c: 5 }, "ixscan", idxName );

   // 主表删除索引
   maincl.dropIndex( idxName );

   // 查看索引任务信息
   checkIndexTask( "Drop index", csName, mainCLName, idxName, 0 );
   checkIndexTask( "Drop index", csName, subCLName1, idxName, 0 );
   checkIndexTask( "Drop index", csName, subCLName2, idxName, 0 );

   // 校验主备节点不存在索引
   checkIndexExist( db, csName, mainCLName, idxName, false );
   commCheckIndexConsistent( db, csName, subCLName1, idxName, false );
   commCheckIndexConsistent( db, csName, subCLName2, idxName, false );

   var actResult = maincl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
   checkExplainByMaincl( maincl, { a: 5, c: 5 }, "ixscan", "$shard" );

   //清除环境
   commDropCS( db, csName );
}