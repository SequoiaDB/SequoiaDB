/************************************
*@Description: no id index��insert record with the same id value 
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

   var clName = COMMCLNAME + "_ES_12006";
   var clFullName = COMMCSNAME + "." + clName
   var indexName = "a_12006";

   dropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false } );
   commCreateIndex( dbcl, indexName, { a: "text" } );
   dbcl.insert( { _id: 1, a: "text1" } );
   dbcl.insert( { _id: 1, a: "text2" } );

   //_id is the same, ES sync the last one, but can return all records by fullText query even if the same _id value
   var dbOperator = new DBOperator();
   checkFullSyncToES( COMMCSNAME, clName, indexName, 1 );

   var expectRecords = dbOperator.findFromCL( dbcl, null, null, { a: 1 } );
   var actRecords = dbOperator.findFromCL( dbcl, { "": { "$Text": { query: { match_all: {} } } } }, null, { a: 1 } );
   checkResult( expectRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, indexName );
   dropCL( db, COMMCSNAME, clName );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
