/******************************************************************************
 * @Description   : seqDB-23132:使用数据源的集合，源集群上主表执行sort/limit/skip混合查询 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23132";
   var csName = "cs_23132";
   var clName = "cl_23132";
   var srcCSName = "datasrcCS_23132";
   var mainCLName = "mainCL_23132";
   var subCLName = "subCL_23132";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // 返回记录全部在使用数据源的子表上
   var cursor = mainCL.find().sort( { a: 1 } ).skip( 1000 ).limit( 1000 );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 1000, 2000 );
   commCompareResults( cursor, expRecs );

   // 返回记录覆盖两个子表
   var cursor = mainCL.find().sort( { a: 1 } ).skip( 100 ).limit( 1500 );
   var expRecs = docs.slice( 100, 1600 );
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}