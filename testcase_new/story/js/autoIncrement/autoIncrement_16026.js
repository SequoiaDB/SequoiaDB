/***************************************************************************
@Description : seqDB-16026:range分区表，shardKey同时为自增字段，不指定自增字段插入记录
@Modify list :
              2018-10-26  zhaoyu  Create
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

   var clName = COMMCLNAME + "_16026";
   var fieldName = "id";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var cacheSize = 20;
   var acquireSize = 11;
   var increment = 10;
   var dbcl = commCreateCL( db, COMMCSNAME, clName, {
      ShardingType: "range", ShardingKey: { id: 1 }, Group: dataGroupNames[0],
      AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment }
   } );

   var clID = getCLID( db, COMMCSNAME, clName );
   var sequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize, Increment: increment };
   checkSequence( db, sequenceName, expSequenceObj );

   dbcl.split( dataGroupNames[0], dataGroupNames[1], { id: 150 } );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: sortField, b: i } );
      expR.push( { a: sortField, b: i, id: i * increment + 1 } );
      sortField++;
   }
   dbcl.insert( doc );

   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   //插入100条记录，catalog上缓存序列生成了100/cacheSize次,且序列值已用完
   var currentValue = ( Math.ceil( 100 / cacheSize ) + 1 ) * cacheSize * increment + 1 - increment;

   //插入100条记录，coord获取的序列值未用完，alter后会清空coord上未用完的序列值
   var nextValue = Math.ceil( 100 / acquireSize ) * acquireSize * increment + 1;
   var cacheSize = 32;
   var acquireSize = 12;
   dbcl.setAttributes( { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize } } );
   var clID = getCLID( db, COMMCSNAME, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db, COMMCSNAME, clName, expIncrementArr );

   var clExpSequenceObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: currentValue };
   checkSequence( db, clSequenceName, clExpSequenceObj );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( COMMCSNAME ).getCL( clName );

      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id: nextValue + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = dbcl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, COMMCSNAME, clName, true, true );
}
