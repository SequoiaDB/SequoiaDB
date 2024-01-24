/******************************************************************************
 * @Description   : seqDB-22881:使用数据源的集合为子表，挂载到主表
 * @Author        : Wu Yan
 * @CreateTime    : 2020.10.20
 * @LastEditTime  : 2021.03.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22881";
   var csName = "cs_22881";
   var clName = "cl_22881";
   var mainCLName = "mainCL_22881";
   var subCLName = "subCL_22881";
   commDropCS( datasrcDB, csName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, csName );
   commCreateCL( datasrcDB, csName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   var cs = db.createCS( csName );
   cs.createCL( clName, { DataSource: dataSrcName } );
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   commCreateCL( db, csName, subCLName, { ShardingKey: { a: 1 }, ShardingType: "hash" } );
   mainCL.attachCL( csName + "." + subCLName, { LowBound: { a: 0 }, UpBound: { a: 10000 } } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 10000 }, UpBound: { a: 20000 } } );

   var recordNum = 20000;
   var expRecs = insertBulkData( mainCL, recordNum, 0, 40000 );
   var cursor = mainCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   var count = mainCL.count();
   assert.equal( count, recordNum );

   mainCL.remove( { a: { $gte: 500 } } );
   var count = mainCL.count();
   assert.equal( count, 500 );

   mainCL.truncate();
   var count = mainCL.count();
   assert.equal( count, 0 );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( csName );
   datasrcDB.close();
}


