/******************************************************************************
 * @Description   : seqDB-24820:验证 S 是否能升级为 X
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.16
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24820";

main( test );
function test ()
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;

   insertData( dbcl, 20 );

   db.transBegin();
   dbcl.find().toArray();
   checkCLLockType( db, LOCK_S );
   insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_X );
   db.transCommit();
}
