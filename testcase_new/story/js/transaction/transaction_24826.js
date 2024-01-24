/******************************************************************************
 * @Description   : seqDB-24826:验证 X 是否能升级为 S(DDL)
 * @Author        : Yao Kang
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2021.12.31
 * @LastEditors   : Yao Kang
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24826";
testConf.csName = COMMCSNAME + "_24826";
main( test );
function test ()
{
   var clNameNew = "cl_new_24826";
   var sessionAttr = { TransIsolation: 2, TransMaxLockNum: 10 };
   db.setSessionAttr( sessionAttr );

   var dbcs = testPara.testCS;
   var dbcl = testPara.testCL;

   db.transBegin();
   insertData( dbcl, 20 );
   checkCLLockType( db, LOCK_X );

   assert.tryThrow( SDB_DPS_INVALID_LOCK_UPGRADE_REQUEST, function()
   {
      dbcs.renameCL( testConf.clName, clNameNew );
   } );
   db.transCommit();

   // 验证集合是否重命名成功
   assert.tryThrow( SDB_DMS_NOTEXIST, function()
   {
      dbcs.getCL( clNameNew );
   } );

}
