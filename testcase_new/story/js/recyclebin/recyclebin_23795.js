/******************************************************************************
 * @Description   : seqDB-23795:主子表属于同一个CS，drop主表，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.01
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23795";
   var mainCLName = "mainCL_23795";
   var subCLName1 = "subCL_23795_1";
   var subCLName2 = "subCL_23795_2";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   var dbcs = commCreateCS( db, csName );
   var maincl = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName1, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, subCLName2, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   maincl.attachCL( csName + "." + subCLName1, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   maincl.attachCL( csName + "." + subCLName2, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

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
   maincl.insert( docs );

   // 删除主表后进行恢复
   dbcs.dropCL( mainCLName );
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Drop" );
   db.getRecycleBin().returnItem( recycleName );

   // 恢复后对数据进行校验
   var maincl = db.getCS( csName ).getCL( mainCLName );
   var cursor = maincl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   // 直接校验子表数据
   var subcl1 = db.getCS( csName ).getCL( subCLName1 );
   var cursor = subcl1.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs1 );
   var subcl2 = db.getCS( csName ).getCL( subCLName2 );
   var cursor = subcl2.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs2 );

   // 校验恢复后主子表对应关系及分区范围
   var subCLNames = [csName + "." + subCLName1, csName + "." + subCLName2];
   var shardRanges = [0, 1000, 2000];
   checkSubCL( csName, mainCLName, subCLNames, shardRanges );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}