/******************************************************************************
 * @Description   : seqDB-25639:主表重命名恢复，存在表名为子表<原表名>_<recycleID>的集合
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25639";
   var mainCLName = "mainCL_25639";
   var subCLName1 = "subCL_25639_1";
   var subCLName2 = "subCL_25639_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = insertBulkData( maincl, 2000 );

   // 删除主表
   dbcs.dropCL( mainCLName );

   // 创建 subcl1_recycleID 名称CL
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   var recycleID = getRecycleID( db, recycleName );
   var subCLNameNew1 = subCLName1 + "_" + recycleID;
   var dbcl = dbcs.createCL( subCLNameNew1 );
   var docs2 = insertBulkData( dbcl, 1000 );

   // 重命名恢复主表
   db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLName );

   // 恢复后对数据进行校验
   var maincl = dbcs.getCL( mainCLName );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验新建表数据
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs2 );

   // 校验主子表对应关系
   var subCLNames = [csName + "." + subCLName1, csName + "." + subCLName2];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLName, subCLNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}