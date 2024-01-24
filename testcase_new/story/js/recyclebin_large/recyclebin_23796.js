/******************************************************************************
 * @Description   : seqDB-23796:主子表属于不同CS，主表下挂载多个子表，drop主表，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.01
 * @LastEditTime  : 2022.02.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_23796_1";
   var csName2 = "cs_23796_2";
   var clName = "cl_23796";
   var mainCLName = "mainCL_23796";
   var subCLNames = [];
   var shardRanges = [];

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23796" );

   var dbcs = commCreateCS( db, csName1 );
   var mainCL = commCreateCL( db, csName1, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   // 在 csName2 上创建 1000 个子表，并插入数据
   for( var i = 0; i < 50000; i += 100 )
   {
      commCreateCL( db, csName2, clName + "_" + i, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
      mainCL.attachCL( csName2 + "." + clName + "_" + i, { LowBound: { a: i }, UpBound: { a: i + 100 } } );
      subCLNames.push( csName2 + "." + clName + "_" + i );
      shardRanges.push( i );
   }
   shardRanges.push( i );

   var docs = [];
   for( var i = 0; i < 50000; i += 50 )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // 删除主表后进行恢复
   dbcs.dropCL( mainCLName );
   var originName = csName1 + "." + mainCLName;
   var recycleName = getOneRecycleName( db, originName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后对数据进行校验
   var mainCL = db.getCS( csName1 ).getCL( mainCLName );
   var cursor = mainCL.find().sort( { a: 1 } );
   docs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验恢复后主子表对应关系及分区范围
   checkSubCL( csName1, mainCLName, subCLNames, shardRanges );

   //执行 DDL/DML 操作
   mainCL.insert( { a: 21 } );
   db.getCS( csName2 ).dropCL( clName + "_" + 0 );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23796" );
}

function sortBy ( field )
{
   return function( a, b )
   {
      return a[field] > b[field];
   }
}