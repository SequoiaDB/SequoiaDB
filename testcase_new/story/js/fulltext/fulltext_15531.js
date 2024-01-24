/******************************************************************************
@Description :   seqDB-15531:指定_id字段插入记录，全文检索并排序
@Modify list :   2018-9-30  xiaoni Zhao  Init
******************************************************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var dbOperator = new DBOperator();
   var clName = COMMCLNAME + "_ES_15531";
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var selectorCond = { a: "" };
   var textIndexName = "a_15531";

   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );

   insertData( dbcl );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   var limitCond = 10;
   var acSortCond = { a: 1 };
   var expectResult = dbOperator.findFromCL( dbcl, null, selectorCond, acSortCond, null, limitCond );
   var actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond, acSortCond );
   checkResult( expectResult, actResult );

   skipCond = 4;
   var decSortCond = { a: -1 };
   expectResult = dbOperator.findFromCL( dbcl, null, selectorCond, decSortCond, null, null, skipCond );
   actResult = dbOperator.findFromCL( dbcl, findCond, selectorCond, decSortCond );
   checkResult( expectResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
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
   dbcl.insert( { _id: { "$minKey": 1 }, a: "a91" } );
   dbcl.insert( { _id: { "$maxKey": 1 }, a: "a92" } );
   dbcl.insert( { _id: null, a: "a93" } );
   dbcl.insert( { _id: { "$binary": "aGVsbG8gd29ybGQ=", "$type": "1" }, a: "a94" } );
}
