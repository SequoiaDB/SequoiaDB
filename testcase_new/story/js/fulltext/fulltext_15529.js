/******************************************************************************
@Description :    seqDB-15529:指定_id字段插入记录，并更新
@Modify list :   2018-9-30  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var esOperator = new ESOperator();
   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_15529";
   var queryCond = '{ "query" : { "match_all" : {} }, "size" : "20" }';
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15529";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateToSameTypeAndSuppot( dbcl );
   dbcl.insert( { a: "new1" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 11 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateToSameTypeButNotSuppot( dbcl );
   dbcl.insert( { a: "new2" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 12 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateToDifferTypeAndSuppot( dbcl );
   dbcl.insert( { a: "new3" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 17 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   updateToDifferTypeNotSuppot( dbcl );
   dbcl.insert( { a: "new4" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: 1, a: "a0" } );
   dbcl.insert( { _id: { "$numberLong": "4354" }, a: "a1" } );
   dbcl.insert( { _id: 123.456, a: "a2" } );
   dbcl.insert( { _id: { $decimal: "156.456" }, a: "a3" } );
   dbcl.insert( { _id: "string", a: "a4" } );
   dbcl.insert( { _id: false, a: "a5" } );
   dbcl.insert( { _id: { "$date": "2012-01-01" }, a: "a6" } );
   dbcl.insert( { _id: { "$timestamp": "2012-01-01-13.14.26.124233" }, a: "a7" } );
   dbcl.insert( { _id: { "key": "value" }, a: "a8" } );
   dbcl.insert( { _id: { "$oid": "5156c192f970aed30c020000" }, a: "a9" } );
   dbcl.insert( { _id: { "$minKey": 1 }, a: "a10" } );
   dbcl.insert( { _id: { "$maxKey": 1 }, a: "a11" } );
   dbcl.insert( { _id: null, a: "a12" } );
   dbcl.insert( { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, a: "a13" } );
}

function updateToSameTypeAndSuppot ( dbcl )
{
   dbcl.update( { $set: { _id: 56, } }, { a: "a0" } );
   dbcl.update( { $set: { _id: { "$numberLong": "343545" } } }, { a: "a1" } );
   dbcl.update( { $set: { _id: 43523.3452 } }, { a: "a2" } );
   dbcl.update( { $set: { _id: { $decimal: "46423.4524" } } }, { a: "a3" } );
   dbcl.update( { $set: { _id: "dasfjldsl" } }, { a: "a4" } );
   dbcl.update( { $set: { _id: true } }, { a: "a5" } );
   dbcl.update( { $set: { _id: { "$date": "2018-10-01" } } }, { a: "a6" } );
   dbcl.update( { $set: { _id: { "$timestamp": "2018-02-01-13.14.26.144233" } } }, { a: "a7" } );
   dbcl.update( { $set: { _id: { "obj": "value" } } }, { a: "a8" } );
   dbcl.update( { $set: { _id: { "$oid": "5156c192f970aed30c020100" } } }, { a: "a9" } );
}

function updateToSameTypeButNotSuppot ( dbcl )
{
   dbcl.update( { $set: { _id: { "$binary": "aDDsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a13" } );
}

function updateToDifferTypeAndSuppot ( dbcl )
{
   dbcl.update( { $set: { _id: false } }, { a: "a0" } );
   dbcl.update( { $set: { _id: 1 } }, { a: "a1" } );
   dbcl.update( { $set: { _id: 2 } }, { a: "a2" } );
   dbcl.update( { $set: { _id: 3 } }, { a: "a3" } );
   dbcl.update( { $set: { _id: 4 } }, { a: "a4" } );
   dbcl.update( { $set: { _id: 5 } }, { a: "a5" } );
   dbcl.update( { $set: { _id: 6 } }, { a: "a6" } );
   dbcl.update( { $set: { _id: 7 } }, { a: "a7" } );
   dbcl.update( { $set: { _id: 8 } }, { a: "a8" } );
   dbcl.update( { $set: { _id: 9 } }, { a: "a9" } );
   dbcl.update( { $set: { _id: 10 } }, { a: "a10" } );
   dbcl.update( { $set: { _id: 11 } }, { a: "a11" } );
   dbcl.update( { $set: { _id: 12 } }, { a: "a12" } );
   dbcl.update( { $set: { _id: 13 } }, { a: "a13" } );
}

function updateToDifferTypeNotSuppot ( dbcl )
{
   dbcl.update( { $set: { _id: { "$minKey": 1 } } }, { a: "a0" } );
   dbcl.update( { $set: { _id: { "$maxKey": 1 } } }, { a: "a1" } );
   dbcl.update( { $set: { _id: null } }, { a: "a2" } );
   dbcl.update( { $set: { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a3" } );
   dbcl.update( { $set: { _id: { "$binary": "ahksbG8gd29ybGQ=", "$type": "1" } } }, { a: "a4" } );
   dbcl.update( { $set: { _id: { "$binary": "agfsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a5" } );
   dbcl.update( { $set: { _id: { "$binary": "afssbG8gd29ybGQ=", "$type": "1" } } }, { a: "a6" } );
   dbcl.update( { $set: { _id: { "$binary": "aJFsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a7" } );
   dbcl.update( { $set: { _id: { "$binary": "aNNsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a8" } );
   dbcl.update( { $set: { _id: { "$binary": "aDDsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a9" } );
   dbcl.update( { $set: { _id: { "$binary": "aSSsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a10" } );
   dbcl.update( { $set: { _id: { "$binary": "aFFsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a11" } );
   dbcl.update( { $set: { _id: { "$binary": "aHHsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a12" } );
   dbcl.update( { $set: { _id: { "$binary": "aJJsbG8gd29ybGQ=", "$type": "1" } } }, { a: "a13" } );
}
