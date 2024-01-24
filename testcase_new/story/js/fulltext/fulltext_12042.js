/***************************************************************************
@Description :seqDB-12042 :全文检索返回多条记录 
@Modify list :
              2018-9-28  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12042";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12042";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 40000, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 40000 );

   //指定索引字段进行全文检索，返回记录数覆盖与ES需要多次交互(超过1w条)，检查结果
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var dbOperator = new DBOperator();
   var actRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );
   var expRecords = records;

   checkRecords( expRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about", compare( "content" ) ) );
   actRecords.sort( compare( "about", compare( "content" ) ) );
   checkResult( expRecords, actRecords )
}
