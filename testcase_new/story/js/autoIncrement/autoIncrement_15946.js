/***************************************************************************
@Description : seqDB-15945:创建range分区集合，创建自增字段，不指定自增字段插入记录 
@Modify list :
              2018-10-17  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   var dataGroupNames = commGetDataGroupNames( db );
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_15946";
   var field = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var increment = 10
   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      ShardingType: "range", ShardingKey: { a: 1 }, Group: dataGroupNames[0],
      AutoIncrement: { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment }
   } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expIncrementArr = [{ Field: "id", SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize, Increment: increment };
   checkSequence( db, sequenceName, expSequenceObj );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a1: sortField, a: i, b: i } );
      expR.push( { a1: sortField, a: i, b: i, id: i * increment + 1 } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a1: 1 } );
   checkRec( actR, expR );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], { a: 50 } );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a1: sortField, a: i, b: i } );
      expR.push( { a1: sortField, a: i, b: i, id: ( 100 + i ) * increment + 1 } );
      sortField++;
   }
   dbcl.insert( doc );

   checkCountFromNode( db, dataGroupNames[0], COMMCSNAME, clName, 100 );
   checkCountFromNode( db, dataGroupNames[1], COMMCSNAME, clName, 100 );

   var actR = dbcl.find().sort( { a1: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
