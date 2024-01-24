/***************************************************************************
@Description :seqDB-12032 :AutoIndexId为false时删除记录 
@Modify list :
              2018-9-30  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   };

   var clName = COMMCLNAME + "_ES_12032";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合指定AutoIndexId为false    
   var dbcl = commCreateCL( db, COMMCSNAME, clName, { AutoIndexId: false } );

   //创建全文索引，并插入包含全文索引字段的记录
   var textIndexName = "textIndexName_ES_12032";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );
   var records = new Array();
   records[0] = { content: "this is my college" };
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   //删除记录，检查结果
   removeRecords( dbcl );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   //记录删除失败，固定集合中记录正确，ES中记录正确
   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } }
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var expCLRecords = dbOperator.findFromCL( dbcl, findConf, { content: "" }, null, null );
   checkRecords( expCLRecords, actESRecords );

   //创建id索引后，再次删除记录，检查结果
   dbcl.createIdIndex();
   dbcl.remove( { content: "this is my college" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 0 );

   //记录删除成功，ES中最终无记录
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var expCLRecords = dbOperator.findFromCL( dbcl, findConf, { content: "" }, null, null );
   checkRecords( expCLRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function removeRecords ( dbcl )
{
   assert.tryThrow( SDB_RTN_AUTOINDEXID_IS_FALSE, function()
   {
      dbcl.remove( { content: "this is my college" } );
   } );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
