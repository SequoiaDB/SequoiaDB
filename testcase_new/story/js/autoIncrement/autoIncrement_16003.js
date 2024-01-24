/***************************************************************************
@Description :seqDB-16003 :��������ʱ��ָ��coordһ�λ�ȡ���������������ֶ� 
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

   var clName = COMMCLNAME + "_16003";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 11;
   var cacheSize = 1000;
   var acquireSize = 10;
   var fieldName = "id";
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIncrement: { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, clSequenceName, expObj );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 1000;
   var acquireSize = 11;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, clSequenceName, expObj );

   var doc = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 111;
   var acquireSize = 111;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
   var expArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db,  COMMCSNAME, clName, expArr );

   var expObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, clSequenceName, expObj );

   var doc = [];
   for( var i = 0; i < 2000; i++ )
   {
      doc.push( { a: sortField } );
      expR.push( { a: sortField, id: 1 + increment * i } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   dbcl.dropAutoIncrement( fieldName );
   var increment = 10;
   var cacheSize = 111;
   var acquireSize = 112;

   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );
   } );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
