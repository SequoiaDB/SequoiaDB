/***************************************************************************
Description :seqDB-12064 :带sort进行全文检索 
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

   var clName = COMMCLNAME + "_ES_12064";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var textIndexName = "textIndexName_ES_12064";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 12000, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 12000 );

   //使用全文索引字段进行查询并使用sort,返回的记录数超过1万条，检查结果
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var sortConf = { about: 1, content: 1, _id: 1 };
   var dbOperator = new DBOperator();
   var actRecords = dbOperator.findFromCL( dbcl, findConf, null, sortConf, null );
   var expRecords = dbOperator.findFromCL( dbcl, null, null, sortConf, null );

   checkResult( actRecords, expRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   checkIndexNotExistInES( esIndexNames );
}
