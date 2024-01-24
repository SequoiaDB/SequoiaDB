/******************************************************************************
 * @Description   : seqDB-23126:新创建的序列修改CurrentValue为原值，获取下一个序列值
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

   var sequenceName = 's23126';
   var increment = 1;

   dropSequence( db, sequenceName );

   var s = db.createSequence( sequenceName, { Increment: increment } );

   s.fetch( 10 );
   var currentValue = s.getCurrentValue();
   s.setAttributes( { CurrentValue: currentValue } );

   var nextValue = s.getNextValue();
   assert.equal( nextValue, currentValue + increment );

   db.dropSequence( sequenceName );
}