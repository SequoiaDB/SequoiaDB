/***************************************************************************
@Description :seqDB-15762 :集合中已存在记录，创建全文索引字段为唯一索引 
@Modify list :
              2018-10-09  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_ES_15762";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15762";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   //插入两条相同且包含全文索引字段的记录
   var records = new Array();
   records[0] = { about: "about for you", content: "this is my college" };
   records[1] = { about: "about for you", content: "this is my college" };
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   var esOperator = new ESOperator();
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var findConf = { "": { $Text: { "query": { "exists": { "field": "content" } } } } };
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   var expESRecords = new Array();
   expESRecords.push( { about: "about for you", content: "this is my college" } );
   expESRecords.push( { about: "about for you", content: "this is my college" } );
   var expCLRecords = records;

   //记录插入成功，原始集合、固定集合及ES端记录正确
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //创建全文索引字段为唯一索引，检查结果
   createUniqueIndex( dbcl );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //唯一索引创建失败，报错-38，检查集合索引、原始集合、固定集合记录无变化使用inspect工具检测主备节点数据一致，ES上记录无变化 
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );

   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function createUniqueIndex ( dbcl )
{
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.createIndex( "contentIndex", { content: 1 }, true );
   } );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
