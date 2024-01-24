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
   var textIndexName = "a_15138";
   var commIndexName = "b";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   commCreateIndex( dbcl, textIndexName, { b: "text" } );
   commCreateIndex( dbcl, commIndexName, { a: -1 } );

   for( var j = 0; j < 3; j++ )
   {
      var doc = [];
      for( var i = 0; i < 10000; i++ )
      {
         doc.push( { a: j * 10000 + i, b: "test" + j * 10000 + i, c: j * 10000 + i } );
      }
      dbcl.insert( doc );
   }

   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 30000 );

   var actCount = dbcl.count( { $and: [{ a: 1 }, { "": { $Text: { query: { match: { b: "text2" } } } } }] } );
   var expectCount = 0;
   if( parseInt( actCount ) !== expectCount )
   {
      throw new Error( "COUNT_ERR" );
   }

   var actCount = dbcl.count( { $and: [{ a: { $gt: 5000 } }, { "": { $Text: { query: { range: { b: { gt: "test1" } } } } } }] } );
   var expectCount = 20000;
   if( parseInt( actCount ) !== expectCount )
   {
      throw new Error( "COUNT_ERR" );
   }

   var actCount = dbcl.count( { $and: [{}, { "": { $Text: { query: { match_all: {} } } } }] } );
   var expectCount = 30000;
   if( parseInt( actCount ) !== expectCount )
   {
      throw new Error( "COUNT_ERR" );
   }

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
