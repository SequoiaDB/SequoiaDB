/******************************************************************************
 * @Description   : seqDB-23286:可翻转序列，第一次翻转后获取当前值
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

   var sequenceName = 's23286';

   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { StartValue: 1, MinValue: 1, MaxValue: 6, CacheSize: 2, AcquireSize: 2, Cycled: true } );
   s.getNextValue();
   var sequenceAttr = getSequenceAttr( db, sequenceName, { CurrentValue: '' } );
   var expObj = { CurrentValue: 2 };
   assert.equal( sequenceAttr, expObj );

   for( var i = 0; i < 6; i++ )
   {
      s.getNextValue();
   }
   var currentValue = s.getCurrentValue();
   assert.equal( currentValue, 1 );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { CurrentValue: '' } );
   var expObj = { CurrentValue: 2 };
   assert.equal( sequenceAttr, expObj );

   db.dropSequence( sequenceName );
}