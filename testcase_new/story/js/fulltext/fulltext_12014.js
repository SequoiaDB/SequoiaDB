/***************************************************************************
@Description :seqDB-12014 :range分区表中插入/更新/删除包含全文索引字段的记录 
@Modify list :
              2018-11-21  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_12014";
   dropCL( db, COMMCSNAME, clName );
   var srcGroup = groups[0][0]["GroupName"];
   var desGroup = groups[1][0]["GroupName"];
   var cl = commCreateCL( db, COMMCSNAME, clName, { "ShardingType": "range", "ShardingKey": { "a": 1, "b": 1 }, "Group": srcGroup } );
   cl.split( srcGroup, desGroup, { "a": "a_1000" }, { "a": "a_6000" } );
   commCreateIndex( cl, "fullIndex_12014", { "a": "text", "b": "text", "c": "text" } );
   commCheckIndexConsistency( cl, "fullIndex_12014", true );

   //插入包含全文索引字段的记录
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { "a": "a_" + i, "b": "b_" + i, "c": "c_" + i };
      records.push( record );
   }
   cl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10000 );

   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   //更新包含全文索引字段的记录
   cl.update( { "$set": { "a": "a", "b": "b", "c": "c" } } );
   cl.insert( { "a": "a_10001", "b": "b_10001", "c": "c_10001" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 10001 );

   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   //删除包含全文索引字段的记录
   cl.remove( { "c": "c" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_12014", 1 );

   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_12014" );
   dropCL( db, COMMCSNAME, clName, true, true );

   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
