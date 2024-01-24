/******************************************************************************
 * @Description   : seqDB-23784:主子表属于不同CS，dropCS后恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.07
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_23784_1";
   var csName2 = "cs_23784_2";
   var mainCLName = "mainCL_23784";
   var subCLName1 = "subCL_23784_1";
   var subCLName2 = "subCL_23784_2";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23784" );

   commCreateCS( db, csName1 );
   commCreateCS( db, csName2 );
   var mainCL = commCreateCL( db, csName1, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName2, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName2, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName2 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName2 + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // 插入数据分布在两个子表上
   var docs = [];
   var docs1 = [];
   var docs2 = [];
   for( var i = 0; i < 800; i++ )
   {
      docs.push( { a: i, b: i } );
      docs1.push( { a: i, b: i } )
   }
   for( var i = 1000; i < 1800; i++ )
   {
      docs.push( { a: i, b: i } );
      docs2.push( { a: i, b: i } )
   }
   mainCL.insert( docs );

   // 删除cs1后进行恢复
   db.dropCS( csName1 );
   var recycleName = getOneRecycleName( db, csName1, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 删除cs2后进行恢复
   db.dropCS( csName2 );
   var recycleName = getOneRecycleName( db, csName2, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 校验主表下不存在子表
   var cataInfo = db.snapshot( SDB_SNAP_CATALOG, { Name: csName1 + "." + mainCLName } ).current().toObj().CataInfo;
   assert.equal( cataInfo, [], "expect subcl does not exist, actual " + JSON.stringify( cataInfo, 0, 2 ) );

   // 校验子表数据
   var subcl1 = db.getCS( csName2 ).getCL( subCLName1 );
   var cursor = subcl1.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );

   var subcl2 = db.getCS( csName2 ).getCL( subCLName2 );
   var cursor = subcl2.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs2 );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23784" );
}