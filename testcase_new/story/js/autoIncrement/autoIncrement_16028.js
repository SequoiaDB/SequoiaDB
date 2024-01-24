/***************************************************************************
@Description : seqDB-16028:主子表区表修改子表自增字段属性
@Modify list :
              2018-10-26  zhaoyu  Create
****************************************************************************/
var sortField = 0;
main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db );
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var maincsName = COMMCSNAME + "_maincs_16028";
   var mainclName = COMMCLNAME + "_maincl_16028";

   var subcsName = COMMCSNAME + "_subcs_16028";
   var subclName1 = COMMCLNAME + "_subcl_16028_1";
   var subclName2 = COMMCLNAME + "_subcl_16028_2";
   var subclName3 = COMMCLNAME + "_subcl_16028_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var fieldName1 = "id1";
   var fieldName2 = "id2";

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 }, ShardingType: "range" };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0] };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = {};
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 1000 }, { a0: 2000 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a: 0 }, UpBound: { a: 2000 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a: 2000 }, UpBound: { a: 4000 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a: 4000 }, UpBound: { a: 6000 } } );

   var cacheSize = 20;
   var acquireSize = 11;
   var increment = 10;
   maincl.createAutoIncrement( { Field: fieldName1, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment } );
   subcl1.createAutoIncrement( { Field: fieldName2, CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment } );

   var mainclID = getCLID( db, maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + fieldName1 + "_SEQ";
   var expIncrementArr = [{ Field: fieldName1, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementArr );

   var subclID = getCLID( db, maincsName, subclName1 );
   var subclSequenceName = "SYS_" + subclID + "_" + fieldName2 + "_SEQ";
   var expIncrementArr = [{ Field: fieldName2, SequenceName: subclSequenceName }];
   checkAutoIncrementonCL( db, maincsName, subclName1, expIncrementArr );

   var mainExpSequenceObj = { CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment };
   checkSequence( db, mainclSequenceName, mainExpSequenceObj );
   var subExpSequenceObj = { CacheSize: cacheSize, AcquireSize: acquireSize, Increment: increment };
   checkSequence( db, subclSequenceName, subExpSequenceObj );

   var coordNodes = getCoordNodeNames( db );
   var coordNum = coordNodes.length;
   var expR = [];
   var mainclCoordCurrentvalue = [];
   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( mainclName );
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField, a0: i } );
         expR.push( { a: sortField, a0: i, id1: 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
      //第1个coord，插入99条后，自增字段值为：1、1+10、1+10*2-->1+10*99+10
      mainclCoordCurrentvalue.push( 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i );

   }

   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( subclName1 );
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField, a0: i } );
         expR.push( { a: sortField, a0: i, id2: 1 + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = subcl1.find().sort( { a: 1 } );
   checkRec( actR, expR );

   /*每个coord插入100条记录，需要从catalog上取acquireSizeNum = Math.ceil(100 /acquireSize)次序列值，多个coord总共取coordNum*acquireSizeNum
     再计算这么多序列值，catalog需要生成多少次，再加上初始值*/
   var currentValue = Math.ceil( Math.ceil( 100 / acquireSize ) * coordNum * acquireSize / cacheSize ) * cacheSize * increment + 1 - increment;

   //插入coordNum*100条记录，coord获取的序列值未用完，alter后会清空coord上未用完的序列值
   var nextValue = Math.ceil( 100 / acquireSize ) * acquireSize * coordNum * increment + 1;
   var cacheSize = 32;
   var acquireSize = 12;
   subcl1.setAttributes( { AutoIncrement: { Field: fieldName2, CacheSize: cacheSize, AcquireSize: acquireSize } } );
   var clID = getCLID( db, maincsName, subclName1 );
   var clSequenceName = "SYS_" + clID + "_" + fieldName2 + "_SEQ";
   var expIncrementArr = [{ Field: fieldName2, SequenceName: clSequenceName }];
   checkAutoIncrementonCL( db, maincsName, subclName1, expIncrementArr );

   var clExpSequenceObj = { Increment: increment, CacheSize: cacheSize, AcquireSize: acquireSize, CurrentValue: currentValue };
   checkSequence( db, clSequenceName, clExpSequenceObj );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( mainclName );
      var doc = [];
      cl.insert( { a: sortField } );
      expR.push( { a: sortField, id1: mainclCoordCurrentvalue[k] } );
      sortField++;
      coord.close();
   }
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   for( var k = 0; k < coordNum; k++ )
   {
      var coord = new Sdb( coordNodes[k] );
      var cl = coord.getCS( maincsName ).getCL( subclName1 );
      //alter操作会变更集合版本号，插入时会取2次seqence值，SEQUOIADBMAINSTREAM-3895,通过find操作更新版本号
      var cursor = cl.find();
      while( cursor.next() ) { }
      var doc = [];
      for( var i = 0; i < 100; i++ )
      {
         doc.push( { a: sortField } );
         expR.push( { a: sortField, id2: nextValue + Math.ceil( 100 / acquireSize ) * acquireSize * increment * k + increment * i } );
         sortField++;
      }
      cl.insert( doc );
      coord.close();
   }
   var actR = subcl1.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
