/************************************
*@Description: 带全文索引条件删除记录 
*@author:      liuxiaoxuan
*@createdate:  2018.10.09
*@testlinkCase: seqDB-14383
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14383";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14383";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   dbcl.insert( { "a": "testa" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var dbOpr = new DBOperator();
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var expResult = [{ "a": "testa" }];
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   checkResult( expResult, actResult );

   // remove with DSL 
   try
   {
      dbcl.remove( { "": { "$Text": { "query": { "match_all": {} } } } } ); // should fail
      throw new Error( "remove with DSL" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )  
      {
         throw e;
      }
   }

   // check result
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
