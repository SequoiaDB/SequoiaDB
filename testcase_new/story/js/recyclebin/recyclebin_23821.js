/******************************************************************************
 * @Description   : seqDB-23821:主子表，插入记录，在子表做truncate，恢复子表
 * @Author        : liuli
 * @CreateTime    : 2021.04.09
 * @LastEditTime  : 2021.08.13
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var csName = "cs_23821";
   var clName = "cl_23821";
   var mainCLName = "mainCL_23821";
   var subCLName = "subCL_23821";

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );

   commCreateCS( db, csName );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCL = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   commCreateCL( db, csName, clName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 1500; i++ )
   {
      docs.push( { a: i, b: i } );
   }
   mainCL.insert( docs );

   // 子表subCL执行truncate后从恢复
   subCL.truncate();
   var recycleName = getOneRecycleName( db, csName + "." + subCLName, "Truncate" );
   db.getRecycleBin().returnItem( recycleName );

   // 主表读取数据并校验
   var cursor = mainCL.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
   checkRecycleItem( recycleName );

   commDropCS( db, csName );
   cleanRecycleBin( db, csName );
}