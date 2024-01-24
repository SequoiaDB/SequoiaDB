/***************************************************************************
@Description :seqDB-11992 :普通表中已存在全文索引alter为hash分区表 
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

   var clName = COMMCLNAME + "_ES_11992";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName, { Group: groups[0][0]["GroupName"] } );
   var textIndexName = "fullIndex_11992";
   commCreateIndex( dbcl, textIndexName, { b: "text" } );

   //插入大量包含全文索引字段的数据
   var records = new Array();
   for( var i = 0; i < 8000; i++ )
   {
      var record = { a: "a" + i, b: "b" + i };
      records.push( record );
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 8000 );

   //alter为hash切分表，切分键覆盖：非全文索引字段
   dbcl.alter( { ShardingType: "hash", ShardingKey: { a: 1 } } )

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 8000 );
   var esOperator = new ESOperator();
   var dbOperator = new DBOperator();
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { b: "" } );
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var esIndexName = esIndexNames[0];
   var actResult = esOperator.findFromES( esIndexName, '{"query" : {"match_all" : {}}, "size" : 10000}' );
   expResult.sort( compare( "b" ) );
   actResult.sort( compare( "b" ) );
   checkResult( expResult, actResult );

   //alter为hash切分表，切分键覆盖：全文索引字段
   dropIndex( dbcl, textIndexName );
   checkIndexNotExistInES( esIndexNames );

   commCreateIndex( dbcl, textIndexName, { a: "text" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 8000 );
   var expResult = dbOperator.findFromCL( dbcl, { "": { $Text: { "query": { "match_all": {} } } } }, { a: "" } );
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var esIndexName = esIndexNames[0];
   var actResult = esOperator.findFromES( esIndexName, '{"query" : {"match_all" : {}}, "size" : 10000}' );
   expResult.sort( compare( "a" ) );
   actResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   dropCL( db, COMMCSNAME, clName, true, true );

   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
