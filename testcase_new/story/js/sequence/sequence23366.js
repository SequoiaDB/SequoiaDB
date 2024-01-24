/******************************************************************************
 * @Description   : seqDB-23366:未使用的序列设置序列当前值
 * @Author        : zhaoyu
 * @CreateTime    : 2021.01.14
 * @LastEditTime  : 2021.01.14
 * @LastEditors   : zhaoyu
 ******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   };

   //正序序列
   var sequenceName = 's23366';
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName );

   //序列未使用，设置currentValue
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      s.setCurrentValue( 1 );
   } )
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '', CacheSize: '', CurrentValue: '', Initial: '' } );
   var expObj = { AcquireSize: 1000, CacheSize: 1000, CurrentValue: 1000, Initial: false };
   assert.equal( sequenceAttr, expObj );

   s.setCurrentValue( 1001 );
   var nextValue = s.getNextValue();
   assert.equal( nextValue, 1002 );

   db.dropSequence( sequenceName );

   //逆序序列
   //序列未使用，设置currentValue
   var s = db.createSequence( sequenceName, { Increment: -1 } );
   assert.tryThrow( SDB_SEQUENCE_VALUE_USED, function()
   {
      s.setCurrentValue( -1 );
   } )
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '', CacheSize: '', CurrentValue: '', Initial: '' } );
   var expObj = { AcquireSize: 1000, CacheSize: 1000, CurrentValue: -1000, Initial: false };
   assert.equal( sequenceAttr, expObj );

   s.setCurrentValue( -1001 );
   var nextValue = s.getNextValue();
   assert.equal( nextValue, -1002 );

   db.dropSequence( sequenceName );

}