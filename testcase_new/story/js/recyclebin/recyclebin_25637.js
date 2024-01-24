/******************************************************************************
 * @Description   : seqDB-25637:主子表重命名恢复，子表名存在冲突 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25637";
   var mainCLName = "mainCL_25637";
   var mainCLNameNew = "mainCL_new_25637";
   var subCLName1 = "subCL_25637_1";
   var subCLName2 = "subCL_25637_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 删除主表后创建子表同名CL
   dbcs.dropCL( mainCLName );
   dbcs.createCL( subCLName1 );
   dbcs.createCL( subCLName2 );

   // 重命名恢复主表
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   var recycleID = getRecycleID( db, recycleName );
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLNameNew );

   // 恢复后对数据进行校验
   var maincl = dbcs.getCL( mainCLNameNew );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验子表自动重命名
   var subCLNameNew1 = subCLName1 + "_" + recycleID;
   var subCLNameNew2 = subCLName2 + "_" + recycleID;
   dbcs.getCL( subCLNameNew1 );
   dbcs.getCL( subCLNameNew2 );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [csName + "." + subCLNameNew1, csName + "." + subCLNameNew2];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLNameNew, subCLNames, shardRanges );

   // 校验新建冲突CL中不存在数据
   var subcl1 = dbcs.getCL( subCLName1 );
   var subcl2 = dbcs.getCL( subCLName2 );

   var cursor1 = subcl1.find();
   commCompareResults( cursor1, [] );
   var cursor2 = subcl2.find();
   commCompareResults( cursor2, [] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}