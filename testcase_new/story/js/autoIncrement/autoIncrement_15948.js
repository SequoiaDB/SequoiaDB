/***************************************************************************
@Description : seqDB-15948:range分区表，shardKey同时为自增字段，不指定自增字段插入记录
@Modify list :
              2018-10-17  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var clName = COMMCLNAME + "_15948";
   var field = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 10;
   var acquireSize = 1;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      ShardingType: "range", ShardingKey: { id: 1 }, Group: dataGroupNames[0],
      AutoIncrement: { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize }
   } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expIncrementArr = [{ Field: "id", SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize };
   checkSequence( db, sequenceName, expSequenceObj );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], { id: 50 } );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i, b: i } );
      expR.push( { a: i, b: i, id: 1 + i } );
   }
   dbcl.insert( doc );

   checkCountFromNode( db, dataGroupNames[0], COMMCSNAME, clName, 49 );
   checkCountFromNode( db, dataGroupNames[1], COMMCSNAME, clName, 51 );
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
