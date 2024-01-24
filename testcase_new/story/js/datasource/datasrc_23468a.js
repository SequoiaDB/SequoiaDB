/******************************************************************************
 * @Description   : seqDB-23468:创建数据源设置访问权限，主子表使用数据源执行操作 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 设置数据源 AccessMode:"READ" 权限
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23468a";
   var csName = "cs_23468a";
   var clName = "cl_23468a";
   var srcCSName = "datasrcCS_23468a";
   var mainCLName = "mainCL_23468a";
   var subCLName = "subCL_23468a";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var dataSrcCL = commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "READ" } );

   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   var subCL = commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // insert 覆盖两个子表
   var docs2 = [];
   for( var i = 500; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs2.push( { a: i, b: bValue, c: cValue } );
   }
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.insert( docs2 );
   } );

   // insert 数据只在本地子表
   subCL.remove();
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // 数据源上的集合插入数据，用于读操作
   var docs2 = [];
   for( var i = 1000; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
      docs2.push( { a: i, b: bValue, c: cValue } );
   }
   dataSrcCL.insert( docs2 );

   // find 数据只在本地子表
   var cursor = mainCL.find( { a: { $lt: 500 } } );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 0, 500 );
   commCompareResults( cursor, expRecs );

   // find 覆盖两个子表
   var cursor = mainCL.find().sort( { a: 1 } ).skip( 100 ).limit( 1500 );
   var expRecs = docs.slice( 100, 1600 );
   commCompareResults( cursor, expRecs );

   // findAndUpdate，覆盖只在本地子表和同时在两个子表
   mainCL.find( { a: 1 } ).update( { $set: { b: "test" } } ).toArray();
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.find( { $and: [{ a: { $gt: 950 } }, { a: { $lt: 1150 } }] } ).update( { $set: { b: "test" } } ).toArray();
   } );

   // fandAndRemove，覆盖只在本地子表和同时在两个子表
   mainCL.find( { a: { $lt: 5 } } ).remove().toArray();
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.find( { $and: [{ a: { $gt: 950 } }, { a: { $lt: 1150 } }] } ).remove().toArray();
   } );

   // remove，覆盖只在本地子表和同时在两个子表
   mainCL.remove( { a: { $lt: 10 } } );
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.remove( { $and: [{ a: { $gt: 900 } }, { a: { $lt: 1100 } }] } )
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.truncate();
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}