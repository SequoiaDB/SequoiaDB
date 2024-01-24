/******************************************************************************
 * @Description   :seqDB-25385:事务中写锁锁定的行删除时，立即释放锁
 * @Author        : 钟子明
 * @CreateTime    : 2022.02.17
 * @LastEditTime  : 2022.02.18
 * @LastEditors   : 钟子明
 ******************************************************************************/
testConf.csName = COMMCSNAME + "_25385";
testConf.clName = COMMCLNAME + "_25385"

main( test );

function test ( testPara )
{
   var cl = testPara.testCL;

   db.transBegin();
   cl.insert( { a: 10000 } );
   assert.notEqual( getCLLockCount( db, LOCK_X ), 0, 'insert 操作失败' );

   cl.remove();
   assert.equal( getCLLockCount( db, LOCK_X ), 0, 'remove 操作未能释放事务写锁' );
   db.transRollback();

   db.transBegin();
   cl.insert( { _id: 1, a: 12345 } );
   assert.notEqual( getCLLockCount( db, LOCK_X ), 0, 'insert 操作失败' );
   cl.update( { $set: { a: 1234 } }, { _id: 1 } );

   cl.remove();
   assert.equal( getCLLockCount( db, LOCK_X ), 0, 'remove 操作未能释放事务写锁' );
   db.transRollback();

   db.transBegin();
   cl.insert( { _id: 1, a: 1048576 } );
   assert.notEqual( getCLLockCount( db, LOCK_X ), 0, 'insert 操作失败' );
   cl.update( { $set: { a: 1024, _id: 2 } }, { _id: 1 } );

   cl.remove();
   assert.equal( getCLLockCount( db, LOCK_X ), 0, 'remove 操作未能释放事务写锁' );
   db.transRollback();

}