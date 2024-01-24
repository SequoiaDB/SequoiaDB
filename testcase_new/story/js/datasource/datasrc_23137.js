/******************************************************************************
 * @Description   : seqDB-23137:使用数据源，指定选择条件查询数据 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23137";
   var csName = "cs_23137";
   var clName = "cl_23137";
   var srcCSName = "datasrcCS_23137";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   var docs = [];
   var expRecs = [];
   for( var i = 0; i < 10000; i++ )
   {
      var bValue = parseInt( Math.random() * 10000 );
      var cValue = parseInt( Math.random() * 10000 );
      docs.push( { a: i, b: bValue, c: cValue } );
      expRecs.push( { a: i, c: cValue } );
   }
   dbcl.insert( docs );

   var cursor = dbcl.find( {}, { b: { $include: 0 } } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}