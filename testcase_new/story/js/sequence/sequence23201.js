/******************************************************************************
 * @Description   : seqDB-23201:序列循环/不循环功能验证 
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

   var sequenceName = 's23201';
   var increment = 1;
   var cycled = true;
   var maxValue = 100;

   dropSequence( db, sequenceName );

   //创建序列，检查序列快照
   var s = db.createSequence( sequenceName, { Increment: increment, Cycled: cycled, CacheSize: maxValue, AcquireSize: maxValue, MaxValue: maxValue } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '', Internal: '', CycledCount: '' } );
   var expObj = { Cycled: cycled, Internal: false, CycledCount: 0 };
   assert.equal( sequenceAttr, expObj );

   //序列被循环使用后，检查序列快照
   for( var i = 0; i < 10; i++ )
   {
      s.fetch( maxValue );
      var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '', Internal: '', CycledCount: '' } );
      var expObj = { Cycled: cycled, Internal: false, CycledCount: i };
      assert.equal( sequenceAttr, expObj );
   }

   //修改序列为不可循环使用
   var cycled = false;
   s.setAttributes( { Cycled: cycled } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '', Internal: '', CycledCount: '' } );
   var expObj = { Cycled: cycled, Internal: false, CycledCount: 0 };
   assert.equal( sequenceAttr, expObj );

   //序列不能循环使用
   assert.tryThrow( SDB_SEQUENCE_EXCEEDED, function()
   {
      s.getNextValue();
   } )
   assert.equal( sequenceAttr, expObj );

   db.dropSequence( sequenceName );
}