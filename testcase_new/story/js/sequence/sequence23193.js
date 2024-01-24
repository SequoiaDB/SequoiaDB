/******************************************************************************
 * @Description   : seqDB-23193:创建序列接口参数校验
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

   var sequenceName = 's23193';
   dropSequence( db, sequenceName );

   //默认值校验
   db.createSequence( sequenceName );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '', CacheSize: '', CurrentValue: '', Cycled: '', CycledCount: '', Increment: '', Initial: '', Internal: '', MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { AcquireSize: 1000, CacheSize: 1000, CurrentValue: 1, Cycled: false, CycledCount: 0, Increment: 1, Initial: true, Internal: false, MaxValue: { $numberLong: "9223372036854775807" }, MinValue: 1, StartValue: 1 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   //传入参数值校验
   db.createSequence( sequenceName, { AcquireSize: 1, CacheSize: 2000, Cycled: true, Increment: 5, MaxValue: 100000, MinValue: 5000, StartValue: 8000 } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '', CacheSize: '', CurrentValue: '', Cycled: '', CycledCount: '', Increment: '', Initial: '', Internal: '', MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { AcquireSize: 1, CacheSize: 2000, CurrentValue: 8000, Cycled: true, CycledCount: 0, Increment: 5, Initial: true, Internal: false, MaxValue: 100000, MinValue: 5000, StartValue: 8000 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   //sequenceName参数校验
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( "$s1" );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( "SYS1" );
   } )

   var name = "序列";
   dropSequence( db, name );
   db.createSequence( name );
   var s = db.getSequence( name );
   var nextValue = s.getNextValue();
   assert.equal( nextValue, 1 );

   //StartValue/ MinValue/MaxValue 参数校验
   db.createSequence( sequenceName, { StartValue: { $numberLong: "-9223372036854775808" }, MinValue: { $numberLong: "-9223372036854775808" }, MaxValue: { $numberLong: "9223372036854775807" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { MaxValue: { $numberLong: "9223372036854775807" }, MinValue: { $numberLong: "-9223372036854775808" }, StartValue: { $numberLong: "-9223372036854775808" } };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   db.createSequence( sequenceName, { StartValue: 0, MinValue: { $numberLong: "-9223372036854775808" }, MaxValue: { $numberLong: "9223372036854775807" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { MaxValue: '', MinValue: '', StartValue: '' } );
   var expObj = { MaxValue: { $numberLong: "9223372036854775807" }, MinValue: { $numberLong: "-9223372036854775808" }, StartValue: 0 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { StartValue: 2.35 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { MinValue: 2.35, StartValue: 2.35 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { MaxValue: 2000.45 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { StartValue: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { MinValue: 'a', StartValue: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { MaxValue: 'a' } );
   } )

   //Increment参数校验
   db.createSequence( sequenceName, { Increment: { $numberLong: "-1000" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Increment: '' } );
   var expObj = { Increment: -1000 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { Increment: 0 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { Increment: 1.25 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { Increment: 'a' } );
   } )

   //CacheSize/AcquireSize参数校验
   db.createSequence( sequenceName, { AcquireSize: { $numberLong: "100" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { AcquireSize: '' } );
   var expObj = { AcquireSize: 100 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   db.createSequence( sequenceName, { CacheSize: { $numberLong: "10000" } } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { CacheSize: '' } );
   var expObj = { CacheSize: 10000 };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { AcquireSize: -100 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { AcquireSize: -100, CacheSize: -1000 } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { AcquireSize: 'a' } );
   } )

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { CacheSize: 'a' } );
   } )

   //Cycled 参数校验
   db.createSequence( sequenceName, { Cycled: true } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '' } );
   var expObj = { Cycled: true };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   db.createSequence( sequenceName, { Cycled: false } );
   var sequenceAttr = getSequenceAttr( db, sequenceName, { Cycled: '' } );
   var expObj = { Cycled: false };
   assert.equal( sequenceAttr, expObj );
   db.dropSequence( sequenceName );

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { Cycled: 1 } );
   } )

   //指定不支持的option 参数
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      db.createSequence( sequenceName, { Cycle: true } );
   } )

}