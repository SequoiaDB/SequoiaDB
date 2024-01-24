/************************************
*@Description: count命令字中使用全文检索与普通查询的or组合
*@author:      liuxiaoxuan
*@createdate:  2018.10.08
*@testlinkCase: seqDB-15140
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_15140";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_15140";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   dbcl.createIndex( "commonIndex", { "b": 1 } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_15140_" + i, b: "testb_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // match 0 record
   var findNoneConf = { "$or": [{ "b": { "$et": "testb" } }, { "": { "$Text": { "query": { "match": { "a": "testa" } } } } }] };
   var actCount = dbcl.count( findNoneConf );
   var expectCount = 0;
   checkCount( expectCount, actCount );

   // match some records
   var findSomeConf = { "$or": [{ "b": { "$gt": "testb_10000" } }, { "": { "$Text": { "query": { "match": { "a": "test_15140_1" } } } } }] };
   var actCount = dbcl.count( findSomeConf );
   var expectCount = 19995;
   checkCount( expectCount, actCount );

   // match all records
   var findAllConf = { "$or": [{ "b": { "$gte": "testb_0" } }, { "": { "$Text": { "query": { "match": { "a": "test_15140_1" } } } } }] };
   var actCount = dbcl.count( findAllConf );
   var expectCount = 20000;
   checkCount( expectCount, actCount );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
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
