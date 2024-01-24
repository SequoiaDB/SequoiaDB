/***************************************************************************
@Description : seqDB-16025:hash分区表修改自增字段属性 
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

   var csName = COMMCSNAME + "_16025";
   var clName = COMMCLNAME + "_16025";
   var fieldName = "id";
   var domainName = "domain_16025";
   commDropCS( db, csName );
   commDropDomain( db, domainName );
   commCreateDomain( db, domainName, [dataGroupNames[0], dataGroupNames[1]], { AutoSplit: true } );
   commCreateCS( db, csName, null, null, { Domain: domainName } );
   var cacheSize = 20;
   var acquireSize = 11;
   var increment = 10;
   var dbcl = commCreateCL( db, csName, clName, {
      ShardingKey: { id: 1 },
      AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment }
   } );

   var clID = getCLID( db, csName, clName );
   var sequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: sequenceName }];
   checkAutoIncrementonCL( db, csName, clName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize, Increment: increment };
   checkSequence( db, sequenceName, expSequenceObj );

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
   var clID = getCLID( db, csName, clName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db, csName, clName, expIncrementArr );

   var clExpSequenceObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: currentValue };
   checkSequence( db, clSequenceName, clExpSequenceObj );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( csName ).getCL( clName );

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

   commDropCS( db, csName );
   commDropDomain( db, domainName );
}
