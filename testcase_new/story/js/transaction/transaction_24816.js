/******************************************************************************
 * @Description   : seqDB-24816:CL 层面 IX 锁升级为 X 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24816";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;

   db.transBegin();
   insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_IX );

   // 读操作之后再执行写操作，IS锁升级为X锁
   insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_X );
   checkIsLockEscalated( db, true );
   db.transCommit();
}
