/******************************************************************************
 * @Description   : seqDB-22842:创建数据源，设置不过滤数据读写错误
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22842";
   var csName = "cs_22842";
   var clName = "cl_22842";
   var srcCSName = "datasrcCS_22842";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "ErrorFilterMask": "NONE" } );

   // CS 级别映射
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   commDropCL( datasrcDB, srcCSName, clName );
   noneAndCheckResult( dbcl );

   // CL 级别映射
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   commDropCL( datasrcDB, srcCSName, clName );
   noneAndCheckResult( dbcl );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function noneAndCheckResult ( dbcl )
{
   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.insert( { a: 1 } );
   } );

   assert.tryThrow( [SDB_DMS_NOTEXIST], function() 
   {
      dbcl.upsert( { $set: { b: 1 } } );
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