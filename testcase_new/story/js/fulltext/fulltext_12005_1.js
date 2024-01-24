/************************************
*@Description: create fullText index 3 keys��insert/update/delete
*@author:      zhaoyu
*@createdate:  2018.10.11
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12005_1";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "abc_12005";
   var doc = [{ No: 1, a: "a1", b: "b1", c: "c1" },
   { No: 2, c: "c2" },
   { No: 3, d: "d" }];

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( doc );
   commCreateIndex( dbcl, indexName, { a: "text", b: "text", c: "text" } );
   dbcl.insert( doc );
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 4 );

   var expectRecords = dbOperator.findFromCL( dbcl, { $or: [{ a: { $type: 2, $et: "string" } }, { b: { $type: 2, $et: "string" } }, { c: { $type: 2, $et: "string" } }] } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   dbcl.update( { $set: { a: "update" } }, { c: { $exists: 1 } } );
   dbcl.insert( { No: 4, a: "update", b: "update", c: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 5 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } } );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" } );
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
