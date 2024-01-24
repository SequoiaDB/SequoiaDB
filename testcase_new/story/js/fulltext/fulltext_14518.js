/***************************************************************************
@Description :seqDB-14518 :range分区表中带sort执行全文检索 
@Modify list :
              2018-10-31  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_14518";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建range分区表，并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { ShardingType: "range", ShardingKey: { a: 1 }, Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, "fullIndex_14518", { a: "text" } );

   //插入包含全文索引字段的记录
   var records = new Array();
   for( var i = 0; i < 6000; i++ )
   {
      var record = { a: "a" + parseInt( Math.random() * 1000000 ) };
      records.push( record );
   }
   for( var i = 0; i < 4000; i++ )
   {
      var record = { a: "f" + parseInt( Math.random() * 1000000 ) };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14518", 10000 );
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], { a: "c" }, { a: "g" } );
   checkFullSyncToES( COMMCSNAME, clName, "fullIndex_14518", 10000 );

   //查询发送到单组
   var dbOperator = new DBOperator();
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "regexp": { "a": "a[0-9]*" } } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, { "a": { $regex: "a[0-9]*" } }, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "regexp": { "a": "a[0-9]*" } } } } }, { a: "" }, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, { "a": { $regex: "a[0-9]*" } }, { a: "" }, { "a": 1 } );
   checkResult( expResult, actResult );
   //查询发送到多组
   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, null, { "_id": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, null, { "_id": 1 } );
   checkResult( expResult, actResult );

   var actResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { a: "" }, { "a": 1 } );
   var expResult = dbOperator.findFromCL( dbcl, null, { a: "" }, { "a": 1 } );
   checkResult( expResult, actResult );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, "fullIndex_14518" );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
