/***************************************************************************
@Description :seqDB-11991 :普通表中已存在全文索引alter为range分区表
@Modify list :
              2018-11-02  YinZhen  Create
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

   var clName = COMMCLNAME + "_ES_11991";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groups[0][0]["GroupName"] } );
   var textIndexName = "fullIndex_11991";
   commCreateIndex( dbcl, textIndexName, { b: "text" } );

   //插入大量包含全文索引字段的数据
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );

   //alter为range切分表，切分键覆盖：非全文索引字段
   dbcl.alter( { ShardingType: "range", ShardingKey: { a: 1 } } )
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], 50 );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );
   var esOperator = new ESOperator();
   var dbOperator = new DBOperator();
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { b: "" } );
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var actResult = new Array();
   for( var i in esIndexNames )
   {
      var result = esOperator.findFromES( esIndexNames[i], '{"query" : {"match_all" : {}}, "size" : 10000}' );
      actResult = actResult.concat( result );
   }
   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );

   //alter为range切分表，切分键覆盖：全文索引字段
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groups[0][0]["GroupName"] } );
   commCreateIndex( dbcl, textIndexName, { b: "text" } );

   //插入大量包含全文索引字段的数据
   var records = new Array();
   for( var i = 0; i < 10000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );

   dbcl.alter( { ShardingType: "range", ShardingKey: { b: 1 } } )
   dbcl.split( groups[0][0]["GroupName"], groups[1][0]["GroupName"], 50 );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10000 );
   var esOperator = new ESOperator();
   var dbOperator = new DBOperator();
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { b: "" } );
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var actResult = new Array();
   for( var i in esIndexNames )
   {
      var result = esOperator.findFromES( esIndexNames[i], '{"query" : {"match_all" : {}}, "size" : 10000}' );
      actResult = actResult.concat( result );
   }
   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
