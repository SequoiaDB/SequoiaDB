/******************************************************************************
 * @Description   : seqDB-24808:事务执行中，更新锁升级的 session 配置
 * @Author        : liuli
 * @CreateTime    : 2021.12.14
 * @LastEditTime  : 2022.01.03
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_24808";

main( test );
function test ( testPara )
{
   var sessionAttr = { TransAllowLockEscalation: false, TransMaxLockNum: 10, TransMaxLogSpaceRatio: 10 };

   var dbcl = testPara.testCL;

   // 开启事务后更新sessionAttr
   db.transBegin();
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   db.transCommit();

   // 更新sessionAttr后开启事务
   db.setSessionAttr( sessionAttr );
   db.transBegin();
   // 插入11条数据，再次插入一条数据
   insertData( dbcl, 11 );
   assert.tryThrow( SDB_DPS_TRANS_LOCK_UP_TO_LIMIT, function()
   {
      dbcl.insert( { a: 12 } );
   } );
   db.transCommit();

   var actResult = dbcl.find();
   commCompareResults( actResult, [] );
}