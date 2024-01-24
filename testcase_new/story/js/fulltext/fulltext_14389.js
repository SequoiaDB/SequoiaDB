/*onf1
*@Description: 全文检索与普通查询的2层or组合验证  
*@author:      liuxiaoxuan
*@createdate:  2019.11.01
*@testlinkCase: seqDB-14389
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14389";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14389";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14389_A " + i, b: i } );
      objs.push( { a: "test_14389_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14389_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14389_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$or": [{ "$or": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_A1" } } } } }] }] }; // or-or
   var findNoneConf2 = { "$or": [{ "$and": [{ "b": { "$gt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_B2" } } } } }] }] }; // or-and, fulltext search under "$and"
   var findNoneConf3 = { "$or": [{ "$and": [{ "b": { "$lte": 0 } }, { "b": { "$gte": 5000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_C3" } } } } }] }; // or-and, fulltext search under "$or"
   var findNoneConf4 = { "$or": [{ "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }; // or-not, fulltext search under "$not"
   var findNoneConf5 = { "$or": [{ "$not": [{ "b": { "$lt": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_D4" } } } } }] }; // or-not, fulltext search under "$or"
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
   var findSomeConf1 = { "$or": [{ "$or": [{ "b": { "$lt": 15000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_A" } } } } }] }] }; // or-or
   var findSomeConf2 = { "$or": [{ "$not": [{ "b": { "$gte": 1000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_B" } } } } }] }] }; // or-not, fulltext search under "$not"
   var findSomeConf3 = { "$or": [{ "$not": [{ "b": { "$gte": 10000 } }, { "b": { "$lt": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_C" } } } } }] }; // or-not, fulltext search under "$or"
   var findSomeConf4 = { "$or": [{ "$and": [{ "b": { "$gte": 10000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_D" } } } } }] }; // or-and, fulltext search under "$or" 
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findSomeConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findSomeConf4, { 'a': '' }, { _id: 1 } );
   var expResult1 = dbOpr.findFromCL( dbcl, { "b": { "$lt": 15000 } }, { 'a': '' }, { _id: 1 } );
   var expResult2 = dbOpr.findFromCL( dbcl, { "$or": [{ "b": { "$lt": 5000 } }, { "b": { "$gte": 10000 } }] }, { 'a': '' }, { _id: 1 } );
   var expResult3 = dbOpr.findFromCL( dbcl, { "$and": [{ "b": { "$gte": 0 } }, { "b": { "$lt": 15000 } }] }, { 'a': '' }, { _id: 1 } );
   var expResult4 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 10000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult1, actResult1 );
   checkResult( expResult2, actResult2 );
   checkResult( expResult3, actResult3 );
   checkResult( expResult4, actResult4 );

   // match all records
   var findAllConf1 = { "$or": [{ "$or": [{ "b": { "$gte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_A" } } } } }] }] }; // or-or
   var findAllConf2 = { "$or": [{ "$not": [{ "b": { "$lte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14389_B" } } } } }] }] }; //or-not, fulltext search under "$not"
   var findAllConf3 = { "$or": [{ "$not": [{ "b": { "$gte": 15000 } }, { "b": { "$lt": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_D" } } } } }] }; // or-not, fulltext search under "$or"
   var findAllConf4 = { "$or": [{ "$and": [{ "b": { "$gte": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14389_C" } } } } }] }; // or-and, fulltext search under "$or"
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
