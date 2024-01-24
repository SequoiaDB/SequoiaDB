/******************************************************************************
 * @Description   : seqDB-25638:主子表重命名恢复，子表名存在冲突，同时子表名存<原表名>_<recycleID>冲突 
 * @Author        : liuli
 * @CreateTime    : 2022.03.26
 * @LastEditTime  : 2022.03.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_25638";
   var mainCLName = "mainCL_25638";
   var subCLName1 = "subCL_25638_1";
   var subCLName2 = "subCL_25638_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   insertBulkData( maincl, 2000 );

   // 删除主表后创建子表subcl1同名CL
   dbcs.dropCL( mainCLName );
   dbcs.createCL( subCLName1 );

   // 同时创建 subcl1_recycleID 名称CL
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   var recycleID = getRecycleID( db, recycleName );
   var subCLNameNew1 = subCLName1 + "_" + recycleID;
   dbcs.createCL( subCLNameNew1 );

   // 重命名恢复主表
   assert.tryThrow( SDB_DMS_EXIST, function()
   {
      db.getRecycleBin().returnItemToName( recycleName, csName + "." + mainCLName );
   } );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}