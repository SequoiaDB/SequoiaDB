/***************************************************************************
@Description :seqDB-12047 :hash分区表中执行全文检索  
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

   var clName = COMMCLNAME + "_ES_12047";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_12047", { a: "text", b: "text" } );

   var records = [];
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "i" };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12047", 10000 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { Partion: 1 }, { Partion: 2048 } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12047", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12047" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
