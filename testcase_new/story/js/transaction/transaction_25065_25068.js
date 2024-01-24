/******************************************************************************
 * @Description   : seqDB-25065:事务中指定SDB_FLG_QUERY_FOR_SHARE查询
 *                : seqDB-25068:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
 * @Author        : liuli
 * @CreateTime    : 2022.02.11
 * @LastEditTime  : 2022.02.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_25065";

main( test );
function test ( testPara )
{
   // 设置事务隔离级别为RU
   var sessionAttr = { TransIsolation: 0, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var indexName = "index_25065";
   var dbcl = testPara.testCL;
   var docs = insertData( dbcl, 20 );
   dbcl.createIndex( indexName, { a: 1 } );

   // 开启事务后查询10条数据
   db.transBegin();
   dbcl.find( { a: { "$lt": 10 } } ).hint( { "": indexName } ).toArray();

   // 校验没有记录锁为S锁
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 0 );

   // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
   dbcl.find( { a: { "$lt": 10 } } ).flags( SDB_FLG_QUERY_FOR_SHARE ).hint( { "": indexName } ).toArray();

   // 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 10 );
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_IS );

   // 不指定flags查询后10条数据
   dbcl.find( { a: { "$gte": 10 } } ).hint( { "": indexName } ).toArray();

   // 记录锁数量不变，集合锁不变，没有发生锁升级
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 10 );
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_IS );

   // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
   dbcl.find( { a: { "$gte": 10 } } ).flags( SDB_FLG_QUERY_FOR_SHARE ).hint( { "": indexName } ).toArray();

   // 发生锁升级，集合锁为S锁
   checkIsLockEscalated( db, true );
   checkCLLockType( db, LOCK_S );

   // 提交事务
   db.transCommit();

   // 设置事务隔离级别RC，重复进行测试
   var sessionAttr = { TransIsolation: 1 };
   db.setSessionAttr( sessionAttr );

   // 开启事务后查询10条数据
   db.transBegin();
   dbcl.find( { a: { "$lt": 10 } } ).hint( { "": indexName } ).toArray();

   // 校验没有记录锁为S锁
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 0 );

   // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询10条数据
   dbcl.find( { a: { "$lt": 10 } } ).flags( SDB_FLG_QUERY_FOR_SHARE ).hint( { "": indexName } ).toArray();

   // 校验记录锁中S锁数量为10，集合锁为IS，没有发生锁升级
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 10 );
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_IS );

   // 不指定flags查询后10条数据
   dbcl.find( { a: { "$gte": 10 } } ).hint( { "": indexName } ).toArray();

   // 记录锁数量不变，集合锁不变，没有发生锁升级
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 10 );
   checkIsLockEscalated( db, false );
   checkCLLockType( db, LOCK_IS );

   // 指定flags为SDB_FLG_QUERY_FOR_SHARE查询后10条数据
   dbcl.find( { a: { "$gte": 10 } } ).flags( SDB_FLG_QUERY_FOR_SHARE ).hint( { "": indexName } ).toArray();

   // 发生锁升级，集合锁为S锁
   checkIsLockEscalated( db, true );
   checkCLLockType( db, LOCK_S );

   // 事务中指定flags为SDB_FLG_QUERY_FOR_SHARE读取数据并校验
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_FOR_SHARE );
   commCompareResults( actResult, docs );

   // 提交事务
   db.transCommit();

   // seqDB-25068:不开起事务使用SDB_FLG_QUERY_FOR_SHARE读数据
   var actResult = dbcl.find().sort( { a: 1 } ).flags( SDB_FLG_QUERY_FOR_SHARE );
   commCompareResults( actResult, docs );
}