/******************************************************************************
 * @Description   : seqDB-25648:主子表，主表truncate后重命名恢复，子表名存在冲突，同时存在<原表名>_<recycleID>冲突 
 * @Author        : liuli
 * @CreateTime    : 2022.03.28
 * @LastEditTime  : 2022.03.28
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25648";
   var mainCLName = "mainCL_25648";
   var mainCLNameNew = "mainCL_new_25648";
   var subCLName1 = "subCL_25648_1";
   var subCLName2 = "subCL_25648_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash", AutoSplit: true } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   insertBulkData( maincl, 2000 );

   // 主表执行truncate
   maincl.truncate();

   // 创建名为子表<原表名>_<recycleID>的CL
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Truncate" );
   var recycleID = getRecycleID( db, recycleName );
   dbcs.createCL( subCLName1 + "_" + recycleID );

   // 重命名恢复truncate项目
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLNameNew );
   } );

   // 校验原始主表不存在数据
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, [] );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}