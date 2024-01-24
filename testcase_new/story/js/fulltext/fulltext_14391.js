/************************************
*@Description: 全文检索与普通查询的3层and组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.28
*@testlinkCase: seqDB-14391
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14391";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14391";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14391 " + i, b: i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$and": [{ "$and": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14391" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //and-and-and
   var findNoneConf2 = { "$and": [{ "$and": [{ "$or": [{ "b": { "$lt": 0 } }, { "a": { "$isnull": 1 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14391" } } } } }] }; //and-and-or
   var findNoneConf3 = { "$and": [{ "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 0 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14391" } } } } }] }; //and-and-not
   var findNoneConf4 = { "$and": [{ "$or": [{ "$and": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 1 } }] }] }, { "": { "$Text": { "query": { "match": { "a": "test_14391" } } } } }] }; //and-or-and
   var findNoneConf5 = { "$and": [{ "$or": [{ "$or": [{ "b": { "$gte": 20000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14391_01" } } } } }] }] }, { "a": { "$isnull": 0 } }] }; //and-or-or
   var findNoneConf6 = { "$and": [{ "$or": [{ "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14391 2" } } } } }] }] }, { "a": { "$exists": 1 } }] } //and-or-not
   var actResult1 = dbOpr.findFromCL( dbcl, findNoneConf1 );
   var actResult2 = dbOpr.findFromCL( dbcl, findNoneConf2 );
   var actResult3 = dbOpr.findFromCL( dbcl, findNoneConf3 );
   var actResult4 = dbOpr.findFromCL( dbcl, findNoneConf4 );
   var actResult5 = dbOpr.findFromCL( dbcl, findNoneConf5 );
   var actResult6 = dbOpr.findFromCL( dbcl, findNoneConf6 );
   var expResult = [];
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );
   checkResult( expResult, actResult5 );
   checkResult( expResult, actResult6 );

   // match some records
   var findSomeConf1 = { "$and": [{ "$and": [{ "$and": [{ "b": { "$lte": 10000 } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_14391" } } } } }] }, { b: { "$lt": 10000 } }] }, { "a": { "$exists": 1 } }] }; //and-and-and
   var findSomeConf2 = { "$and": [{ "$and": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_14391" } } } } }] }, { b: { "$lt": 10000 } }] }, { "a": { "$exists": 1 } }] }; //and-and-not
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );

   // match all records
   var findAllConf1 = { "$and": [{ "$and": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14391 2" } } } } }] }] }, { "a": { "$exists": 1 } }] }; //and-and-and
   var findAllConf2 = { "$and": [{ "$and": [{ "$not": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14391 2" } } } } }] }] }, { "a": { "$isnull": 0 } }] }; //and-and-not
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
