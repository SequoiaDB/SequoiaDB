/************************************
*@Description: 带from/size进行全文检索  
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-12045
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_12045";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_12045";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10001; i++ )
   {
      objs.push( { a: "test_12045_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10001 );

   var esOpr = new ESOperator();
   var dbOpr = new DBOperator();
   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );

   // from 
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "from": 9990 } } };
   var searchCond = '{"query":{"match_all":{}}, "from": 9990}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // size
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "size": 10000 } } };
   var searchCond = '{"query":{"match_all":{}}, "size": 10000}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // from+size < 10000
   var findCond = { "": { "$Text": { "query": { "match_all": {} }, "from": 1, "size": 9990 } } };
   var searchCond = '{"query":{"match_all":{}}, "from": 1, "size": 9990}'
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = esOpr.findFromES( esIndexNames[0], searchCond );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // from+size > 10000, should fail
   try
   {
      var rec = dbcl.find( { "": { "$Text": { "query": { "match_all": {} }, "from": 0, "size": 10001 } } } );
      rec.next();
      throw new Error( "find es overrize" );
   }
   catch( e )
   {
      if( SDB_SYS != e.message )
      {
         throw e;
      }
   }

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
