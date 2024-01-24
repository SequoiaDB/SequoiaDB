/***************************************************************************
@Description : seqDB-16010:主子表中存在自增字段，删除子表 
@Modify list :
              2018-10-25  zhaoyu  Create
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

   var maincsName = COMMCSNAME + "_maincs_16010";
   var mainclName = COMMCLNAME + "_maincl_16010";

   var subcsName = COMMCSNAME + "_subcs_16010";
   var subclName1 = COMMCLNAME + "_subcl_16010_1";
   var subclName2 = COMMCLNAME + "_subcl_16010_2";
   var subclName3 = COMMCLNAME + "_subcl_16010_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var mainclFieldName = "id1";
   var increment = 10;
   var startValue = 100;
   var minValue = -100;
   var maxValue = 1000000;
   var cacheSize = 32;
   var acquireSize = 10;
   var cycled = true;
   var generated = "strict";
   var mainclOption = {
      IsMainCL: true, ShardingKey: { "a1": 1 }, ShardingType: "range", AutoIncrement: {
         Field: mainclFieldName, Increment: increment,
         StartValue: startValue, MinValue: minValue, MaxValue: maxValue, CacheSize: cacheSize, AcquireSize: acquireSize, Cycled: cycled,
         Generated: generated
      }
   };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclFieldName1 = "id2";
   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0], AutoIncrement: { Field: subclFieldName1 } };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclFieldName2 = "id3";
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0], AutoIncrement: { Field: subclFieldName2 } };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclFieldName3 = "id3";
   var subclOption3 = { AutoIncrement: { Field: subclFieldName3 } };
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a1: 0 }, UpBound: { a1: 20 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a1: 20 }, UpBound: { a1: 40 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a1: 40 }, UpBound: { a1: 6000 } } );

   var mainclID = getCLID( db, maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + mainclFieldName + "_SEQ";
   var expIncrementArr = [{ Field: mainclFieldName, SequenceName: mainclSequenceName, Generated: generated }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementArr );

   var subclID = getCLID( db, maincsName, subclName1 );
   var subclSequenceName1 = "SYS_" + subclID + "_" + subclFieldName1 + "_SEQ";
   var expIncrementArr = [{ Field: subclFieldName1, SequenceName: subclSequenceName1 }];
   checkAutoIncrementonCL( db, maincsName, subclName1, expIncrementArr );

   var subclID = getCLID( db, subcsName, subclName2 );
   var subclSequenceName2 = "SYS_" + subclID + "_" + subclFieldName2 + "_SEQ";
   var expIncrementArr = [{ Field: subclFieldName2, SequenceName: subclSequenceName2 }];
   checkAutoIncrementonCL( db, subcsName, subclName2, expIncrementArr );

   var subclID = getCLID( db, subcsName, subclName3 );
   var subclSequenceName3 = "SYS_" + subclID + "_" + subclFieldName3 + "_SEQ";
   var expIncrementArr = [{ Field: subclFieldName3, SequenceName: subclSequenceName3 }];
   checkAutoIncrementonCL( db, subcsName, subclName3, expIncrementArr );

   var mainExpSequenceObj = {
      Increment: increment, StartValue: startValue, MinValue: minValue, MaxValue: maxValue, CacheSize: cacheSize,
      AcquireSize: acquireSize, Cycled: cycled, CurrentValue: startValue
   };
   checkSequence( db, mainclSequenceName, mainExpSequenceObj );
   checkSequence( db, subclSequenceName1, {} );
   checkSequence( db, subclSequenceName2, {} );
   checkSequence( db, subclSequenceName3, {} );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id1: startValue + increment * i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCL( db, maincsName, subclName1 );
   var mainExpSequenceObj = {
      Increment: increment, StartValue: startValue, MinValue: minValue, MaxValue: maxValue, CacheSize: cacheSize,
      AcquireSize: acquireSize, Cycled: cycled, CurrentValue: startValue + Math.ceil( 100 / cacheSize ) * cacheSize * increment - increment
   };
   checkSequence( db, mainclSequenceName, mainExpSequenceObj );
   var subclSequenceNum1 = db.snapshot( 15, { Name: subclSequenceName1 } ).toArray().length;
   if( subclSequenceNum1 !== 0 )
   {
      throw new Error( "SEQUENCE_NOT_DROP" );
   };
   checkSequence( db, subclSequenceName2, {} );
   checkSequence( db, subclSequenceName3, {} );

   var doc = [];
   var expR = [];
   for( var i = 100; i < 200; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id1: startValue + increment * i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find( { a1: { $gte: 100 } } ).sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   var expR = [];
   var j = 0;
   for( var i = 200; i < 300; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id3: 1 + j } );
      sortField++;
      j++;
   }
   subcl3.insert( doc );
   var actR = maincl.find( { a1: { $gte: 200 } } ).sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
