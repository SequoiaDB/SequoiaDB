/************************************
*@Description: limit/skip,limit + skip < record num
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

   var clName = COMMCLNAME + "_ES_15130";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_15130";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( dbcl, indexName, { a: "text" } );

   //insert random record
   for( var i = 0; i < 3; i++ )
   {
      var rd = new commDataGenerator();
      var recs = rd.getRecords( 10000, "string", ['a', 'b'] );
      dbcl.insert( recs );
   }

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 30000 );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 }, null, 1000, 2000 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 }, null, 1000, 2000 );
   checkResult( expectRecords, actRecords );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 }, null, 8000, 7000 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 }, null, 8000, 7000 );
   checkResult( expectRecords, actRecords );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 }, null, 8000, 15000 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 }, null, 8000, 15000 );
   checkResult( expectRecords, actRecords );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 }, null, 11000, 12000 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 }, null, 11000, 12000 );
   checkResult( expectRecords, actRecords );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { _id: 1 }, null, 11000, 8000 );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { _id: 1 }, null, 11000, 8000 );
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
