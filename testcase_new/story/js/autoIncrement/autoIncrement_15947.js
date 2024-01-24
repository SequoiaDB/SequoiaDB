/***************************************************************************
@Description : seqDB-15947:hash分区表，shardKey同时为自增字段，不指定自增字段插入记录 
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

   var csName = COMMCSNAME + "_15947";
   var clName = COMMCLNAME + "_15947";
   var field = "id";
   var domainName = "domain_15947";
   commDropCS( db, csName );
   commDropDomain( db, domainName );

   commCreateDomain( db, domainName, [dataGroupNames[0], dataGroupNames[1]], { AutoSplit: true } );
   commCreateCS( db, csName, null, null, { Domain: domainName } );
   var cacheSize = 10;
   var acquireSize = 1;
   var increment = 10
   var dbcl = commCreateCL( db, csName, clName, {
      ShardingKey: { id: 1 },
      AutoIncrement: { Field: field, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment }
   } );

   var clID = getCLID( db, csName, clName );
   var sequenceName = "SYS_" + clID + "_" + field + "_SEQ";
   var expIncrementArr = [{ Field: "id", SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, csName, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize, Increment: increment };
   checkSequence( db, sequenceName, expSequenceObj );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: i, b: i } );
      expR.push( { a: i, b: i, id: i * increment + 1 } );
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, csName );
   commDropDomain( db, domainName );
}
