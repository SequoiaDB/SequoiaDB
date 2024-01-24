/************************************
*@Description: 全文检索与普通查询的1层or组合验证  
*@author:      liuxiaoxuan
*@createdate:  2019.11.01
*@testlinkCase: seqDB-14386
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14386";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14386";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "test_14386_A " + i, b: i } );
      objs.push( { a: "test_14386_B " + i, b: i + 5000 } );
      objs.push( { a: "test_14386_C " + i, b: i + 10000 } );
      objs.push( { a: "test_14386_D " + i, b: i + 15000 } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf = { "$or": [{ "b": { "$lt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14386" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findNoneConf );
   var expResult = [];
   checkResult( expResult, actResult );

   // match some records
   var findSomeConf1 = { "$or": [{ "b": { "$gte": 7000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14386_B" } } } } }] }; // with intersection
   var findSomeConf2 = { "$or": [{ "b": { "$lte": 5000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14386_D" } } } } }] }; // without intersection
   var actResult1 = dbOpr.findFromCL( dbcl, findSomeConf1, { 'a': '' }, { _id: 1 } );
   var actResult2 = dbOpr.findFromCL( dbcl, findSomeConf2, { 'a': '' }, { _id: 1 } );
   var expResult1 = dbOpr.findFromCL( dbcl, { "b": { "$gte": 5000 } }, { 'a': '' }, { _id: 1 } );
   var expResult2 = dbOpr.findFromCL( dbcl, { $or: [{ "b": { "$lte": 5000 } }, { "b": { "$gte": 15000 } }] }, { 'a': '' }, { _id: 1 } );
   checkResult( expResult1, actResult1 );
   checkResult( expResult2, actResult2 );

   // match all records
   var findAllConf1 = { "$or": [{ "b": { "$gt": 0 } }, { "": { "$Text": { "query": { "match": { "a": "test_14386_A" } } } } }] };  // with intersection
   var findAllConf2 = { "$or": [{ "b": { "$lt": 15000 } }, { "": { "$Text": { "query": { "match": { "a": "test_14386_D" } } } } }] }; // without intersection
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
