/******************************************************************************
 * @Description   : seqDB-24827:验证 X 是否能升级为 X
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.20
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24827";

main( test );
function test ()
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;

   db.transBegin();

   insertData( dbcl, 20 );
   var lockCount1 = getCLLockCount( db, LOCK_X );
   checkCLLockType( db, LOCK_X );
   insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_X );
   var lockCount2 = getCLLockCount( db, LOCK_X );
   assert.equal( lockCount2, lockCount1, "expected lock count is " + lockCount1 + " actual lock count is " + lockCount2 );

   db.transCommit();
}
