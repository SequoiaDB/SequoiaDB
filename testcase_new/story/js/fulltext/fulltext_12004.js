/************************************
*@Description: create fullText index only one key��insert/update/delete
*@author:      zhaoyu
*@createdate:  2018.9.28
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12004";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12004";
   var doc = [{ a: "string1" },
   { a: "string2", b: 1 },
   { b: 2 }];

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( doc );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( doc );

   //all of record sync to ES
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 4 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //string update to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: "string1" } );
   dbcl.insert( { a: "string3" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } }, null, { _id: 1 } );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //string update to int,not sync ES
   dbcl.update( { $set: { a: 1 } }, { a: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 3 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //int update to int,not sync ES
   dbcl.update( { $set: { a: 100 } }, { a: 1 } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 3 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   //int update to string,sync ES
   dbcl.update( { $set: { a: "update" } }, { a: 100 } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, { _id: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } }, null, { _id: 1 } );
   checkResult( expectRecords, actRecords );

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
