/******************************************************************************
 * @Description   : seqDB-23820:主子表，插入记录，在主表做truncate，从主表恢复
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2022.02.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23820";
   var clName = "cl_23820";
   var mainCLName = "mainCL_23820";
   var subCLName = "subCL_23820";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   commCreateCS( db, csName );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 1500; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   // 主表truncate后从主表恢复
   mainCL.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + mainCLName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   var cursor = mainCL.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}