/******************************************************************************
 * @Description   : seqDB-24104:主子表删除主表，创建主表同名CL，恢复/强制恢复主表项目
 * @Author        : liuli
 * @CreateTime    : 2021.04.16
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_24104";
   var clName = "cl_24104";
   var mainCLName = "mainCL_24104";
   var subCLName = "subCL_24104";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   // 删除主表后创建主表同名CL
   dbcs.dropCL( mainCLName );
   dbcs.createCL( mainCLName );

   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   assert.tryThrow( SDB_RECYCLE_CONFLICT, function()
   {
      db.getRecycleBin().returnItem( recycleName );
   } );

   // 强制恢复主表
   db.getRecycleBin().returnItem( recycleName, { Enforced: true } );

   // 恢复后对数据进行校验
   var mainCL = dbcs.getCL( mainCLName );
   var cursor = mainCL.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 校验主子表对应关系及分区范围
   var subCLNames = [csName + "." + clName, csName + "." + subCLName];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLName, subCLNames, shardRanges );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}