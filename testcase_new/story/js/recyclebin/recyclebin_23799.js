/******************************************************************************
 * @Description   : seqDB-23799:主子表属于相同CS，先drop子表，再drop主表，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.12
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23799";
   var clName = "cl_23799";
   var mainCLName = "mainCL_23799";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );

   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   // 删除子表后删除主表
   dbcs.dropCL( clName );
   dbcs.dropCL( mainCLName );

   // 恢复主表
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   var mainCL = db.getCS( csName ).getCL( mainCLName );
   var cursor = mainCL.find().sort( { a: 1 } );
   commCompareResults( cursor, [] );
   checkRecycleItem( recycleName );

   // 恢复子表并校验子表数据
   var recycleName = getOneRecycleName( db, csName + "." + clName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   var subCL = db.getCS( csName ).getCL( clName );
   var cursor = subCL.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验恢复后主子表对应关系及分区范围
   var clNames = [];
   var shardRanges = [];
   checkSubCL( csName, mainCLName, clNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}