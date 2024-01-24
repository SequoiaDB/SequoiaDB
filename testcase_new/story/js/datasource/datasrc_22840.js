/******************************************************************************
 * @Description   : seqDB-22840:创建数据源，设置过滤数据读错误 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.02
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var dataSrcName = "datasrc22840";
   var csName = "cs_22840";
   var clName = "cl_22840";
   var srcCSName = "datasrcCS_22840";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { "ErrorFilterMask": "READ" } );

   // CS 级别映射
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   commDropCL( datasrcDB, srcCSName, clName );
   readAndCheckResult( dbcl );

   // CL 级别映射
   commDropCS( db, csName );
   commCreateCL( datasrcDB, srcCSName, clName );
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   commDropCL( datasrcDB, srcCSName, clName );
   readAndCheckResult( dbcl );

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
      dbcl.upsert( { $set: { b: 1 } } );
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