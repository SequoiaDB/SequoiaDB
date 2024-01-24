/************************************
*@Description: create fullText index 16 keys��insert/update/delete
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

   var clName = COMMCLNAME + "_ES_12005_2";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12005";
   var doc = [{ No: 1, a1: "text", a2: "text", a3: "text", a4: "text", a5: "text", a6: "text", a7: "text", a8: "text", a9: "text", a10: "text", a11: "text", a12: "text", a13: "text", a14: "text", a15: "text", a16: "text" }];

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( doc );
   commCreateIndex( dbcl, indexName, { a1: "text", a2: "text", a3: "text", a4: "text", a5: "text", a6: "text", a7: "text", a8: "text", a9: "text", a10: "text", a11: "text", a12: "text", a13: "text", a14: "text", a15: "text", a16: "text" } );
   dbcl.insert( doc );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );

   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   dbcl.update( { $set: { a1: "update" } }, { a1: { $exists: 1 } } );
   dbcl.insert( { a1: "update", a2: "update", a3: "update", a4: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 3 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a1: "update" } } } } } );
   var expectRecords = dbOperator.findFromCL( dbcl, { a1: "update" } );
   checkResult( expectRecords, actRecords );

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   //SEQUOIADBMAINSTREAM-3983 
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   checkIndexNotExistInES( esIndexNames );
}
