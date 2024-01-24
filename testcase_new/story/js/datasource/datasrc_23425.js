/******************************************************************************
 * @Description   : seqDB-23425:源集群上创建使用数据源的集合空间/集合查看访问计划
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23425";
   var csName = "cs_23425";
   var clName = "cl_23425";
   var srcCSName = "datasrcCS_23425";
   var indexName = "index_23425";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var datascrCL = commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   datascrCL.createIndex( indexName, { a: 1 } );

   // 集合级映射
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      var bValue = parseInt( Math.random() * 10000 );
      var cValue = parseInt( Math.random() * 10000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   dbcl.insert( docs );
   db.analyze( { Collection: csName + "." + clName } );

   var actualResult = dbcl.find().explain();
   var expectResult = dbcl.find().explain();
   checkExplain( actualResult, expectResult );

   var actualResult = dbcl.find( { a: 1 } ).explain();
   var expectResult = dbcl.find( { a: 1 } ).explain();
   checkExplain( actualResult, expectResult );

   // 集合空间级映射
   commDropCS( db, csName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      var bValue = parseInt( Math.random() * 10000 );
      var cValue = parseInt( Math.random() * 10000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   dbcl.insert( docs );
   db.analyze( { Collection: csName + "." + clName } );

   var actualResult = dbcl.find().explain();
   var expectResult = dbcl.find().explain();
   checkExplain( actualResult, expectResult );

   var actualResult = dbcl.find( { a: 1 } ).explain();
   var expectResult = dbcl.find( { a: 1 } ).explain();
   checkExplain( actualResult, expectResult );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function checkExplain ( actualResult, expectResult )
{
   assert.equal( actualResult.current().toObj().ScanType, expectResult.current().toObj().ScanType );
   assert.equal( actualResult.current().toObj().IndexName, expectResult.current().toObj().IndexName );
   assert.equal( actualResult.current().toObj().NodeName, expectResult.current().toObj().NodeName );
   actualResult.close();
   expectResult.close();
}