/******************************************************************************
 * @Description   : seqDB-23131:使用数据源的集合空间，源集群上执行sort/limit/skip混合查询 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23131";
   var csName = "cs_23131";
   var clName = "cl_23131";
   var srcCSName = "datasrcCS_23131";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );

   var docs = [];
   for( var i = 0; i < 5000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   dbcl.insert( docs );

   // sort 正序查看
   var cursor = dbcl.find().sort( { a: 1 } ).skip( 1000 ).limit( 1000 );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 1000, 2000 );
   commCompareResults( cursor, expRecs );

   // sort 逆序查看
   var cursor = dbcl.find().sort( { a: -1 } ).skip( 1000 ).limit( 1000 );
   docs.sort( sortByReverseOrder( 'a' ) );
   var expRecs = docs.slice( 1000, 2000 );
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function sortByReverseOrder ( field )
{
   return function( a, b )
   {
      return a[field] < b[field];
   }
}