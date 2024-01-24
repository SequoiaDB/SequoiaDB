/******************************************************************************
 * @Description   : seqDB-24818:CL 层面 S 锁升级为 S 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24818";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var indexName = "index_24818";
   dbcl.createIndex( indexName, { a: 1 } );
   insertData( dbcl, 30 );

   db.transBegin();
   // 读20条数据后集合锁为S，校验锁个数
   dbcl.find( { a: { "$lt": 20 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_S );
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 11, "expected lock count is 10, actual lock count is " + lockCount );

   // 读取最后10条数据后校验锁个数
   dbcl.find( { a: { "$gte": 20 } } ).hint( { "": indexName } ).toArray();
   checkCLLockType( db, LOCK_S );
   var lockCount = getCLLockCount( db, LOCK_S );
   assert.equal( lockCount, 11, "expected lock count is 10, actual lock count is " + lockCount );
   db.transCommit();
}
