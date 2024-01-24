/***************************************************************************
@Description :seqDB-16002 :��������ʱ��ָ��catalogһ�λ������������������ֶ� 
@Modify list :
              2018-10-24  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_16002";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = -2;
   var cacheSize = 2147483647;
   var acquireSize = 1;
   var fieldName = "id";
   var minValue = -2147483647;
   var maxValue = 2147483647;
   var startValue = 0;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: startValue, MinValue: minValue, MaxValue: maxValue, StartValue: startValue };
   checkSequence( db, clSequenceName, expObj );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: startValue + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.dropAutoIncrement( fieldName );
   var increment = -3;
   var cacheSize = 2147483647;
   var acquireSize = 1;
   var fieldName = "id";
   var minValue = -2147483647;
   var maxValue = 2147483647;
   var startValue = 0;
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, MinValue: minValue, MaxValue: maxValue, StartValue: startValue } );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
