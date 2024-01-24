/******************************************************************************
 * @Description   : seqDB-23194:修改序列属性接口参数校验
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

   var sequenceName = 's23194';
   dropSequence( db, sequenceName );
   var s = db.createSequence( sequenceName );

   //StartValue/ MinValue/MaxValue 参数校验
   s.setAttributes( { StartValue: { $numberLong: "-9223372036854775808" }, MinValue: { $numberLong: "-9223372036854775808" }, MaxValue: { $numberLong: "9223372036854775807" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { MaxValue: { $numberLong: "9223372036854775807" }, MinValue: { $numberLong: "-9223372036854775808" }, StartValue: { $numberLong: "-9223372036854775808" } };
   assert.equal( sequenceAttr, expObj );

   s.setAttributes( { StartValue: 0, MinValue: { $numberLong: "-9223372036854775808" }, MaxValue: { $numberLong: "9223372036854775807" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { MaxValue: { $numberLong: "9223372036854775807" }, MinValue: { $numberLong: "-9223372036854775808" }, StartValue: 0 };
   assert.equal( sequenceAttr, expObj );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { StartValue: 2.35 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { MinValue: 2.35, StartValue: 2.35 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { MaxValue: 2000.45 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { StartValue: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { MinValue: 'a', StartValue: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { MaxValue: 'a' } );
   } )

   //Increment参数校验
   s.setAttributes( { Increment: { $numberLong: "-1000" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Increment: '' } );
   var expObj = { Increment: -1000 };
   assert.equal( sequenceAttr, expObj );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { Increment: 0 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { Increment: 1.25 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { Increment: 'a' } );
   } )

   //CacheSize/AcquireSize参数校验
   s.setAttributes( { AcquireSize: { $numberLong: "100" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '' } );
   var expObj = { AcquireSize: 100 };
   assert.equal( sequenceAttr, expObj );

   s.setAttributes( { CacheSize: { $numberLong: "10000" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { CacheSize: '' } );
   var expObj = { CacheSize: 10000 };
   assert.equal( sequenceAttr, expObj );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { AcquireSize: -100 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { AcquireSize: -100, CacheSize: -1000 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { AcquireSize: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { CacheSize: 'a' } );
   } )

   //Cycled 参数校验
   s.setAttributes( { Cycled: true } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '' } );
   var expObj = { Cycled: true };
   assert.equal( sequenceAttr, expObj );

   s.setAttributes( { Cycled: false } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '' } );
   var expObj = { Cycled: false };
   assert.equal( sequenceAttr, expObj );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { Cycled: 1 } );
   } )

   //设置不支持的option 参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      s.setAttributes( { Cycle: true } );
   } )

   db.dropSequence( sequenceName );

}