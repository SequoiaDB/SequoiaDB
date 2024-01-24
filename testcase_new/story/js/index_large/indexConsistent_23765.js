/******************************************************************************
 * @Description   : seqDB-23765:主表上所有索引复制到主表
 * @Author        : Yi Pan
 * @CreateTime    : 2021.03.29
 * @LastEditTime  : 2021.09.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var indexNames = ["idx23765_1", "idx23765_2", "idx23765_3", "idx23765_4"];
   var csName = "cs_23765";
   var subCLName1 = "subcl_23765_1";

   // 复制所有索引到所有子表，子表不存在相同索引
   testCopyIndex( csName, subCLName1, indexNames, function() { } );

   // 复制所有索引到所有子表，部分子表存在相同索引
   testCopyIndex( csName, subCLName1, indexNames, function()
   {
      db.getCS( csName ).getCL( subCLName1 ).createIndex( indexNames[1], { c: 1 } );
   } );

   // 复制所有索引到所有子表，部分子表存在不同索引
   var indexNames = ["idx23765_1", "idx23765_2", "idx23765_3", "idx23765_4", "idx23765_5"];
   testCopyIndex( csName, subCLName1, indexNames, function()
   {
      db.getCS( csName ).getCL( subCLName1 ).createIndex( indexNames[4], { f: 1 } );
   } );
}

function testCopyIndex ( csName, subCLName1, subcl1IndexNames, func )
{
   var mainCLName = "maincl_23765";
   var subCLName2 = "subcl_23765_2";
   var indexNames = ["idx23765_1", "idx23765_2", "idx23765_3", "idx23765_4"];

   commDropCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { IsMainCL: true, ShardingKey: { a: 1 }, ShardingType: "range" } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );

   // 主表创建索引
   maincl.createIndex( indexNames[0], { b: 1 } );
   maincl.createIndex( indexNames[1], { c: 1 } );
   maincl.createIndex( indexNames[2], { d: 1 } );
   maincl.createIndex( indexNames[3], { e: 1 } );

   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 执行传入的函数
   func();

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { "a": i, "b": i, "c": i, "d": i, "e": i, "f": i } );
   }
   maincl.insert( docs );

   // 复制所有索引到所有子表
   maincl.copyIndex();

   // 校验任务和索引一致性
   checkCopyTask( csName, mainCLName, indexNames, [csName + "." + subCLName1, csName + "." + subCLName2], 0 );
   checkIndexTask( "Create index", csName, mainCLName, indexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName1, subcl1IndexNames, 0 );
   checkIndexTask( "Create index", csName, subCLName2, indexNames, 0 );

   //清除环境
   commDropCS( db, csName );
}