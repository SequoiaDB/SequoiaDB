/******************************************************************************
@Description :   seqDB-15528:更新_id及全文索引字段
@Modify list :   2018-9-28  xiaoni Zhao  Init
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
   var clName = COMMCLNAME + "_ES_15528";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var queryCond = '{ "query" : { "match_all" : {} }, "size" : "20" }';
   var selectorCond = { a: "" };
   var textIndexName = "a_15528";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   insertData( dbcl );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   update( dbcl );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 7 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   update( dbcl );
   dbcl.insert( { a: "new" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 7 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   dbcl.insert( { _id: 1, a: "text1" } );
   dbcl.insert( { _id: 2, a: "text2" } );
   dbcl.insert( { _id: 3 } );
   dbcl.insert( { _id: 4 } );
   dbcl.insert( { _id: 5, a: "text5" } );
   dbcl.insert( { _id: 6, a: "text6" } );
   dbcl.insert( { _id: 7 } );
   dbcl.insert( { _id: 8 } );
   dbcl.insert( { _id: { "$minKey": 1 }, a: "text9" } );
   dbcl.insert( { _id: { "$maxKey": 1 }, a: "text10" } );
   dbcl.insert( { _id: null, b: 1 } );
   dbcl.insert( { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, b: 2 } );
   dbcl.insert( { _id: { "$binary": "aDDsbG8gd29ybGQ=", "$type": "1" }, a: "text13" } );
   dbcl.insert( { _id: { "$binary": "aFFsbG8gd29ybGQ=", "$type": "1" }, a: "text14" } );
   dbcl.insert( { _id: { "$binary": "aVVsbG8gd29ybGQ=", "$type": "1" }, b: 3 } );
   dbcl.insert( { _id: { "$binary": "aQQsbG8gd29ybGQ=", "$type": "1" }, b: 4 } );
}

function update ( dbcl )
{
   //update es support to es support
   dbcl.update( { $set: { _id: 111 } }, { _id: 1 } );
   dbcl.update( { $set: { _id: 22, a: "text22" } }, { _id: 2 } );
   dbcl.update( { $set: { _id: 33, a: "text33" } }, { _id: 3 } );
   dbcl.update( { $set: { _id: 44 } }, { _id: 4 } );
   //update es support to es not support
   dbcl.update( { $set: { _id: { "$binary": "aAAsbG8gd29ybGQ=", "$type": "1" } } }, { _id: 5 } );
   dbcl.update( { $set: { _id: { "$binary": "aLLsbG8gd29ybGQ=", "$type": "1" }, a: "text66" } }, { _id: 6 } );
   dbcl.update( { $set: { _id: { "$binary": "aHHsbG8gd29ybGQ=", "$type": "1" }, a: "text77" } }, { _id: 7 } );
   dbcl.update( { $set: { _id: { "$binary": "aIIsbG8gd29ybGQ=", "$type": "1" } } }, { _id: 8 } );
   //update es not support to es support
   dbcl.update( { $set: { _id: 9 } }, { a: "text9" } );
   dbcl.update( { $set: { _id: 10, a: "text1010" } }, { a: "text10" } );
   dbcl.update( { $set: { _id: 11, a: "text1111" } }, { b: 1 } );
   dbcl.update( { $set: { _id: 12 } }, { b: 2 } );
   //update es not support to es not support
   dbcl.update( { $set: { _id: { "$binary": "aBBsbG8gd29ybGQ=", "$type": "1" } } }, { a: "text13" } );
   dbcl.update( { $set: { _id: { "$minKey": 1 }, a: "text1414" } }, { a: "text14" } );
   dbcl.update( { $set: { _id: null, a: "text1515" } }, { b: 3 } );
   dbcl.update( { $set: { _id: { "$maxKey": 1 } } }, { b: 4 } );
}
