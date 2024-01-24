/************************************
*@Description: insert record with 16M
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

   var clName = COMMCLNAME + "_ES_12000";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12000";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var str = new Array( 1024 * 1024 * 15 ).join( "a" );
   dbcl.insert( { a: str } );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { a: str } );

   //check count,but not check record(out of memery)
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );

   var str = new Array( 1024 * 1024 * 16 ).join( "a" );
   assert.tryThrow( SDB_DMS_RECORD_TOO_BIG, function()
   {
      dbcl.insert( { a: str } );
   } );

   checkFullSyncToES( COMMCSNAME, clName, indexName, 2 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983 
   checkIndexNotExistInES( esIndexNames );
}
