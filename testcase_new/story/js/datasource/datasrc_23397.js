/******************************************************************************
 * @Description   : seqDB-23397:使用数据源的集合空间，源集群上主表执行sort/limit/skip混合查询 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
// jira SEQUOIADBMAINSTREAM-6673
// main( test );
function test ()
{
   var dataSrcName = "datasrc23397";
   var csName = "cs_23397";
   var mainCSName = "mainCS_23397";
   var srcCSName = "datasrcCS_23397";
   var clName = "cl_23397";
   var mainCLName = "mainCL_23397";
   var subCLName = "subCL_23397";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   clearDataSource( mainCSName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var mainCL = commCreateCL( db, mainCSName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, mainCSName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( mainCSName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 1000 }, UpBound: { a: 2000 } } );

   var docs = [];
   for( var i = 0; i < 2000; i++ )
   {
      var bValue = parseInt( Math.random() * 5000 );
      var cValue = parseInt( Math.random() * 5000 );
      docs.push( { a: i, b: bValue, c: cValue } );
   }
   mainCL.insert( docs );

   var cursor = mainCL.find().sort( { a: 1 } ).skip( 1000 ).limit( 1000 );
   docs.sort( sortBy( 'a' ) );
   var expRecs = docs.slice( 1000, 2000 );
   commCompareResults( cursor, expRecs );

   var cursor = mainCL.find().sort( { a: 1 } ).skip( 100 ).limit( 1500 );
   var expRecs = docs.slice( 100, 1600 );
   commCompareResults( cursor, expRecs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
}