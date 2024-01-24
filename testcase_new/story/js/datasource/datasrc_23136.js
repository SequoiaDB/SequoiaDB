/******************************************************************************
 * @Description   : seqDB-23136:使用数据源，指定查询条件查询数据
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23136";
   var csName = "cs_23136";
   var clName = "cl_23136";
   var srcCSName = "datasrcCS_23136";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );

   var docs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var cValue = parseInt( Math.random() * 10000 );
      docs.push( { a: i, b: i + 5, c: cValue } );
   }
   dbcl.insert( docs );

   // 查询数据满足匹配条件
   var cursor = dbcl.find( { $and: [{ a: { $gte: 1000 } }, { a: { $lte: 9000 } }] } );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 1000, 9001 );
   commCompareResults( cursor, expRecs );

   // 查询数据不满足匹配条件
   var cursor = dbcl.find( { a: { $gt: 9999 } } );
   var expRecs = [];
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}