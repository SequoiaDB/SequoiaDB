/******************************************************************************
 * @Description   : seqDB-23199:获取多个序列值接口参数校验
 * @Author        : zhaoyu
 * @CreateTime    : 2021.01.11
 * @LastEditTime  : 2021.01.11
 * @LastEditors   : zhaoyu
 ******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   var sequenceName = 's23199';
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.fetch( -1 );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.fetch( 2.35 );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.fetch( 'a' );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.fetch( { $numberLong: '100' } );
   } )

   db.dropSequence( sequenceName );

}