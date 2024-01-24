/******************************************************************************
 * @Description   : seqDB-24871:锁升级后事务回滚
 * @Author        : liuli
 * @CreateTime    : 2021.12.20
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24871";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcl = testPara.testCL;
   var docs = insertData( dbcl, 20 );

   // 开启事务后删除所有数据
   db.transBegin();
   dbcl.remove();
   checkCLLockType( db, LOCK_X );
   checkIsLockEscalated( db, true );

   // 回滚事务后校验结果
   db.transRollback();
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}