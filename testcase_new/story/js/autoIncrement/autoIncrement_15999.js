/***************************************************************************
@Description :seqDB-15999 :��������ʱ�����������������ֶ� 
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

   var clName = COMMCLNAME + "_15999";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var increment = 1;
   var cacheSize = 10;
   var acquireSize = 1;
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
   var increment = 2147483647;
   dbcl.createAutoIncrement( { Field: fieldName, Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize } );

   var clID = getCLID( db,  COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
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

   commDropCL( db, COMMCSNAME, clName, true, true );

}
