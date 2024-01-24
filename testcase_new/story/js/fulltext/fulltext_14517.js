/***************************************************************************
@Description :seqDB-14517 :hash分区表中带sort执行全文检索 
@Modify list :
              2018-11-01  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_14517";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建range分区表，并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "hash", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_14517", { a: "text" } );

   //插入包含全文索引字段的记录
   var records = new Array();
   for( var i = 0; i < 100; i++ )
   {
      var record = { a: "a" + parseInt( Math.random() * 1000000 ) };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14517", 100 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], 50 );

   var dbOperator = new DBOperator();

   //查询单组上有记录数超过3条的记录
   var result = new Array();
   var groupName = "";
   for( var i in groups )
   {
      groupName = groups[i][0]["GroupName"];
      var db2 = db.getRG( groupName ).getMaster().connect();
      var dbcl2 = db2.getCS( COMMCSNAME ).getCL( clName );
      result = dbOperator.findFromCL( dbcl2 );
      if( result.length >= 3 )
      {
         break;
      }
   }

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14517", 100 );

   //查询发送到单组
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "bool": { "should": [{ "match": { "a": result[0]["a"] } }, { "match": { "a": result[1]["a"] } }] } } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, { $or: [{ a: result[0]["a"] }, { a: result[1]["a"] }] }, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "bool": { "should": [{ "match": { "a": result[0]["a"] } }, { "match": { "a": result[1]["a"] } }] } } } } }, null, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, { $or: [{ a: result[0]["a"] }, { a: result[1]["a"] }] }, null, { "a": 1 } );
   checkResult( expResult, actResult );
   //查询发送到多组
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "a": 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_14517" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
