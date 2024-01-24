/***************************************************************************
@Description : seqDB-15949:创建主表时，创建多个自增字段，连接同一个coord不指定自增字段插入记录
@Modify list :
              2018-10-18  zhaoyu  Create
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

   var maincsName = COMMCSNAME + "_maincs_15949";
   var mainclName = COMMCLNAME + "_maincl_15949";

   var subcsName = COMMCSNAME + "_subcs_15949";
   var subclName1 = COMMCLNAME + "_subcl_15949_1";
   var subclName2 = COMMCLNAME + "_subcl_15949_2";
   var subclName3 = COMMCLNAME + "_subcl_15949_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var fieldNames = ["id1", "id2"];

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var cacheSize = 10;
   var acquireSize = 1;
   var mainclOption = { IsMainCL: true, ShardingKey: { "a1": 1 }, ShardingType: "range", AutoIncrement: [{ Field: fieldNames[0], CacheSize: cacheSize, AcquireSize: acquireSize }, { Field: fieldNames[1], CacheSize: cacheSize, AcquireSize: acquireSize }] };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0], AutoIncrement: { Field: fieldNames[0], Increment: 10 } };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0], AutoIncrement: { Field: fieldNames[0], StartValue: 1000 } };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = { Group: dataGroupNames[0], AutoIncrement: { Field: fieldNames[0], StartValue: -1, MinValue: -1 } };
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a1: 1 }, UpBound: { a1: 21 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a1: 21 }, UpBound: { a1: 41 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a1: 41 }, UpBound: { a1: 61 } } );

   var mainclID = getCLID( db, maincsName, mainclName );
   var mainclSequenceName1 = "SYS_" + mainclID + "_" + fieldNames[0] + "_SEQ";
   var mainclSequenceName2 = "SYS_" + mainclID + "_" + fieldNames[1] + "_SEQ";
   var expIncrementArr = [{ Field: fieldNames[0], SequenceName: mainclSequenceName1 }, { Field: fieldNames[1], SequenceName: mainclSequenceName2 }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementArr );

   var expSequenceObj = { CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, mainclSequenceName1, expSequenceObj );

   var doc = [];
   var expR = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id1: i, id2: i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, a0: i } );
      expR.push( { a: sortField, a1: i, a0: i, id1: ( i - 1 ) * 10 + 1, } );
      sortField++;
   }
   subcl1.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
