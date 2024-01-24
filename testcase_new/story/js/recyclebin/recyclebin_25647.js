/******************************************************************************
 * @Description   : seqDB-25647:主子表，主表truncate后删除部分子表，重命名恢复主表 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25647";
   var mainCLName = "mainCL_25647";
   var mainCLNameNew = "mainCL_new_25647";
   var subCLName1 = "subCL_25647_1";
   var subCLName2 = "25647_子表";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 主表执行truncate
   maincl.truncate();

   // 删除子表subCLName1
   dbcs.dropCL( subCLName1 );

   // 重命名恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Truncate" );
   var recycleID = getRecycleID( db, recycleName );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLNameNew );

   // 校验原始主表不存在数据
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, [] );

   // 恢复后对数据进行校验
   var maincl = dbcs.getCL( mainCLNameNew );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验恢复后主子表对应关系及分区范围subCLName1原名恢复，subCLName2自动重命名
   var subCLNames = [csName + "." + subCLName1, csName + "." + subCLName2 + "_" + recycleID];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLNameNew, subCLNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}