/***************************************************************************
@Description :seqDB-12040 :带选择条件进行全文检索 
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

   var clName = COMMCLNAME + "_ES_12040";
   dropCL( db, COMMCSNAME, clName, true, true );

   //在同一字段上创建全文索引及普通索引，并插入包含索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12040";
   commCreateIndex( dbcl, textIndexName, { content: "text", about: "text" } );
   commCreateIndex( dbcl, "commIndex", { content: 1 } );

   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 30, "string", ["about", "content", "information"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 30 );

   //使用全文索引字段进行查询并对字段进行选择，查询覆盖：普通查询、全文检索，选择符进行抽测，选择字段覆盖：_id、部分全文索引字段、全部索引字段、其他字段，检查结果 
   var dbOperator = new DBOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var selectorConf = { _id: { $include: 1 }, about: { $include: 1 }, content: { $default: "this is content " }, information: "" };
   var hintConf = { "": textIndexName };
   var actRecords = dbOperator.findFromCL( dbcl, findConf, selectorConf, null, hintConf );

   var expRecords = dbOperator.findFromCL( dbcl, null, null, null, null );
   checkRecords( expRecords, actRecords );

   var selectorConf = { about: { $include: 1 }, information: { $default: "this is information " } };
   var hintConf = { "": "commIndex" };
   var actRecords = dbOperator.findFromCL( dbcl, null, selectorConf, null, hintConf );

   var expRecords = getExpRecords( records );
   checkRecords( expRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function getExpRecords ( records )
{
   var expRecordsCommSearch = new Array();
   for( var i in records )
   {
      var obj = new Object();
      obj.about = records[i].about;
      obj.information = records[i].information;
      expRecordsCommSearch.push( obj );
   }
   return expRecordsCommSearch;
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about", compare( "information" ) ) );
   actRecords.sort( compare( "about", compare( "information" ) ) );
   checkResult( expRecords, actRecords )
}
