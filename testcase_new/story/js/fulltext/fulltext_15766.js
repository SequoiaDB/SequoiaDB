/***************************************************************************
@Description :seqDB-15766 :全文索引同时为唯一索引，插入唯一索引重复记录 
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

   var clName = COMMCLNAME + "_ES_15766";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引，创建全文索引字段同时为唯一索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15766";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   commCreateIndex( dbcl, "contentIndex", { content: 1 }, { Unique: true } );

   //插入包含全文索引字段的记录 
   var records = new Array();
   records[0] = { about: "about for you", content: "this is my college" };
   records[1] = { about: "how it go on", content: "this is my hometown" };
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
   var expCLRecords = records;

   //记录插入成功，固定集合中新增记录，ES同步该条记录成功
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //重复执行步骤2，检查结果
   insertRecordsAgain( dbcl, records );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //重复执行后报错-38 ，检查原始集合、固定集合记录无变化，使用inspect工具检测主备节点数据一致，ES上记录无变化 
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertRecordsAgain ( dbcl, records )
{
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( records );
   } );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
