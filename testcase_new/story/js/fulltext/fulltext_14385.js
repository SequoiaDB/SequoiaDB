/************************************
*@Description: 全文检索与普通查询的1层and组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.26
*@testlinkCase: seqDB-14385
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14385";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14385";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14385 " + i, b: i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf = { "$and": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14385" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findNoneConf );
   var expResult = [];
   checkResult( expResult, actResult );

   // match some records
   var findSomeConf = { "$and": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14385" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findSomeConf, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$gte": 10000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult );

   // match all records
   var findAllConf = { "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14385 2" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findAllConf, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
