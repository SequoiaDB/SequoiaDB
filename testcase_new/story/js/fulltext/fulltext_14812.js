/************************************
*@Description: 全文检索查询条件值长度验证  
*@author:      liuxiaoxuan
*@createdate:  2018.10.09
*@testlinkCase: seqDB-14812
**************************************/
main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   //create CL
   var clName = COMMCLNAME + "_ES_14812";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndex_14812";
   dbcl.createIndex( textIndexName, { "a": "text" } );

   // insert record length < 1000 and > 1000
   var strings4 = getStringsByLength( 4 );
   var strings1024 = getStringsByLength( 1024 );
   var objs = new Array();
   for( var i = 0; i < 10000; i++ ) { objs.push( { a: strings4 + "_" + i } ); }
   for( var i = 0; i < 10; i++ ) { objs.push( { a: strings1024 + "_" + i } ); }
   dbcl.insert( objs );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10010 );

   // match all records
   var dbOperator = new DBOperator();
   var findAllConf = { "": { "$Text": { "query": { "match_all": {} } } } };
   var actResult = dbOperator.findFromCL( dbcl, findAllConf, { "a": { "$include": 1 } } );
   var expResult = dbOperator.findFromCL( dbcl, null, { "a": { "$include": 1 } } );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   // match + conds
   var strings = getStringsByLength( 1024 );
   var findMatchConf = { "": { "$Text": { "query": { "match": { "a": strings } } } } };
   var actResult = dbOperator.findFromCL( dbcl, findMatchConf, { "a": { "$include": 1 } } );
   var expResult = dbOperator.findFromCL( dbcl, { "a": { "$gte": strings } }, { "a": { "$include": 1 } } );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
function getStringsByLength ( strLen )
{
   var strings = "";
   var stringArray = new Array();
   for( var i = 0; i < strLen; i++ ) { stringArray.push( "a" ); }
   strings = stringArray.join( "" );
   return strings;
}
