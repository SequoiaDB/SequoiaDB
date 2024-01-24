/************************************
*@Description: 带hint删除记录 
*@author:      liuxiaoxuan
*@createdate:  2018.10.09
*@testlinkCase: seqDB-14384
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14384";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14384";
   var commonIndexName = "commonIndex"
   dbcl.createIndex( textIndexName, { "a": "text" } );
   dbcl.createIndex( commonIndexName, { "b": 1 } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "test_14384_" + i, b: "testb_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );

   // remove with hint textIndex
   dbcl.remove( { "a": "test_14384_0" }, { "": textIndexName } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 9999 );

   // check result
   var dbOpr = new DBOperator();
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond );
   var expResult = dbOpr.findFromCL( dbcl, null );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // remove with hint commonIndex
   dbcl.remove( { "a": { "$gt": "test_14384_1000" } }, { "": commonIndexName } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 4 );

   // check result
   var actResult = dbOpr.findFromCL( dbcl, findCond );
   var expResult = dbOpr.findFromCL( dbcl, null );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
