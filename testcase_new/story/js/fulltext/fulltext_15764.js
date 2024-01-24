/***************************************************************************
@Description :seqDB-15764 :集合中已存在记录，创建其它字段为唯一索引 
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

   var clName = COMMCLNAME + "_ES_15764";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15764";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   //插入非全文索引字段重复的记录
   var records = new Array();
   records[0] = { about: "about for you", content: "this is my college", name: "zsan" };
   records[1] = { about: "how it go on", content: "this is my hometown", name: "zsan" };
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
   expESRecords.push( { about: "how it go on", content: "this is my hometown" } );
   var expCLRecords = new Array();
   expCLRecords.push( { about: "about for you", content: "this is my college" } );
   expCLRecords.push( { about: "how it go on", content: "this is my hometown" } );

   //记录插入成功，检查原始集合、固定集合及ES中已同步记录
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //创建步骤2存在重复记录的字段为唯一索引，检查结果
   createDuplicateIndex( dbcl );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //唯一索引创建失败，报错-38，检查集合索引、原始集合、固定集合及ES的记录无变化，使用inspect工具检测主备数据节点数据无差别
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function createDuplicateIndex ( dbcl )
{
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.createIndex( "nameIndex", { name: 1 }, true );
   } );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
