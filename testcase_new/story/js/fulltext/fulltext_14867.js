/************************************
*@Description: full text sort
*@author:      zhaoyu
*@createdate:  2018.10.12
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_14867";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_14867";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { a: "text" } );

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 1 );

   //not support full text sort
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var cursor = dbcl.find( { "": { $Text: { query: { match_all: {} }, sort: [{ a: { order: "desc" } }] } } } );
      while( cursor.next() ) { }
   } );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
