/******************************************************************************
 * @Description   : seqDB-32896 :: usercacheinterval配置参数校验
 * @Author        : Wang Xingming
 * @CreateTime    : 2023.09.05
 * @LastEditTime  : 2023.09.05
 * @LastEditors   : Wang Xingming
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   // 修改usercacheinterval配置，指定不支持的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { usercacheinterval: -1 } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { usercacheinterval: "string" } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { usercacheinterval: true } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { usercacheinterval: ['array'] } );
   } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( { usercacheinterval: { other: 1 } } );
   } );

   if( commIsArmArchitecture() == false )
   {
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         db.updateConf( { usercacheinterval: 2147483648 } );
      } );
   }

   // 修改usercacheinterval配置，指定支持的参数
   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      db.updateConf( { usercacheinterval: 2147483647 } );
   } );

   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      db.updateConf( { usercacheinterval: 0 } );
   } );

   assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
   {
      db.updateConf( { usercacheinterval: 0 } );
   } );
}
