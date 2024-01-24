/******************************************************************************
 * @Description   : seqDB-24804:验证锁升级 session 配置项 TransAllowLockEscalation
 *                : seqDB-24805:验证锁升级 session 配置项 TransMaxLockNum
 *                : seqDB-24806:验证锁升级 session 配置项 TransMaxLogSpaceRatio
 * @Author        : liuli
 * @CreateTime    : 2021.12.15
 * @LastEditTime  : 2022.01.03
 * @LastEditors   : liuli
 ******************************************************************************/

main( test );
function test ()
{
   // TransAllowLockEscalation参数校验
   var sessionAttr = { TransAllowLockEscalation: false };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransAllowLockEscalation: true };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransAllowLockEscalation: 1 };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   var sessionAttr = { TransAllowLockEscalation: true };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransAllowLockEscalation: 0 };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransAllowLockEscalation: "true" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransAllowLockEscalation: -1 };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   // TransMaxLockNum参数校验
   var sessionAttr = { TransMaxLockNum: -1 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLockNum: 0 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLockNum: 100 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLockNum: Math.pow( 2, 31 ) - 1 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLockNum: Math.pow( 2, 31 ) };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLockNum: Math.pow( 2, 31 ) - 1 };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLockNum: -2 };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLockNum: -1 };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLockNum: true };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLockNum: "100" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLockNum: 100.1 };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLockNum: 100 };
   checkSnapshot( db, actSessionAttr );

   // TransMaxLogSpaceRatio参数校验
   var sessionAttr = { TransMaxLogSpaceRatio: 1 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: 10 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: 50 };
   db.setSessionAttr( sessionAttr );
   checkSnapshot( db, sessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: 0 };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLogSpaceRatio: 1 };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: 51 };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLogSpaceRatio: 50 };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: 10.1 };
   db.setSessionAttr( sessionAttr );
   var actSessionAttr = { TransMaxLogSpaceRatio: 10 };
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: true };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );

   var sessionAttr = { TransMaxLogSpaceRatio: "10" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.setSessionAttr( sessionAttr );
   } );
   checkSnapshot( db, actSessionAttr );
}

function checkSnapshot ( sdb, option )
{
   var obj = sdb.getSessionAttr().toObj();
   for( var key in option )
   {
      assert.equal( obj[key], option[key] );
   }
}