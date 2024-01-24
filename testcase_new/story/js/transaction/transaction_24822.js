/******************************************************************************
 * @Description   : seqDB-24822:seqDB-24822:验证 SIX 是否能升级为 S
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.16
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24822";

main( test );
function test ()
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;

   insertData( dbcl, 20 );

   db.transBegin();

   insertData( dbcl, 10 );
   dbcl.find().limit( 10 ).toArray();
   checkCLLockType( db, LOCK_SIX );
   dbcl.find().skip( 10 ).limit( 10 ).toArray();
   checkCLLockType( db, LOCK_SIX );

   db.transCommit();
}
