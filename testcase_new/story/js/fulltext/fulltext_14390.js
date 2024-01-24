/*onf1
*@Description: 全文检索与普通查询的2层not组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.28
*@testlinkCase: seqDB-14390
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14390";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14390";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14390_A " + i, b: i } );
      objs.push( { a: "test_14390_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14390_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14390_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$not": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }; // not-and, fulltext search under "$and"
   var findNoneConf2 = { "$not": [{ "$and": [{ "b": { "$gte": 0 } }, { "b": { "$lte": 20000 } }] }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }; // not-and, fulltext search under "$not"
   var findNoneConf3 = { "$not": [{ "$or": [{ "b": { "$gte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14390_A" } } } } }] }] }; // not-or, fulltext search under "$or" 
   var findNoneConf4 = { "$not": [{ "$or": [{ "b": { "$gte": 0 } }, { "b": { "$gt": 20000 } }] }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }; //not-or, fulltext search under "$not"
   var findNoneConf5 = { "$not": [{ "$not": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14390_B" } } } } }] }] }; // not-not 
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
   var findSomeConf1 = { "$not": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14390_A" } } } } }] }] }; // not-and, fulltext search under "$and"
   var findSomeConf2 = { "$not": [{ "$and": [{ "b": { "$gte": 10000 } }, { "b": { "$lte": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14390_D" } } } } }] }; // not-and, fulltext search under "$not"
   var findSomeConf3 = { "$not": [{ "$or": [{ "b": { "$lt": -1 } }, { "b": { "$gte": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14390_B" } } } } }] }; // not-or, fulltext search under "$not"
   var findSomeConf4 = { "$not": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }; // not-not
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findSomeConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findSomeConf4, { 'a': '' }, { _id: 1 } );
   var expResult1 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 5000 } }, { 'a': '' }, { _id: 1 } );
   var expResult2 = dbOpr.findFromCL( dbcl, { "b": { "$lt": 15000 } }, { 'a': '' }, { _id: 1 } );
   var expResult3 = dbOpr.findFromCL( dbcl, { "$or": [{ "b": { "$lt": 5000 } }, { "b": { "$gte": 10000 } }] }, { 'a': '' }, { _id: 1 } );
   var expResult4 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 10000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult1, actResult1 );
   checkResult( expResult2, actResult2 );
   checkResult( expResult3, actResult3 );
   checkResult( expResult4, actResult4 );

   // match all records
   var findAllConf1 = { "$not": [{ "$and": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14390_A" } } } } }] }] }; // not-and, fulltext search under "$and" 
   var findAllConf2 = { "$not": [{ "$and": [{ "b": { "$gt": 0 } }, { "b": { "$lt": 5000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14390_B" } } } } }] }; // not-and, fulltext search under "$not"
   var findAllConf3 = { "$not": [{ "$or": [{ "b": { "$lt": -1 } }, { "b": { "$gte": 20000 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14390" } } } } }] }; // not-or, fulltext search under "$not"
   var findAllConf4 = { "$not": [{ "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }; // not-not
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findAllConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findAllConf4, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
