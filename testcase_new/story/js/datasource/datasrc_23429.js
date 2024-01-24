/******************************************************************************
 * @Description   : seqDB-23429 : 修改数据源ErrorControlLevel属性
 * @Author        : Wu Yan
 * @CreateTime    : 2021.01.17
 * @LastEditTime  : 2021.03.16
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc23429";
   var csName = "cs_23429";
   var csName1 = "cs_23429b";
   var clName = "cl_23429";
   var datasrcDB = new Sdb( datasrcIp, datasrcPort, userName, passwd );
   commDropCS( datasrcDB, csName );
   commDropCS( db, csName1 );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, csName );
   commCreateCL( datasrcDB, csName, clName );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: clName } );
   var cs1 = db.createCS( csName1, { DataSource: dataSrcName, Mapping: csName } );

   //test a:ErrorControlLevel更新为Low
   var src = db.getDataSource( dataSrcName );
   src.alter( { "ErrorControlLevel": "low" } );
   //集合级使用数据源
   ignoreOperationError( dbcl );

   //集合空间级使用数据源
   var dbcl1 = db.getCS( csName1 ).getCL( clName );
   ignoreOperationError( dbcl1 );

   //test b:ErrorControlLevel更新为HIGH
   src.alter( { "ErrorControlLevel": "high" } );
   //集合级使用数据源
   operationPremDenied( dbcl );
   //集合空间级使用数据源
   operationPremDenied( dbcl1 );

   datasrcDB.dropCS( csName );
   db.dropCS( csName );
   db.dropCS( csName1 );
   db.dropDataSource( dataSrcName );
   datasrcDB.close();
}

function ignoreOperationError ( dbcl )
{
   dbcl.createIndex( "testno", { no: 1 } );
   dbcl.listIndexes().toArray();
   assert.tryThrow( SDB_IXM_NOTEXIST, function()
   {
      dbcl.getIndex( "testno" );
   } );
   dbcl.dropIndex( "testno" );
   dbcl.getDetail();

   var recordNum = 10000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 40000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );

   dbcl.truncate();
}

function operationPremDenied ( dbcl )
{
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.createIndex( "testno", { no: 1 } );
   } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.listIndexes().toArray();
   } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.getIndex( "testa" );
   } );

   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.dropIndex( "testa" );
   } );

   /*
   assert.tryThrow( SDB_OPERATION_INCOMPATIBLE, function()
   {
      dbcl.getDetail();
   } );
   */

   var recordNum = 10000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 40000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   dbcl.remove();
}
