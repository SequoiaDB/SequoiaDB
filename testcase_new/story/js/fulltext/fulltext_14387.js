/************************************
*@Description: 全文检索与普通查询的1层not组合验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.26
*@testlinkCase: seqDB-14387
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14387";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14387";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14387_A " + i, b: i } );
      objs.push( { a: "test_14387_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14387_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14387_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf = { "$not": [{ "b": { "$gte": 0 } }, { "": { "$Text": { "query": { "exists": { "field": "a" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findNoneConf );
   var expResult = [];
   checkResult( expResult, actResult );

   // match some records
   var findSomeConf = { "$not": [{ "b": { "$gt": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14387_C" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findSomeConf, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, { "$or": [{ "$and": [{ "b": { "$gte": 0 } }, { "b": { "$lte": 10000 } }] }, { "b": { "$gte": 15000 } }] }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult );

   // match all records
   var findAllConf = { "$not": [{ "b": { "$lt": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14387_D" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findAllConf, { 'a': '' }, { _id: 1 } );
   var expResult = dbOpr.findFromCL( dbcl, null, { 'a': '' }, { _id: 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
