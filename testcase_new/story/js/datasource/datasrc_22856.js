/******************************************************************************
 * @Description   : seqDB-22856:修改数据源ErrorFilterMask属性 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 修改数据源 ErrorFilterMask 属性，校验数据操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22856";
   var csName = "cs_22856";
   var clName = "cl_22856";
   var srcCSName = "datasrcCS_22856";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   //集合级使用数据源
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   commDropCL( datasrcDB, srcCSName, clName );
   var dataSource = db.getDataSource( dataSrcName );
   dataSource.alter( { ErrorFilterMask: "READ" } );
   readAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "READ|WRITE" } );
   allAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "WRITE" } );
   writeAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "NONE" } );
   noneAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "ALL" } );
   allAndCheckResult( dbcl );

   //集合空间级使用数据源
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = db.getCS( csName ).getCL( clName );
   commDropCL( datasrcDB, srcCSName, clName );
   dataSource.alter( { ErrorFilterMask: "READ" } );
   readAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "READ|WRITE" } );
   allAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "WRITE" } );
   writeAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "ALL" } );
   allAndCheckResult( dbcl );

   dataSource.alter( { ErrorFilterMask: "NONE" } );
   noneAndCheckResult( dbcl );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function readAndCheckResult ( dbcl )
{
   dbcl.find().toArray();
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.remove();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).update( { $set: { a: 3 } } ).toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.truncate();
   } );
}

function writeAndCheckResult ( dbcl )
{
   dbcl.insert( { a: 1 } );
   dbcl.remove();
   dbcl.truncate();

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find().toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).update( { $set: { a: 3 } } ).toArray();
   } );
}

function noneAndCheckResult ( dbcl )
{
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.remove();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.truncate();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find().toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.find( { a: 1 } ).update( { $set: { a: 3 } } ).toArray();
   } );
}

function allAndCheckResult ( dbcl )
{
   dbcl.insert( { a: 1 } );
   dbcl.remove();
   dbcl.truncate();
   dbcl.find().toArray();
   dbcl.find( { a: 1 } ).remove().toArray();
   dbcl.find( { a: 1 } ).update( { $set: { a: 3 } } ).toArray();
}