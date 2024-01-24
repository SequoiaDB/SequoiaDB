/***************************************************************************
@Description :seqDB-15768 :全文索引字段同时为唯一索引，更新使记录的唯一索引重复 
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

   var clName = COMMCLNAME + "_ES_15768";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引且同时为唯一索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15768";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   commCreateIndex( dbcl, "contentIndex", { content: 1 }, { Unique: true } );

   //插入包含全文索引的记录 
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

   //记录插入成功，原始集合、固定集合及ES端记录正确
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   //更新使记录的唯一索引字段与与集合中其它记录的唯一索引重复
   updateRecords( dbcl );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //更新失败，报错-38，检查原始集合、固定集合及主备数据节点数据无差别
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );
   checkConsistency( COMMCSNAME, clName );
   checkInspectResult( COMMCSNAME, clName, 5 );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function updateRecords ( dbcl )
{
   try
   {
      dbcl.update( { $set: { content: "this is my hometown" } }, { content: "this is my college" } );
      throw new Error( "update duplicate key should fail!" );
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
