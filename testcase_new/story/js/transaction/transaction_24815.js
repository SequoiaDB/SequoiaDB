/******************************************************************************
 * @Description   : seqDB-24815:验证 IX 是否能升级为 S(DDL)
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.30
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24815";

main( test );
function test ()
{
   var clNameNew = "cl_new_24815";
   var sessionAttr = { TransIsolation: 2 };
   db.setSessionAttr( sessionAttr );
   var dbcl = testPara.testCL;
   var dbcs = testPara.testCS;

   db.transBegin();
   var docs = insertData( dbcl, 10 );
   checkCLLockType( db, LOCK_IX );

   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcs.renameCL( testConf.clName, clNameNew );
   } );
   db.transCommit();

   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );

}
