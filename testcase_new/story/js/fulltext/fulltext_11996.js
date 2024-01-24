/************************************
*@Description: record not include text index field, insert/update/delete
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

   var clName = COMMCLNAME + "_ES_11996";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_11996";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   dbcl.insert( { b: "text" } );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { b: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );

   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
   checkResult( expectRecords, actRecords );

   dbcl.update( { $set: { b: "update" } } );
   checkFullSyncToES( COMMCSNAME, clName, indexName, 0 );
   var expectRecords = dbOperator.findFromCL( dbcl, { a: { $type: 2, $et: "string" } } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } } );
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
