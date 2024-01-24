/***************************************************************************
@Description : seqDB-15989:主子表上创建/删除自增字段，删除主表自增字段
@Modify list :
              2018-10-18  zhaoyu  Create
****************************************************************************/

main( test );
function test ()
{
   var sortField = 0;
   var dataGroupNames = commGetDataGroupNames( db);
   if( commIsStandalone( db ) || dataGroupNames.length < 2 )
   {
      return;
   }

   var maincsName = COMMCSNAME + "_maincs_15989";
   var mainclName = COMMCLNAME + "_maincl_15989";

   var subcsName = COMMCSNAME + "_subcs_15989";
   var subclName1 = COMMCLNAME + "_subcl_15989_1";
   var subclName2 = COMMCLNAME + "_subcl_15989_2";
   var subclName3 = COMMCLNAME + "_subcl_15989_3";

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

   var mainclOption = { IsMainCL: true, ShardingKey: { "a1": 1 }, ShardingType: "range" };
   maincl = commCreateCL( db, maincsName, mainclName, mainclOption );

   var subclOption1 = { ShardingKey: { "a0": 1 }, ShardingType: "range", Group: dataGroupNames[0] };
   subcl1 = commCreateCL( db, maincsName, subclName1, subclOption1 );
   var subclOption2 = { ShardingKey: { "a0": 1 }, ShardingType: "hash", Group: dataGroupNames[0] };
   subcl2 = commCreateCL( db, subcsName, subclName2, subclOption2 );
   var subclOption3 = {};
   subcl3 = commCreateCL( db, subcsName, subclName3, subclOption3 );

   subcl1.split( dataGroupNames[0], dataGroupNames[1], { a0: 10 }, { a0: 20 } );
   subcl2.split( dataGroupNames[0], dataGroupNames[1], 50 );

   maincl.attachCL( subclFullName1, { LowBound: { a1: 1 }, UpBound: { a1: 21 } } );
   maincl.attachCL( subclFullName2, { LowBound: { a1: 21 }, UpBound: { a1: 41 } } );
   maincl.attachCL( subclFullName3, { LowBound: { a1: 41 }, UpBound: { a1: 61 } } );

   var mainclCacheSize = 10;
   var mainclAcquireSize = 1;
   var subclIncrement = 10;
   maincl.createAutoIncrement( { Field: fieldName, CacheSize: mainclCacheSize, AcquireSize: mainclAcquireSize } );
   subcl1.createAutoIncrement( { Field: fieldName, Increment: subclIncrement } );

   var mainclID = getCLID( db,  maincsName, mainclName );
   var mainclSequenceName = "SYS_" + mainclID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: mainclSequenceName }];
   checkAutoIncrementonCL( db,  maincsName, mainclName, expIncrementArr );

   var subclID = getCLID( db,  maincsName, subclName1 );
   var subclSequenceName = "SYS_" + subclID + "_" + fieldName + "_SEQ";
   var expIncrementArr = [{ Field: fieldName, SequenceName: subclSequenceName }];
   checkAutoIncrementonCL( db,  maincsName, subclName1, expIncrementArr );

   var mainExpSequenceObj = { CacheSize: mainclCacheSize, AcquireSize: mainclAcquireSize };
   checkSequence( db, mainclSequenceName, mainExpSequenceObj );
   var subExpSequenceObj = { Increment: subclIncrement };
   checkSequence( db, subclSequenceName, subExpSequenceObj );

   var doc = [];
   var expR = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i, id: i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, a0: i } );
      expR.push( { a: sortField, a1: i, a0: i, id: ( i - 1 ) * 10 + 1, } );
      sortField++;
   }
   subcl1.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   maincl.dropAutoIncrement( fieldName );

   var doc = [];
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, b: i } );
      expR.push( { a: sortField, a1: i, b: i } );
      sortField++;
   }
   maincl.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   var doc = [];
   var j = 61;
   for( var i = 1; i < 61; i++ )
   {
      doc.push( { a: sortField, a1: i, a0: i } );
      expR.push( { a: sortField, a1: i, a0: i, id: ( j - 1 ) * 10 + 1 } );
      sortField++;
      j++;
   }
   subcl1.insert( doc );
   var actR = maincl.find().sort( { a: 1 } );
   checkRec( actR, expR );

   commDropCS( db, subcsName );
   commDropCS( db, maincsName );
}
