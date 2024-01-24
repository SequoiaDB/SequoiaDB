/******************************************************************************
 * @Description   : seqDB-28264:updateConf配置参数的无效类型校验
 * @Author        : Xu Mingxing
 * @CreateTime    : 2022.11.09
 * @LastEditTime  : 2022.11.10
 * @LastEditors   : Xu Mingxing
 ******************************************************************************/
main( test );

function test ()
{
   // 更新Number类型配置参数,例如: transreplsize
   // 1.指定无效参数类型,例如: String类型
   var config = { transreplsize: "M" };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( config );
   } );
   // 2.指定无效参数类型,例如: Boolean类型
   var config = { transreplsize: true };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( config );
   } );
   // 3.指定无效参数类型,例如: Array类型
   var config = { transreplsize: [1, 2, 3] };
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.updateConf( config );
   } );
}