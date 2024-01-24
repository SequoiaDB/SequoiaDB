/******************************************************************************
 * @Description   : seqDB-23468:创建数据源设置访问权限，主子表使用数据源执行操作 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 设置数据源 AccessMode:"WRITE" 权限
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23468b";
   var csName = "cs_23468b";
   var clName = "cl_23468b";
   var srcCSName = "datasrcCS_23468b";
   var mainCLName = "mainCL_23468b";
   var subCLName = "subCL_23468b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { AccessMode: "WRITE" } );

   var dbcs = db.createCS( csName );
   dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   // insert 数据只在本地子表
   var docs = [];
   for( var i = 0; i < 500; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   // insert 覆盖两个子表
   var docs2 = [];
   for( var i = 500; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
      docs2.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs2 );

   // find 数据只在本地子表
   var cursor = mainCL.find( { a: { $lt: 500 } } );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 0, 500 );
   commCompareResults( cursor, expRecs );

   // find 覆盖两个子表
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      mainCL.find().toArray();
   } );

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
   mainCL.remove( { $and: [{ a: { $gt: 900 } }, { a: { $lt: 1100 } }] } )
   mainCL.truncate();

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}