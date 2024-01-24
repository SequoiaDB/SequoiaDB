/************************************
*@Description: 全文检索与普通查询的3层not组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.26
*@testlinkCase: seqDB-14393
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14393";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14393";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14393_A " + i, b: i } );
      objs.push( { a: "test_14393_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14393_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14393_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$not": [{ "$and": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }] }, { "a": { "$isnull": 0 } }] }; //not-and-and
   var findNoneConf2 = { "$not": [{ "$and": [{ "$or": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 0 } }] }] }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }; //not-and-or
   var findNoneConf3 = { "$not": [{ "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 1 } }] }] }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }; //not-and-not
   var findNoneConf4 = { "$not": [{ "$or": [{ "$and": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 0 } }] }] }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] }; //not-or-and
   var findNoneConf5 = { "$not": [{ "$or": [{ "$or": [{ "b": { "$gte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_A" } } } } }] }] }, { "a": { "$isnull": 0 } }] }; //not-or-or
   var findNoneConf6 = { "$not": [{ "$or": [{ "$not": [{ "b": { "$gt": 15000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_C" } } } } }] }] }, { "a": { "$exists": 1 } }] }; //not-or-not
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
   var findSomeConf1 = { "$not": [{ "$and": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_A" } } } } }] }, { b: { "$lt": 10000 } }] }, { "a": { "$exists": 1 } }] }; //not-and-and
   var findSomeConf2 = { "$not": [{ "$and": [{ "$or": [{ "b": { "$lte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_D" } } } } }] }, { b: { "$gte": 15000 } }] }, { "a": { "$exists": 1 } }] }; //not-and-or
   var findSomeConf3 = { "$not": [{ "$and": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "field": "a" } } } } }] }, { b: { "$lt": 5000 } }] }, { "a": { "$exists": 1 } }] }; //not-and-not
   var findSomeConf4 = { "$not": [{ "$or": [{ "$and": [{ "b": { "$lt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_B" } } } } }] }, { b: { "$lt": 10000 } }] }, { "a": { "$exists": 1 } }] }; //not-or-and
   var findSomeConf5 = { "$not": [{ "$or": [{ "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_D" } } } } }] }, { b: { "$lt": 15000 } }] }, { "a": { "$exists": 1 } }] }; //not-or-not
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findSomeConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findSomeConf4, { 'a': '' }, { _id: 1 } );
   var actResult5 = dbOpr.findFromCL( dbcl, findSomeConf5, { 'a': '' }, { _id: 1 } );
   var expResult1 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 5000 } }, { 'a': '' }, { _id: 1 } );
   var expResult2 = dbOpr.findFromCL( dbcl, { "b": { "$lt": 15000 } }, { 'a': '' }, { _id: 1 } );
   var expResult3 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 5000 } }, { 'a': '' }, { _id: 1 } );
   var expResult4 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 10000 } }, { 'a': '' }, { _id: 1 } );
   var expResult5 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 15000 } }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult1, actResult1 );
   checkResult( expResult2, actResult2 );
   checkResult( expResult3, actResult3 );
   checkResult( expResult4, actResult4 );
   checkResult( expResult5, actResult5 );

   // match all records
   var findAllConf1 = { "$not": [{ "$and": [{ "$and": [{ "b": { "$gt": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_A" } } } } }] }] }, { "a": { "$exists": 1 } }] }; //not-and-and
   var findAllConf2 = { "$not": [{ "$and": [{ "$or": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //not-and-or
   var findAllConf3 = { "$not": [{ "$and": [{ "$not": [{ "b": { "$lt": 15000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_D" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //not-and-not
   var findAllConf4 = { "$not": [{ "$or": [{ "$and": [{ "b": { "$gte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_A" } } } } }] }] }, { "a": { "$isnull": 1 } }] }; //not-or-and
   var findAllConf5 = { "$not": [{ "$or": [{ "$or": [{ "b": { "$gt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_C" } } } } }] }] }, { "a": { "$exists": 0 } }] }; //not-or-or
   var findAllConf6 = { "$not": [{ "$or": [{ "$not": [{ "b": { "$gt": 1000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14393_B" } } } } }] }] }, { "a": { "$exists": 0 } }] }; //not-or-not
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' }, { _id: 1 } );
   var actResult3 = dbOpr.findFromCL( dbcl, findAllConf3, { 'a': '' }, { _id: 1 } );
   var actResult4 = dbOpr.findFromCL( dbcl, findAllConf4, { 'a': '' }, { _id: 1 } );
   var actResult5 = dbOpr.findFromCL( dbcl, findAllConf5, { 'a': '' }, { _id: 1 } );
   var actResult6 = dbOpr.findFromCL( dbcl, findAllConf6, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );
   checkResult( expResult, actResult4 );
   checkResult( expResult, actResult5 );
   checkResult( expResult, actResult6 );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
