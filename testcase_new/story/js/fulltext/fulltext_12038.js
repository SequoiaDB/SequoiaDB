/***************************************************************************
@Description :seqDB-12038 :普通索引字段同时是全文索引字段，在该字段上执行查询 
@Modify list :
              2018-9-29  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12038";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引同时在该字段上创建索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12038";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );
   commCreateIndex( dbcl, "commonIndex", { content: 1 } );

   //插入包含全文索引字段的记录 
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 20, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   //在索引字段上执行查询，覆盖:普通查询、全文检索，检查结果 
   var dbOperator = new DBOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var actRecordsFullSearch = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, { "": textIndexName } );
   var actRecordsCommSearch = dbOperator.findFromCL( dbcl, null, { about: "", content: "" }, null, { "": "commonIndex" } );

   var expRecords = records;

   checkRecords( expRecords, actRecordsFullSearch );
   checkRecords( expRecords, actRecordsCommSearch );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "content" ) );
   actRecords.sort( compare( "content" ) );
   checkResult( expRecords, actRecords )
}
