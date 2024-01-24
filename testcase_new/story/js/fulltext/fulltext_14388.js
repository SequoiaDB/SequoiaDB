/*onf1
*@Description: 全文检索与普通查询的2层and组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.26
*@testlinkCase: seqDB-14388
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14388";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14388";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14388 " + i, b: i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$and": [{ "$and": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; // and-and
   var findNoneConf2 = { "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; // and-not, fulltext search under "$not"
   var findNoneConf3 = { "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "b": { "$lte": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-not, fulltext search under "$and"
   var findNoneConf4 = { "$and": [{ "$or": [{ "b": { "$gte": 20000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388_01" } } } } }] }] }; // and-or, fulltext search under "$or"
   var findNoneConf5 = { "$and": [{ "$or": [{ "b": { "$lt": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-or, fulltext search under "$and"
   var actResult1 = dbOpr.findFromCL( dbcl, findNoneConf1 );
   var actResult2 = dbOpr.findFromCL( dbcl, findNoneConf2 );
   var actResult3 = dbOpr.findFromCL( dbcl, findNoneConf3 );
   var actResult4 = dbOpr.findFromCL( dbcl, findNoneConf4 );
   var actResult5 = dbOpr.findFromCL( dbcl, findNoneConf5 );
   var expResult = [];
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );
   checkResult( expResult, actResult5 );

   // match some records
   var findSomeConf1 = { "$and": [{ "$and": [{ "b": { "$lt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; // and-and
   var findSomeConf2 = { "$and": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; // and-not, fulltext search under "$not"
   var findSomeConf3 = { "$and": [{ "$not": [{ "b": { "$gte": 10000 } }, { "b": { "$lte": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-not, fulltext search under "$and"
   var findSomeConf4 = { "$and": [{ "$or": [{ "b": { "$lt": 10000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-or, fulltext search under "$and" 
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findSomeConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findSomeConf4, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );

   // match all records
   var findAllConf1 = { "$and": [{ "$and": [{ "b": { "$gt": -1 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; // and-and
   var findAllConf2 = { "$and": [{ "$not": [{ "b": { "$gte": 30000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }] }; //and-not, fulltext search under "$not"
   var findAllConf3 = { "$and": [{ "$not": [{ "b": { "$gte": 30000 } }, { "b": { "$lte": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-not, fulltext search under "$and"
   var findAllConf4 = { "$and": [{ "$or": [{ "b": { "$gte": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14388" } } } } }] }; // and-or, fulltext search under "$and"
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findAllConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findAllConf4, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
