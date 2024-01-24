/************************************
*@Description: 查询接口参数验证 
*@author:      liuxiaoxuan
*@createdate:  2018.10.09
*@testlinkCase: seqDB-12046
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_12046";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_12046";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   dbcl.insert( { "a": "testa" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   // check result
   var dbOperator = new DBOperator();
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var expResult = [{ "a": "testa" }];
   var actResult = dbOperator.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   checkResult( expResult, actResult );

   // find with wrong search command
   try
   {
      var rec = dbcl.find( { "": { "$text": { "query": { "match_all": {} } } } } ); // should fail
      rec.next();
      throw new Error( "find with wrong command" );
   }
   catch( e )
   {
      if( SDB_INVALIDARG != e.message )  
      {
         throw e;
      }
   }

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
