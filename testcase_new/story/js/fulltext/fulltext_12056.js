/************************************
*@Description: 主子表中执行全文检索,全文索引字段为非分区键 
*@author:      liuxiaoxuan
*@createdate:  2018.11.27
*@testlinkCase: seqDB-12056
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

   var mainCLName = COMMCLNAME + "_ES_12056_maincl";
   var subCLName1 = COMMCLNAME + "_ES_12056_subcl_1";
   var subCLName2 = COMMCLNAME + "_ES_12056_subcl_2";
   dropCL( db, COMMCSNAME, mainCLName, true, true );
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   dropCL( db, COMMCSNAME, subCLName2, true, true );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   commCreateCL( db, COMMCSNAME, subCLName1 );
   commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a0: 1 }, ShardingType: "range", Group: groups[0][0]["GroupName"] } );

   // attach cl
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: "testa" }, UpBound: { a: "testa_99999" } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: "zzza" }, UpBound: { a: "zzza_99999" } } );
   var textIndexName = "textIndex12056";
   commCreateIndex( mainCL, textIndexName, { b: "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "testa_" + i, a0: "test_a0 " + i, b: "test_12056 " + i } );
   }
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "zzza_" + i, a0: "zzz_a0 " + i, b: "test_12056 " + ( 10000 + i ) } );
   }
   mainCL.insert( objs );
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, textIndexName, 20000 );

   // check result
   var dbOpr = new DBOperator();
   var actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "b": "test_12056" } } } } }, { "b": "" }, null, null, null, null );
   var expResult = dbOpr.findFromCL( mainCL, null, { "b": "" }, null, null, null, null );
   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
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
