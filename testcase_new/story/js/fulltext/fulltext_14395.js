/***************************************************************************
@Description :seqDB-14395 :在cont使用全文检索 
@Modify list :
              2018-9-30  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_14395";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含索引字段的记录，记录总数大于1万条 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_14395";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 30000, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 30000 );

   //在count命令字中使用全文检索执行查询，覆盖：无记录匹配、部分记录匹配(大于1w条)、记录全匹配(大于1w条)，检查结果
   var actCount = dbcl.count( { "": { $Text: { "query": { "match": { "content": "movieaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa" } } } } } );
   checkAllResult( actCount, 0 );

   var dbOperator = new DBOperator();
   var actCount = dbcl.count( { "": { $Text: { "query": { "match_all": {} } } }, content: { $gt: "c" } } );
   var expCount = dbOperator.findFromCL( dbcl, { "content": { $gt: "c" } }, null, null, null ).length;
   checkAllResult( actCount, expCount );

   var actCount = dbcl.count( { "": { $Text: { "query": { "match_all": {} } } } } );
   checkAllResult( actCount, 30000 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkAllResult ( actCount, expCount )
{
   if( actCount != expCount )
   {
      throw new Error( "expect record num: " + expCount + ",actual record num: " + actCount );
   }
}
