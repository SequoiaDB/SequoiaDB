/************************************
*@Description: fullText index and common index on the same key��insert/update/delete/query
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

   var clName = COMMCLNAME + "_ES_12002";
   var clFullName = COMMCSNAME + "." + clName
   var textIndexName = "fullIndex_12002";
   var commIndexName = "commIndex";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   commCreateIndex( dbcl, commIndexName, { a: 1 } );
   dbcl.insert( { a: "text" } );
   commCreateIndex( dbcl, textIndexName, { a: "text" } );
   dbcl.insert( { a: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: "text" }, null, null, { "": "commIndex" } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   dbcl.update( { $set: { a: "update" } }, { a: "text" } );
   dbcl.insert( { a: "update" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 3 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: "update" }, null, null, { "": "commIndex" } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match: { a: "update" } } } } } );
   checkResult( expectRecords, actRecords );

   dbcl.remove();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
