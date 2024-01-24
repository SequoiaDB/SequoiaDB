/************************************
*@Description: no fullText index,query with fullText
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

   var clName = COMMCLNAME + "_ES_14802";
   var clFullName = COMMCSNAME + "." + clName;

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.insert( { a: "text" } );

   assert.tryThrow( SDB_RTN_INDEX_NOTEXIST, function()
   {
      var cursor = dbcl.find( { "": { $Text: { query: { match_all: {} } } } } );
      while( cursor.next() ) { }
   } );

   dropCL( db, COMMCSNAME, clName );
}
