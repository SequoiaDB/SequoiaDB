/******************************************************************************
 * @Description   : seqDB-24817:CL 层面 IX 锁升级为 Z 锁
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24817";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;

   db.transBegin();
   var docs = insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_IX );

   // 写操作之后执行truncate
   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcl.truncate();
   } );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
