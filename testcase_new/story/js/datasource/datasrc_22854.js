/******************************************************************************
 * @Description   : seqDB-22854:修改数据源访问权限
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
// 修改数据源访问权限并校验，验证数据操作
testConf.skipStandAlone = true;
main( test );
function test ()
{
   var dataSrcName = "datasrc22854";
   var csName = "cs_22854";
   var clName = "cl_22854";
   var srcCSName = "datasrcCS_22854";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   var datasrcCL = commCreateCL( datasrcDB, srcCSName, clName );
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2" }];
   datasrcCL.insert( docs );

   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );
   //集合级使用数据源
   var dbcs = db.createCS( csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   var dataSource = db.getDataSource( dataSrcName );
   // 修改为 "AccessMode": "READ"
   dataSource.alter( { AccessMode: "READ" } );
   readOnlyAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   // 修改为 "AccessMode": "WRITE"
   dataSource.alter( { AccessMode: "WRITE" } );
   writeOnlyAndCheckResult( dbcl );

   datasrcCL.remove();
   datasrcCL.insert( docs );
   // 修改为 "AccessMode": "NONE"
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   // 修改为 "AccessMode": "ALL"
   dataSource.alter( { AccessMode: "ALL" } );
   allPermAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   datasrcCL.insert( docs );
   // 验证 ALL 权限改回 NONE 权限，NONE权限生效
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, docs );

   //集合空间级使用数据源
   commDropCS( db, csName );
   datasrcCL.remove();
   datasrcCL.insert( docs );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   // 修改为 "AccessMode": "READ"
   dataSource.alter( { AccessMode: "READ" } );
   readOnlyAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   // 修改为 "AccessMode": "WRITE"
   dataSource.alter( { AccessMode: "WRITE" } );
   writeOnlyAndCheckResult( dbcl );

   datasrcCL.remove();
   datasrcCL.insert( docs );
   // 修改为 "AccessMode": "NONE"
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   // 修改为 "AccessMode": "ALL"
   dataSource.alter( { AccessMode: "ALL" } );
   allPermAndCheckResult( dbcl, docs );

   datasrcCL.remove();
   datasrcCL.insert( docs );
   // 验证 ALL 权限改回 NONE 权限，NONE权限生效
   dataSource.alter( { AccessMode: "NONE" } );
   nonePermAndCheckResult( dbcl, docs );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function readOnlyAndCheckResult ( dbcl, docs )
{
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.insert( { a: 3 } );
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.update( { $set: { c: "testcccc" } } );
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.remove();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );

   var cursor = dbcl.find( {}, { "_id": { "$include": 0 } } ).sort( { "a": 1 } );
   docs.sort( sortBy( 'a' ) );
   commCompareResults( cursor, docs );
}

function writeOnlyAndCheckResult ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "tests2", c: "test" }, { a: 3, b: { b: 1 } }];
   dbcl.insert( docs );
   dbcl.update( { $set: { c: "testcccc" } }, { a: 2 } );
   dbcl.remove( { a: 3 } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find().toArray();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );
}

function nonePermAndCheckResult ( dbcl, docs )
{
   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.insert( { a: 3 } );
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.update( { $set: { c: "testcccc" } } );
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.remove();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );

   assert.tryThrow( [SDB_COORD_DATASOURCE_PERM_DENIED], function() 
   {
      dbcl.find().toArray();
   } );
}

function allPermAndCheckResult ( dbcl )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test2" }, { a: 3, b: 234.3 }];
   dbcl.insert( docs );

   dbcl.update( { $set: { b: 3 } }, { a: { $gt: 2 } } );
   dbcl.remove( { a: { $et: 2 } } );
   dbcl.insert( { a: 2, b: 2 } );

   var docs = [{ a: 1, b: 1 }, { a: 2, b: 2 }, { a: 3, b: 3 }];
   var cursor = dbcl.find().sort( { "a": 1 } );
   commCompareResults( cursor, docs );
}