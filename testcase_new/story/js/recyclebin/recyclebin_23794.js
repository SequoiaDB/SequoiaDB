/******************************************************************************
 * @Description   : seqDB-23794:主表为空
 * @Author        : liuli
 * @CreateTime    : 2021.04.01
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var csName = "cs_23794";
   var mainCLName = "mainCL_23794";
   var subCLName1 = "subCL_23794_1";
   var subCLName2 = "subCL_23794_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );

   // 删除主表后进行恢复
   dbcs.dropCL( mainCLName );
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复后挂载子表，插入数据并校验
   var mainCL = db.getCS( csName ).getCL( mainCLName );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );
   curdOperate( mainCL );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}

function curdOperate ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }, { a: 1500 }];
   dbcl.insert( docs );
   dbcl.update( { $set: { b: 3 } }, { a: { $gt: 2 } } );
   dbcl.remove( { a: { $et: 2 } } );
   dbcl.insert( { a: 1200, b: 2 } );
   var docs = [{ a: 1, b: 1 }, { a: 3, b: 3 }, { a: 1200, b: 2 }, { a: 1500, b: 3 }];
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
}