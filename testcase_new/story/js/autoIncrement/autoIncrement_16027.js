/***************************************************************************
@Description : seqDB-16027:主子表修改主表自增字段属性
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

   var maincsName = COMMCSNAME + "_maincs_16027";
   var mainclName = COMMCLNAME + "_maincl_16027";

   var subcsName = COMMCSNAME + "_subcs_16027";
   var subclName1 = COMMCLNAME + "_subcl_16027_1";
   var subclName2 = COMMCLNAME + "_subcl_16027_2";
   var subclName3 = COMMCLNAME + "_subcl_16027_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var fieldName = "id";

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var cacheSize = 20;
   var acquireSize = 11;
   var increment = 10;
   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 }, ShardingType: "range", AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment } };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0] };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = {};
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a: 0 }, UpBound: { a: 20 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a: 20 }, UpBound: { a: 40 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a: 40 }, UpBound: { a: 6000 } } );

   var mainclID = getCLID( db, maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementArr );

   var expSequenceObj = { AcquireSize: acquireSize, CacheSize: cacheSize, Increment: increment };
   checkSequence( db, mainclSequenceName, expSequenceObj );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( mainclName );
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField, a0: i } );
         expR.push( { a: sortField, a0: i, id: 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }

   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   /*每个coord插入100条记录，需要从catalog上取acquireSizeNum = Math.ceil(100 /acquireSize)次序列值，多个coord总共取coordNum*acquireSizeNum
     再计算这么多序列值，catalog需要生成多少次，再加上初始值*/
   var currentValue = Math.ceil( Math.ceil( 100 / acquireSize ) * coordNum * acquireSize / cacheSize ) * cacheSize * increment + 1 - increment;

   //插入coordNum*100条记录，coord获取的序列值未用完，alter后会清空coord上未用完的序列值
   var nextValue = Math.ceil( 100 / acquireSize ) * acquireSize * coordNum * increment + 1;
   var cacheSize = 32;
   var acquireSize = 12;
   maincl.setAttributes( { AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize } } );
   var clID = getCLID( db, maincsName, mainclName );
   var clSequenceName = "SYS_" + clID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementArr );

   var clExpSequenceObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: currentValue };
   checkSequence( db, clSequenceName, clExpSequenceObj );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( mainclName );

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
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
