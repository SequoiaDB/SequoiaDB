/******************************************************************************
@Description :   seqDB-15530:指定_id字段插入记录，并删除
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
   var clName = COMMCLNAME + "_ES_15530";
   var queryCond = '{ "query" : { "match_all" : {} }, "size" : "20" }';
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15530";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dbcl.remove();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //insert again to test synchronize from cappedCL to ES
   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   expectResult = esOperator.findFromES( esIndexNames[0], queryCond ).sort( compare( 'a' ) );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond ).sort( compare( 'a' ) );
   checkResult( expectResult, actResult );

   dbcl.truncate();

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

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
