/************************************
*@Description: 使用query.count()执行记录数统计，query中包含全文检索 
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-12044
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_12044";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_12044";
   var commonIndexName = "commonIndex"
   dbcl.createIndex( textIndexName, { "a": "text" } );
   dbcl.createIndex( commonIndexName, { "b": 1 } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_12044 " + i, b: "testb_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // $and
   var findNoneCond = { "$and": [{ "b": { "$et": "testb_0" } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_12044 1" } } } } }] };
   var actCount = dbcl.find( findNoneCond ).count();
   var expectCount = 0;
   checkCount( expectCount, actCount );

   var findSomeCond = { "$and": [{ "b": { "$gte": "testb_1" } }, { "": { "$Text": { "query": { "match": { "a": "test_12044" } } } } }] };
   var actCount = dbcl.find( findSomeCond ).count();
   var expectCount = 19999;
   checkCount( expectCount, actCount );

   var findAllCond = { "$and": [{ "b": { "$gte": "testb_0" } }, { "": { "$Text": { "query": { "match": { "a": "test_12044" } } } } }] };
   var actCount = dbcl.find( findAllCond ).count();
   var expectCount = 20000;
   checkCount( expectCount, actCount );

   // $or
   var findNoneCond = { "$or": [{ "b": { "$et": "testb" } }, { "": { "$Text": { "query": { "match": { "a": "testa" } } } } }] };
   var actCount = dbcl.find( findNoneCond ).count();
   var expectCount = 0;
   checkCount( expectCount, actCount );

   var findSomeCond = { "$or": [{ "b": { "$gt": "testb_10000" } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_12044 1" } } } } }] };
   var actCount = dbcl.find( findSomeCond ).count();
   var expectCount = 19995;
   checkCount( expectCount, actCount );

   var findAllCond = { "$or": [{ "b": { "$gte": "testb_0" } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_12044 1" } } } } }] };
   var actCount = dbcl.find( findAllCond ).count();
   var expectCount = 20000;
   checkCount( expectCount, actCount );

   // $not
   var findNoneCond = { "$not": [{ "b": { "$gte": "testb_0" } }, { "": { "$Text": { "query": { "match": { "a": "test_12044" } } } } }] };
   var actCount = dbcl.find( findNoneCond ).count();
   var expectCount = 0;
   checkCount( expectCount, actCount );

   var findSomeCond = { "$not": [{ "b": { "$gte": "testb_9" } }, { "": { "$Text": { "query": { "match": { "a": "test_12044" } } } } }] };
   var actCount = dbcl.find( findSomeCond ).count();
   var expectCount = 18889;
   checkCount( expectCount, actCount );

   var findAllCond = { "$not": [{ "b": { "$et": "testb_0" } }, { "": { "$Text": { "query": { "match_phrase": { "a": "test_12044 2" } } } } }] };
   var actCount = dbcl.find( findAllCond ).count();
   var expectCount = 20000;
   checkCount( expectCount, actCount );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
function checkCount ( expectCount, actCount )
{
   if( expectCount != actCount )
   {
      throw new Error( "expect record num: " + expectCount + ", actual record num: " + actCount );
   }
}
