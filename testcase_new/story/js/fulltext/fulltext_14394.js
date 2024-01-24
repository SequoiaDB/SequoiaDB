/************************************
*@Description: 全文检索与普通查询的N层随机组合  
*@author:      liuxiaoxuan
*@createdate:  2018.10.28
*@testlinkCase: seqDB-14394
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14394";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14394";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_14394 " + i, b: i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf1 = { "$and": [{ "$and": [{ "$and": [{ "$and": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }] }, { "a": { "$isnull": 1 } }] }, { "a": { "exists": 1 } }] }; //and-and-and-and
   var findNoneConf2 = { "$and": [{ "$not": [{ "$and": [{ "$not": [{ "b": { "$gte": 0 } }, { "a": { "$isnull": 0 } }] }] }, { "a": { "$exists": 1 } }] }, { "a": "test_14394" }] }; //and-not-and-not
   var findNoneConf3 = { "$and": [{ "$or": [{ "$and": [{ "$or": [{ "b": { "$lt": 0 } }, { "a": { "$isnull": 1 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }, { "a": { "$isnull": 0 } }] }, { "a": "test_14394" }] };//and-or-and-or
   var actResult1 = dbOpr.findFromCL( dbcl, findNoneConf1 );
   var actResult2 = dbOpr.findFromCL( dbcl, findNoneConf2 );
   var actResult3 = dbOpr.findFromCL( dbcl, findNoneConf3 );
   var expResult = [];
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );
   checkResult( expResult, actResult3 );

   // match some records
   var findSomeConf1 = { "$and": [{ "$and": [{ "$and": [{ "$and": [{ "b": { "$lt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }] }, { "a": { "$isnull": 0 } }] }, { "a": { "$exists": 1 } }] };//and-and-and-and
   var findSomeConf2 = { "$not": [{ "$and": [{ "$not": [{ "$and": [{ "b": { "$lt": 10000 } }, { "a": { "$isnull": 0 } }] }] }, { "a": { "$exists": 1 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }; //not-and-not-and
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' } );
   actResult1.sort( compare( "a" ) );
   actResult2.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );

   // match all records
   var findAllConf1 = { "$and": [{ "$not": [{ "$and": [{ "$or": [{ "b": { "$lt": 0 } }, { "a": { "$isnull": 1 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }, { "a": { "$isnull": 0 } }] }, { "b": { "$exists": 1 } }] }; //and-not-and-or
   var findAllConf2 = { "$and": [{ "$and": [{ "$not": [{ "$not": [{ "b": { "$lt": 20000 } }, { "a": { "$isnull": 0 } }] }, { "": { "$Text": { "query": { "match": { "a": "test_14394" } } } } }] }, { "a": { "$isnull": 0 } }] }, { "b": { "$exists": 1 } }] }; //and-and-not-not
   var actResult1 = dbOpr.findFromCL( dbcl, findAllConf1, { 'a': '' } );
   var actResult2 = dbOpr.findFromCL( dbcl, findAllConf2, { 'a': '' } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' } );
   actResult1.sort( compare( "a" ) );
   actResult2.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult1 );
   checkResult( expResult, actResult2 );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
