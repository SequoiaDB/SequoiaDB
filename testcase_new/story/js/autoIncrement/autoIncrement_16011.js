/***************************************************************************
@Description : seqDB-16011:删除集合空间，将删除集合空间下所有的sequence
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

   var maincsName = COMMCSNAME + "_maincs_16011";
   var mainclName = COMMCLNAME + "_maincl_16011";

   var subcsName = COMMCSNAME + "_subcs_16011";
   var subclName1 = COMMCLNAME + "_subcl_16011_1";
   var subclName2 = COMMCLNAME + "_subcl_16011_2";
   var subclName3 = COMMCLNAME + "_subcl_16011_3";

   var clName1 = COMMCLNAME + "_cl_16011_1";
   var clName2 = COMMCLNAME + "_cl_16011_2";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var cl1;
   var cl2;

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
   cl1 = commCreateCL( db, maincsName, clName1, subclOption2 );
   var subclFieldName3 = "id4";
   var subclOption3 = { AutoIncrement: { Field: subclFieldName3 } };
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );
   cl2 = commCreateCL( db, maincsName, clName2, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );
   cl1.split( dataGroupNames[0], dataGroupNames[1], 50 );

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

   var clID = getCLID( db, maincsName, clName1 );
   var clSequenceName1 = "SYS_" + clID + "_" + subclFieldName2 + "_SEQ";
   var expIncrementArr = [{ Field: subclFieldName2, SequenceName: clSequenceName1 }];
   checkAutoIncrementonCL( db, maincsName, clName1, expIncrementArr );

   var clID = getCLID( db, maincsName, clName2 );
   var clSequenceName2 = "SYS_" + clID + "_" + subclFieldName3 + "_SEQ";
   var expIncrementArr = [{ Field: subclFieldName3, SequenceName: clSequenceName2 }];
   checkAutoIncrementonCL( db, maincsName, clName2, expIncrementArr );

   var mainExpSequenceObj = {
      Increment: increment, StartValue: startValue, MinValue: minValue, MaxValue: maxValue, CacheSize: cacheSize,
      AcquireSize: acquireSize, Cycled: cycled, CurrentValue: startValue
   };
   checkSequence( db, mainclSequenceName, mainExpSequenceObj );
   checkSequence( db, subclSequenceName1, {} );
   checkSequence( db, subclSequenceName2, {} );
   checkSequence( db, subclSequenceName3, {} );
   checkSequence( db, clSequenceName1, {} );
   checkSequence( db, clSequenceName2, {} );

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

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: sortField, a1: i, b: "cl1" } );
      expR.push( { a: sortField, a1: i, b: "cl1", id3: 1 + i } );
      sortField++;
   }
   cl1.insert( doc );
   var actR = cl1.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   var expR = [];
   for( var i = 0; i < 100; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id4: 1 + i } );
      sortField++;
   }
   cl2.insert( doc );
   var actR = cl2.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, maincsName );
   checkSequence( db, subclSequenceName2, {} );
   checkSequence( db, subclSequenceName3, {} );

   var mainclSequenceNum = db.snapshot( 15, { Name: mainclSequenceName } ).toArray().length;
   var subclSequenceNum1 = db.snapshot( 15, { Name: subclSequenceName1 } ).toArray().length;
   var clSequenceNum1 = db.snapshot( 15, { Name: clSequenceName1 } ).toArray().length;
   var clSequenceNum2 = db.snapshot( 15, { Name: clSequenceName2 } ).toArray().length;
   if( mainclSequenceNum !== 0 || subclSequenceNum1 !== 0 || clSequenceNum1 !== 0 || clSequenceNum2 !== 0 )
   {
      throw new Error( "SEQUENCE_NOT_DROP" );
   }

   commDropCS( db, subcsName );
}
