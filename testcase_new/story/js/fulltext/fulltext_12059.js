/************************************
*@Description: 主子表中执行全文检索，全文索引字段为子表分区键
*@author:      liuxiaoxuan
*@createdate:  2018.11.27
*@testlinkCase: seqDB-12059
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

   var mainCLName = COMMCLNAME + "_ES_12059_maincl";
   var subCLName1 = COMMCLNAME + "_ES_12059_subcl_1";
   var subCLName2 = COMMCLNAME + "_ES_12059_subcl_2";
   dropCL( db, COMMCSNAME, mainCLName, true, true );
   dropCL( db, COMMCSNAME, subCLName1, true, true );
   dropCL( db, COMMCSNAME, subCLName2, true, true );

   var mainCL = commCreateCL( db, COMMCSNAME, mainCLName, { ShardingKey: { a: 1 }, IsMainCL: true } );
   var subCL1 = commCreateCL( db, COMMCSNAME, subCLName1 );
   var subCL2 = commCreateCL( db, COMMCSNAME, subCLName2, { ShardingKey: { a0: 1 }, ShardingType: "range", Group: groups[0][0]["GroupName"] } );
   subCL2.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { "a0": "zzz_1111_12059" }, { "a0": "zzz_1111_12059 9999" } );

   // attach cl
   mainCL.attachCL( COMMCSNAME + "." + subCLName1, { LowBound: { a: "testa" }, UpBound: { a: "testa_99999" } } );
   mainCL.attachCL( COMMCSNAME + "." + subCLName2, { LowBound: { a: "zzza" }, UpBound: { a: "zzza_99999" } } );
   var textIndexName = "textIndex12059";
   commCreateIndex( mainCL, textIndexName, { a0: "text" } );

   // insert
   var objs = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      objs.push( { a: "testa_" + i, a0: "test_12059 " + i } );
   }
   for( var i = 0; i < 5000; i++ )
   {
      objs.push( { a: "zzza_" + i, a0: "zzz_1111_12059 " + i } );
   }
   for( var i = 5000; i < 10000; i++ )
   {
      objs.push( { a: "zzza_" + i, a0: "zzz_2222_12059 " + i } );
   }
   mainCL.insert( objs );
   checkMainCLFullSyncToES( COMMCSNAME, mainCLName, textIndexName, 20000 );

   // return datas from one subcl
   var dbOpr = new DBOperator();
   var actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a0": "test_12059" } } } } }, { "a0": "" }, null, null, null, null );
   var expResult = dbOpr.findFromCL( mainCL, { "a0": { "$lt": "z" } }, { "a0": "" }, null, null, null, null );
   expResult.sort( compare( "a0" ) );
   actResult.sort( compare( "a0" ) );
   checkResult( expResult, actResult );

   // return datas from one group
   actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match": { "a0": "zzz_1111_12059" } } } } }, { "a0": "" }, null, null, null, null );
   expResult = dbOpr.findFromCL( mainCL, { "a0": { "$gt": "z", "$lt": "zzz_2222" } }, { "a0": "" }, null, null, null, null );
   expResult.sort( compare( "a0" ) );
   actResult.sort( compare( "a0" ) );
   checkResult( expResult, actResult );

   // return datas from more subcls and groups
   actResult = dbOpr.findFromCL( mainCL, { "": { $Text: { "query": { "match_all": {} } } } }, { "a0": "" }, null, null, null, null );
   expResult = dbOpr.findFromCL( mainCL, null, { "a0": "" }, null, null, null, null );
   expResult.sort( compare( "a0" ) );
   actResult.sort( compare( "a0" ) );
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
