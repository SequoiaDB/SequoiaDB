/******************************************************************************
 * @Description   : seqDB-23277:restart接口参数校验
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

   var sequenceName = 's23277';
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName, { MinValue: -1000 } );

   s.restart( -1 );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.restart( 2.35 );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.restart( 'a' );
   } )

   s.restart( NumberLong( '2147483648' ) );

   db.dropSequence( sequenceName );

}
