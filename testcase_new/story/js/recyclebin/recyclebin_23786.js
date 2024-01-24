/******************************************************************************
 * @Description   : seqDB-23786:主子表，部分子表跟主表属于不同CS，dropCS后恢复CS
 * @Author        : liuli
 * @CreateTime    : 2021.04.08
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName1 = "cs_23786_1";
   var csName2 = "cs_23786_2";
   var mainCLName = "mainCL_23786";
   var subCLName1 = "subCL_23786_1";
   var subCLName2 = "subCL_23786_2";

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23786" );

   commCreateCS( db, csName1 );
   commCreateCS( db, csName2 );
   var mainCL = commCreateCL( db, csName1, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   // 子表subCLName1在cs1下
   commCreateCL( db, csName1, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   // 子表subCLName2在cs2下
   commCreateCL( db, csName2, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName1 + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName2 + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   var docs1 = [];
   var docs2 = [];
   for( var i = 0; i < 2000; i++ )
   {
      docs.push( { a: i, b: i } );
      if( i < 1000 )
      {
         docs1.push( { a: i, b: i } );
      }
      else
      {
         docs2.push( { a: i, b: i } );
      }
   }
   mainCL.insert( docs );

   // 删除cs1后进行恢复
   db.dropCS( csName1 );
   var recycleName = getOneRecycleName( db, csName1, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 删除cs2后进行恢复
   db.dropCS( csName2 );
   var recycleName = getOneRecycleName( db, csName2, "Drop" );
   db.getRecycleBin().returnItem( recycleName );
   checkRecycleItem( recycleName );

   // 恢复后校验主子表关联关系
   checkSubCL( csName1, mainCLName, [csName1 + "." + subCLName1], [0, 1000] );

   // 从主表校验数据
   var maincl = db.getCS( csName1 ).getCL( mainCLName );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );

   // 校验子表subCLName2的数据
   var subcl = db.getCS( csName2 ).getCL( subCLName2 );
   var cursor = subcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs2 );

   commDropCS( db, csName1 );
   commDropCS( db, csName2 );
   cleanRecycleBin( db, "cs_23786" );
}