/***************************************************************************
@Description :seqDB-12036 :使用DSL的方式进行结构化搜索和全文搜索 
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

   var clName = COMMCLNAME + "_ES_12036";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含全文索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12036";
   commCreateIndex( dbcl, textIndexName, { content: "text", about: "text" } );
   var records = insertData( dbcl );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 8 );

   //使用DSL的方式进行全文检索,在es中执行查询，查询结果正确

   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match": { "content": "college" } } } } };
   var queryCond = '{"query" : {"term" : {"content" : "college"}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = dbOperator.findFromCL( dbcl, findConf, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"match" : {"about" : "这是我的"}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = dbOperator.findFromCL( dbcl, findConf, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"match_phrase" : {"content" : "not got"}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var findConfNot = { "": { $Text: { "query": { "match": { "content": "not" } } } } };
   var clRecords = dbOperator.findFromCL( dbcl, findConfNot, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"multi_match" : {"query" : "you", "fields" : ["content", "about"]}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = dbOperator.findFromCL( dbcl, findConfNot, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"bool" : {"must" : [{"match" : {"content" : "not"}}, {"match" : {"about" : "you"}}]}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = dbOperator.findFromCL( dbcl, findConfNot, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"bool" : {"must_not" : {"match" : {"about" : "you"}}}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = dbOperator.findFromCL( dbcl, findConf, { content: "", about: "" }, null, null );
   checkRecords( esRecords, clRecords );

   var queryCond = '{"query" : {"bool" : {"should" : [{"match" : {"content" : "college"}}, {"match" : {"about" : "you"}}]}}}';
   var esRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var clRecords = records;
   checkRecords( esRecords, clRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertData ( dbcl )
{
   var records = new Array();
   for( var i = 0; i < 3; i++ )
   {
      var obj = { content: "i have not,got " + i, about: "do you have " + i };
      records.push( obj );
   }
   for( var i = 0; i < 5; i++ )
   {
      var obj = { content: "this is my college, i have " + i, about: "这是我的" };
      records.push( obj );
   }
   dbcl.insert( records );
   return records;
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "content" ) );
   actRecords.sort( compare( "content" ) );
   checkResult( expRecords, actRecords )
}
