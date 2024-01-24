/******************************************************************************
 * @Description   : seqDB-22890:事务中使用数据源 
 * @Author        : liuli
 * @CreateTime    : 2021.02.04
 * @LastEditTime  : 2021.03.01
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_22890";
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var cl = args.testCL;
   var dataSrcName = "datasrc22890";
   var csName = "cs_22890";
   var clName = "cl_22890";
   var srcCSName = "datasrcCS_22890";
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test" }];

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd );

   // 集合级别映射
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );
   dbcl.insert( docs );

   transAndCheckResult( cl, [], function()
   {
      dbcl.insert( docs );
   } );

   transAndCheckResult( cl, [], function()
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   transAndCheckResult( cl, [], function()
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );

   // 校验事务中使用数据源执行读操作不会进行回滚
   transAndCheckResult( cl, docs, function()
   {
      dbcl.find().toArray();
   } );

   // 集合空间级别映射
   commDropCS( db, csName );
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   cl.remove();

   transAndCheckResult( cl, [], function()
   {
      dbcl.insert( docs );
   } );

   transAndCheckResult( cl, [], function()
   {
      dbcl.find( { a: 1 } ).remove().toArray();
   } );

   transAndCheckResult( cl, [], function()
   {
      dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   } );

   transAndCheckResult( cl, docs, function()
   {
      dbcl.find().toArray();
   } );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function transAndCheckResult ( cl, expextResult, command )
{
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test" }];
   db.transBegin();
   cl.insert( docs );
   assert.tryThrow( [SDB_COORD_DATASOURCE_TRANS_FORBIDDEN], command );
   db.transCommit();
   var cursor = cl.find();
   commCompareResults( cursor, expextResult );
}
