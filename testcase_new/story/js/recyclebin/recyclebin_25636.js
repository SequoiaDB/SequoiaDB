/******************************************************************************
 * @Description   : seqDB-25636:主子表重命名恢复 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25636";
   var mainCLName = "mainCL_25636";
   var mainCLNameNew = "mainCL_new_25636";
   var subCLName1 = "subCL_25636_1";
   var subCLName2 = "subCL_25636_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 删除主表后进行重命名恢复恢复
   dbcs.dropCL( mainCLName );
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   var returnName = db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLNameNew );

   // 校验返回名称
   assert.equal( returnName.toObj().ReturnName, csName + "." + mainCLNameNew );

   // 恢复后对数据进行校验
   var maincl = dbcs.getCL( mainCLNameNew );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验子表名称未发生变化
   dbcs.getCL( subCLName1 );
   dbcs.getCL( subCLName2 );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [csName + "." + subCLName1, csName + "." + subCLName2];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLNameNew, subCLNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}