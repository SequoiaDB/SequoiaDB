/******************************************************************************
 * @Description   : seqDB-23198:设置当前值接口参数校验
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

   var sequenceName = 's23198';
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName, { Increment: -1 } );
   s.setCurrentValue( -100 );

   s.setAttributes( { Increment: 1, MinValue: -100, StartValue: -100, MaxValue: { $numberLong: '9223372036854775807' } } );
   s.setCurrentValue( 0 );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setCurrentValue( 3.6 );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setCurrentValue( 'a' );
   } )

   s.setCurrentValue( NumberLong( '21474836470' ) );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { CurrentValue: '' } );
   var expObj = { CurrentValue: 21474836470 };
   assert.equal( sequenceAttr, expObj );

   s.setAttributes( { MinValue: { $numberLong: '-9223372036854775808' } } );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setCurrentValue( { $decimal: '-9223372036854775809' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setCurrentValue( { $decimal: '9223372036854775808' } );
   } )

   db.dropSequence( sequenceName );

}