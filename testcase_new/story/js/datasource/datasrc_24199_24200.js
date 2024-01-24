/******************************************************************************
 * @Description   : seqDB-24199:TransPropagateMode为notsupport，事务中使用数据源（集合级映射）
 *                  seqDB-24200:TransPropagateMode为notsupport，事务中使用数据源（集合空间级映射）
 * @Author        : liuli
 * @CreateTime    : 2021.05.27
 * @LastEditTime  : 2021.05.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24199";
testConf.skipStandAlone = true;

main( test );
function test ( args )
{
   var cl = args.testCL;
   var dataSrcName = "datasrc24199";
   var csName = "cs_24199";
   var clName = "cl_24199";
   var srcCSName = "datasrcCS_24199";

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   commCreateCL( datasrcDB, srcCSName, clName );
   db.createDataSource( dataSrcName, datasrcUrl, userName, passwd, "SequoiaDB", { TransPropagateMode: "notsupport" } );

   // 集合级别映射
   var dbcs = commCreateCS( db, csName );
   var dbcl = dbcs.createCL( clName, { DataSource: dataSrcName, Mapping: srcCSName + "." + clName } );

   transAndCheckResult( cl, dbcl );
   commDropCS( db, csName );

   // 集合空间级别映射
   var dbcs = db.createCS( csName, { DataSource: dataSrcName, Mapping: srcCSName } );
   var dbcl = dbcs.getCL( clName );
   transAndCheckResult( cl, dbcl );

   commDropCS( datasrcDB, srcCSName );
   clearDataSource( csName, dataSrcName );
   datasrcDB.close();
}

function transAndCheckResult ( cl, dbcl )
{
   // 开启事务，在使用数据源的cl和普通cl上执行数据操作，最终提交事务
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test" }, { a: 3, b: "b" }];
   db.transBegin();
   cl.insert( docs );
   dbcl.insert( docs );
   cl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   cl.find( { a: 3 } ).remove().toArray();
   dbcl.find( { a: 3 } ).remove().toArray();
   dbcl.find().toArray();
   db.transCommit();

   expextResult = [{ a: 1, b: 4 }, { a: 2, b: "test" }];
   var cursor = cl.find().sort( { a: 1 } );
   commCompareResults( cursor, expextResult );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, expextResult );

   db.transBegin();
   dbcl.truncate();
   db.transCommit();
   var cursor = dbcl.find();
   commCompareResults( cursor, [] );

   // 开启事务，在使用数据源的cl和普通cl上执行数据操作，最终回滚事务
   var docs = [{ a: 1, b: 1 }, { a: 2, b: "test" }, { a: 3, b: "b" }];
   db.transBegin();
   cl.insert( docs );
   dbcl.insert( docs );
   cl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   dbcl.find( { a: 1 } ).update( { $inc: { b: 3 } } ).toArray();
   cl.find( { a: 3 } ).remove().toArray();
   dbcl.find( { a: 3 } ).remove().toArray();
   dbcl.find().toArray();
   db.transRollback();

   var cursor = cl.find().sort( { a: 1 } );
   commCompareResults( cursor, expextResult );
   var cursor = dbcl.find().sort( { a: 1 } );
   commCompareResults( cursor, expextResult );

   db.transBegin();
   cl.truncate();
   dbcl.truncate();
   db.transRollback();
   var cursor = cl.find();
   commCompareResults( cursor, [] );
   var cursor = dbcl.find();
   commCompareResults( cursor, [] );
}
