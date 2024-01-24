/******************************************************************************
 * @Description   : seqDB-24418:addUser、setUserConfigs、addGroup、delUser 参数校验
 * @Author        : liuli
 * @CreateTime    : 2021.10.11
 * @LastEditTime  : 2021.10.11
 * @LastEditors   : liuli
 ******************************************************************************/
main( test );

function test ()
{
   // addUser 指定不存在的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      System.addUser( { "sequoiadb": "sequoiadb" } );
   } );

   // setUserConfigs 指定不存在的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      System.setUserConfigs( { "sequoiadb": "sequoiadb" } );
   } );

   // addGroup 指定不存在的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      System.addGroup( { "sequoiadb": "sequoiadb" } );
   } );

   // delUser 指定不存在的参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      System.delUser( { "sequoiadb": "sequoiadb" } );
   } );
}