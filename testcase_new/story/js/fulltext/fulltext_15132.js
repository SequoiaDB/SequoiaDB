/***************************************************************************
@Description :seqDB-15132 :range分区表中带limit/skip进行全文检索 
@Modify list :
              2018-10-29  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_15132";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建range分区表，并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_15132", { a: "text" } );

   //插入包含全文索引字段的记录
   var records = new Array();
   for( var i = 0; i < 25000; i++ )
   {
      var record = { a: "a" };
      records.push( record );
   }
   for( var i = 0; i < 5000; i++ )
   {
      var record = { a: "f" };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_15132", 30000 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { a: "c" }, { a: "g" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_15132", 30000 );

   //skip/limit组合查询结果覆盖单组返回
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 4000, 1000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 4000, 1000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 5000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 5000, 5000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 6000, 6000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 6000, 6000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 4000, 11000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 4000, 11000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 12000, 13000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 12000, 13000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, null, null, 15000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, null, null, 15000, 5000 );
   checkCount( expResult, actResult );

   //skip/limit组合查询结果覆盖多组返回
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 4000, 1000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, null, null, 4000, 1000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 5000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, null, null, 5000, 5000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 9000, 9000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, null, null, 9000, 9000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 8000, 15000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, null, null, 8000, 15000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 12000, 18000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 }, null, 12000, 18000 );
   checkCount( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, null, null, 15000, 9000 );
   var expResult = dbOperator.findFromCL( dbcl, null, null, null, null, 15000, 9000 );
   checkCount( expResult, actResult );

   //skip/limit/sort组合查询结果覆盖单组返回
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 4000, 1000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 4000, 1000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 5000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 5000, 5000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 6000, 6000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 6000, 6000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 4000, 11000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 4000, 11000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 12000, 13000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 12000, 13000 );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match": { "a": "a" } } } } }, null, { "_id": 1 }, null, 15000, 5000 );
   var expResult = dbOperator.findFromCL( dbcl, { "a": "a" }, null, { "_id": 1 }, null, 15000, 5000 );
   checkResult( expResult, actResult );

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

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_15132" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkCount ( expResult, actResult )
{
   if( actResult.length != expResult.length )
   {
      throw new Error( "expect result length: " + expResult.length + ",actual result length: " + actResult.length );
   }
}
