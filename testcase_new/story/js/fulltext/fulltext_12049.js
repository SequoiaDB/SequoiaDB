/***************************************************************************
@Description :seqDB-12049 :hash分区表中带选择符及与sort的组合执行全文检索  
@Modify list :2018-11-22  Zhaoxiaoni  init
****************************************************************************/
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

   var clName = COMMCLNAME + "_ES_12049";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_12049", { a: "text" } );

   var records = [];
   for( var i = 0; i < 30000; i++ )
   {
      var record = { a: "a" + i, b: i };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12049", 30000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { "a": "" } );
   var expResult = dbOperator.findFromCL( dbcl, null, { "a": "" } );
   checkResult( expResult.sort( compare( "a" ) ), actResult.sort( compare( "a" ) ) );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { "b": "" } );
   var expResult = dbOperator.findFromCL( dbcl, null, { "b": "" } );
   checkResult( expResult.sort( compare( "b" ) ), actResult.sort( compare( "b" ) ) );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { "a": "" }, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, { "a": "" }, { "a": 1 } );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { "b": "" }, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, { "b": "" }, { "a": 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12049" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
