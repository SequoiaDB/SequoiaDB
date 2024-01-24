/***************************************************************************
@Description : seqDB-12013 :自动切分的hash分区表中插入/更新/删除包含全文索引字段的记录 
@Modify list : 2018-11-21  YinZhen  Create
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

   var csName = "cs_12013";
   var clName = "cl_12013";
   var domainName = "domain_12013";
   commDropDomain( db, domainName );
   dropCS( db, csName );
   commCreateDomain( db, domainName, [groups[0][0]["GroupName"], groups[1][0]["GroupName"]] );
   var cs = db.createCS( csName, { "Domain": domainName } );
   var cl = commCreateCL( db, csName, clName, { "ShardingType": "hash", "ShardingKey": { "a": 1 }, "AutoSplit": true } );
   commCreateIndex( cl, "fullIndex_12013", { "a": "text", "b": "text" } );
   commCheckIndexConsistency( cl, "fullIndex_12013", true );

   //插入包含全文索引字段的记录
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { "a": "a_" + i, "b": "b_" + i };
      records.push( record );
   }
   cl.insert( records );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10000 );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( csName, clName, "fullIndex_12013" );
   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );

   //更新包含全文索引字段的记录
   cl.update( { $set: { "a": "a", "b": "b" } } );
   cl.insert( { a: "a_10001", b: "b_10001" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 10001 );

   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );

   //删除包含全文索引字段的记录
   cl.remove( { b: "b" } );
   checkFullSyncToES( csName, clName, "fullIndex_12013", 1 );

   var actResult = dbOperator.findFromCL( cl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( cl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );
   checkConsistency( csName, clName );
   checkInspectResult( csName, clName, 5 );

   dropCS( db, csName );
   commDropDomain( db, domainName );

   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
