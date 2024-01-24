/******************************************************************************
 * @Description   : seqDB-23128:使用数据源，源集群上使用索引查询
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23128";
   var csName = "cs_23128";
   var clName = "cl_23128";
   var srcCSName = "datasrcCS_23128";
   var indexName1 = "index_23128a";
   var indexName2 = "index_23128b";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   var datasrcCL = commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   datasrcCL.createIndex( indexName1, { a: 1 } );

   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var dValue = parseInt( Math.random() * 10000 );
      docs.push( { a: i, b: i + 5, c: 10000 - i, d: dValue } );
   }
   dbcl.insert( docs );

   var cursor = dbcl.find( { a: { $gt: 100 }, b: { $lt: 200 } } );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 101, 195 );
   commCompareResults( cursor, expRecs );

   datasrcCL.createIndex( indexName2, { b: 1, c: -1 } );
   var cursor = dbcl.find( { a: { $gt: 5000 } } ).hint( { 1: indexName2 } );
   var expRecs = docs.slice( 5001, 10000 );
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}