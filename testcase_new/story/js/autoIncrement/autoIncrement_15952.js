/***************************************************************************
@Description : seqDB-15952:子表分区键同时为自增字段时，不指定自增字段从主表插入记录
@Modify list :
              2018-10-19  zhaoyu  Create
****************************************************************************/
main( test );
function test ()
{
   var dataGroupNames = commGetDataGroupNames( db);
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var maincsName = COMMCSNAME + "_maincs_15952";
   var mainclName = COMMCLNAME + "_maincl_15952";

   var subcsName = COMMCSNAME + "_subcs_15952";
   var subclName1 = COMMCLNAME + "_subcl_15952_1";
   var subclName2 = COMMCLNAME + "_subcl_15952_2";
   var subclName3 = COMMCLNAME + "_subcl_15952_3";

   var mainclFullName = maincsName + "." + mainclName;
   var subclFullName1 = maincsName + "." + subclName1;
   var subclFullName2 = subcsName + "." + subclName2;
   var subclFullName3 = subcsName + "." + subclName3;

   var maincl;
   var subcl1;
   var subcl2;
   var subcl3;
   var fieldName = "a0";

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );

   var cacheSize = 10;
   var acquireSize = 1;
   var mainclOption = { IsMainCL: true, ShardingKey: { "a": 1 }, ShardingType: "range", AutoIncrement: { Field: fieldName, CacheSize: cacheSize, AcquireSize: acquireSize } };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0] };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = { Group: dataGroupNames[0] };
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 1000 }, { a0: 2000 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a: 1 }, UpBound: { a: 2001 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a: 2001 }, UpBound: { a: 4001 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a: 4001 }, UpBound: { a: 6001 } } );

   var mainclID = getCLID( db, maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + fieldName + "_SEQ";
   var expIncrementObj = [{ Field: fieldName, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( db, maincsName, mainclName, expIncrementObj );

   var expSequenceObj = { CacheSize: cacheSize, AcquireSize: acquireSize };
   checkSequence( db, mainclSequenceName, expSequenceObj );

   var doc = [];
   var expR = [];
   for( var i = 1; i < 6001; i++ )
   {
      doc.push( { a: i } );
      expR.push( { a: i, a0: i } );
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
