/************************************
*@Description: 更新索引字段   
*@author:      liuxiaoxuan
*@createdate:  2018.10.10
*@testlinkCase: seqDB-14380
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14380";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14380";
   var commonIndexName = "commonIndex"
   dbcl.createIndex( textIndexName, { "a": "text" } );
   dbcl.createIndex( commonIndexName, { "b": 1 } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "test_14380_" + i, b: "testb_" + i } );
   }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );

   // update textIndex
   dbcl.update( { "$set": { "a": "update text index 0" } }, { "a": { "$gt": "test_14380_1000" } } );
   dbcl.update( { "$set": { "a": "update text index 1" } }, { "b": { "$et": "testb_0" } } );
   dbcl.insert( { a: "new1" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10001 );

   // check result
   var dbOpr = new DBOperator();
   var findCond = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = dbOpr.findFromCL( dbcl, null, { "a": { "$include": 1 } } );
   actResult.sort( compare( "a", compare( "b" ) ) );
   expResult.sort( compare( "a", compare( "b" ) ) );
   checkResult( expResult, actResult );

   // update commonIndex
   dbcl.update( { "$set": { "b": "update common index 0" } }, { "a": { "$gt": "test_14380_1000" } } );
   dbcl.update( { "$set": { "b": "update common index 1" } }, { "b": { "$et": "testb_0" } } );
   dbcl.insert( { a: "new2" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10002 );

   // check result
   var actResult = dbOpr.findFromCL( dbcl, findCond, { "a": { "$include": 1 } } );
   var expResult = dbOpr.findFromCL( dbcl, null, { "a": { "$include": 1 } } );
   actResult.sort( compare( "a", compare( "b" ) ) );
   expResult.sort( compare( "a", compare( "b" ) ) );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
