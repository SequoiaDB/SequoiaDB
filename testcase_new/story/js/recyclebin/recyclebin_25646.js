/******************************************************************************
 * @Description   : seqDB-25646:主子表，主表truncate后删除主表，重命名恢复truncate项目 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25646";
   var subCSName = "subcs_25646";
   var mainCLName = "mainCL_25646";
   var mainCLNameNew = "mainCL_new_25646";
   var subCLName1 = "subCL_25646_1";
   var subCLName2 = "subCL_25646_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, subCSName );
   cleanRecycleBin( db, subCSName );

   var dbcs = commCreateCS( db, csName );
   var subcs = commCreateCS( db, subCSName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, subCSName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, subCSName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( subCSName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( subCSName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 主表执行truncate
   maincl.truncate();

   // 删除主表
   dbcs.dropCL( mainCLName );

   // 重命名恢复truncate项目
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Truncate" );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLNameNew );

   // 恢复后对数据进行校验
   var maincl = dbcs.getCL( mainCLNameNew );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验子表名称未发生变化
   subcs.getCL( subCLName1 );
   subcs.getCL( subCLName2 );

   // 校验原始主表不存在
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcs.getCL( mainCLName );
   } );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [subCSName + "." + subCLName1, subCSName + "." + subCLName2];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLNameNew, subCLNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
   commDropCS( db, subCSName );
   cleanRecycleBin( db, subCSName );
}