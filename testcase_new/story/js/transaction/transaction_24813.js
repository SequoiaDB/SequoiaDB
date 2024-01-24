/******************************************************************************
 * @Description   : seqDB-24813:验证 IS 是否能升级为 Z
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.16
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24813";

main( test );
function test ()
{
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;

   var docs = insertData( dbcl, 10 );

   db.transBegin();
   dbcl.find().toArray();
   checkCLLockType( db, LOCK_IS );

   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcl.truncate();
   } );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}
