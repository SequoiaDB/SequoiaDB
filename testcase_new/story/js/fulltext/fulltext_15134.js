/***************************************************************************
@Description :seqDB-15134 :range分区表中带limit/skip进行全文检索 
@Modify list :2018-11-21  Zhaoxiaoni  init
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

   var clName = COMMCLNAME + "_ES_15134";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_15134", { a: "text" } );

   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { Partion: 1 }, { Partion: 2048 } );

   var records = [];
   for( var i = 0; i < 30000; i++ )
   {
      var record = { a: "a" + i };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_15134", 30000 );

   //skip/limit/sort组合查询结果覆盖多组返回
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 4000, 1000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 4000, 1000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 5000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 5000, 5000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 9000, 9000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 9000, 9000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 8000, 15000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 8000, 15000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 12000, 18000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 12000, 18000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 }, null, 15000, 9000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 15000, 9000 );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_15134" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
