/***************************************************************************
@Description :seqDB-15767 :全文索引及唯一索引字段为不同字段，插入唯一索引重复记录 
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

   var clName = COMMCLNAME + "_ES_15767";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引，另创建其他字段为唯一索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15767";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   commCreateIndex( dbcl, "nameIndex", { name: 1 }, { Unique: true } );

   //插入的记录包含唯一索引及全文索引字段
   var records = new Array();
   records[0] = { about: "about for you", content: "this is my college", name: "zsan" };
   records[1] = { about: "how it go on", content: "this is my hometown", name: "lisi" };
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

   //记录插入成功，固定集合记录插入成功，ES同步记录成功
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //重复步骤2 ，检查结果
   insertRecordsAgain( dbcl, records );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //重复插入报错-38，检查原始集合及各数据节点固定集合记录，无记录被删除，固定集合中未新增操作记录，ES 上记录未被删除 。
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
   try
   {
      dbcl.insert( records );
   }
   catch( e )
   {
      if( e.message != SDB_IXM_DUP_KEY )
      {
         throw e;
      }
   }
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
