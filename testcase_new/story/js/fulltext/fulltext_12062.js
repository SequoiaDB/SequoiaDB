/************************************
*@Description: 主子表中带选择符及与sort的组合执行全文检索
*@author:      liuxiaoxuan
*@createdate:  2018.11.26
*@testlinkCase: seqDB-12062
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) )
   {
      return;
   }

   var groups = commGetGroups( db );
   if( groups.length < 2 )
   {
      return;
   }

   var mainCLName = COMMCLNAME + "_ES_12062_maincl";
   var subCLName1 = COMMCLNAME + "_ES_12062_subcl_1";
   var subCLName2 = COMMCLNAME + "_ES_12062_subcl_2";
   dropCL( db, COMMCSNAME, mainCLName, true, true );
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   dropCL( db, COMMCSNAME, subCLName2, true, true );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1 );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { b: 1 }, Group: groups[0][0]["GroupName"] } );
   subCL2.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], 50 );

   // attach cl
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: "test_12062" }, UpBound: { a: "test_12063" } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: "zzz_12062" }, UpBound: { a: "zzz_12063" } } );
   var textIndexName = "textIndex12062";
   commCreateIndex( mainCL, textIndexName, { a: "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "test_12062 " + i, b: "testb_" + i } );
   }
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "zzz_12062 " + i, b: "testb_" + ( 10000 + i ) } );
   }
   mainCL.insert( objs );
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, textIndexName, 20000 );

   // selector field is fulltext
   var dbOpr = new DBOperator();
   var selector = { "a": { "$include": 1 } };
   var actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a": "test_12062" } } } } }, selector, null, null, null, null );
   var expResult = dbOpr.findFromCL( mainCL, { "a": { "$lt": "z" } }, selector, null, null, null, null );
   expResult.sort( compare( "a" ) );
   actResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // selector field is non-fulltext
   selector = { "b": { "$include": 1 } };
   actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a": "test_12062" } } } } }, selector, null, null, null, null );
   expResult = dbOpr.findFromCL( mainCL, { "a": { "$lt": "z" } }, selector, null, null, null, null );
   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );

   // selector field is sort field
   var sort = { "a": 1 };
   selector = { "a": { "$include": 1 } };
   actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a": "zzz_12062" } } } } }, selector, sort, null, null, null );
   expResult = dbOpr.findFromCL( mainCL, { "a": { "$gte": "z" } }, selector, sort, null, null, null );
   checkResult( expResult, actResult );

   // selector field is non-sort field
   sort = { "_id": 1 };
   selector = { "b": { "$include": 1 } };
   actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a": "zzz_12062" } } } } }, selector, sort, null, null, null );
   expResult = dbOpr.findFromCL( mainCL, { "a": { "$gte": "z" } }, selector, sort, null, null, null );
   checkResult( expResult, actResult );

   var esIndexNames1 = dbOpr.getESIndexNames( COMMCSNAME, subCLName1, textIndexName );
   var esIndexNames2 = dbOpr.getESIndexNames( COMMCSNAME, subCLName2, textIndexName );
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   dropCL( db, COMMCSNAME, subCLName2, true, true );
   dropCL( db, COMMCSNAME, mainCLName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames1 );
   checkIndexNotExistInES( esIndexNames2 );
}
