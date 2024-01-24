/******************************************************************************
 * @Description   : seqDB-23427:创建数据源，设置ErrorControlLevel权限控制
 * @Author        : Wu Yan
 * @CreateTime    : 2021.01.17
 * @LastEditTime  : 2021.09.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc23427";
   var csName = "cs_23427";
   var srcCSName = "datasrcCS_23427";
   var clName = "cl_23427";
   var mainCLName = "maincl_23427";
   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCS( datasrcDB, srcCSName );
   commCreateCL( datasrcDB, srcCSName, clName );

   // 设置ErrorControlLevel权限控制为小写high
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "ErrorControlLevel": "high" } );

   // 集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   indexOprAndCheckResult( dbcl );

   // 集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   indexOprAndCheckResult( dbcl );

   db.dropCS( csName );
   db.dropDataSource( dataSrcName );

   // 设置ErrorControlLevel权限控制为包含大写，如High
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "ErrorControlLevel": "High" } );
   // 集合级使用数据源
   var cs = db.createCS( csName );
   var dbcl = cs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   indexOprAndCheckResult( dbcl );
   metaOprAndCheckResult( csName, mainCLName, clName );

   // 集合空间级使用数据源
   db.dropCS( csName );
   var cs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   indexOprAndCheckResult( dbcl );

   clearDataSource( csName, dataSrcName );
   datasrcDB.dropCS( srcCSName );
   datasrcDB.close();
}

function indexOprAndCheckResult ( dbcl )
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
      dbcl.dropIndex( "testa" );
   } );

   var recordNum = 10000;
   var expRecs = insertBulkData( dbcl, recordNum, 0, 40000 );
   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   dbcl.truncate();
}

function metaOprAndCheckResult ( csName, mainCLName, clName )
{
   var mainCL = commCreateCL( db, csName, mainCLName, { ShardingKey: { a: 1 }, ShardingType: "range", IsMainCL: true } );
   mainCL.attachCL( csName + "." + clName, { LowBound: { a: 0 }, UpBound: { a: 1000 } } );
   var recordNum = 1000;
   var expRecs = insertBulkData( mainCL, recordNum, 0, 1000 );
   var cursor = mainCL.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   expRecs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, expRecs );
   mainCL.remove();

   mainCL.detachCL( csName + "." + clName );
   assert.tryThrow( SDB_CAT_NO_MATCH_CATALOG, function()
   {
      mainCL.insert( { a: 1 } );
   } );

   var cs = db.getCS( csName );

   cs.dropCL( clName );
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      cs.getCL( clName );
   } );

}
