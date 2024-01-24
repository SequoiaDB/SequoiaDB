/***************************************************************************
@Description :seqDB-14373 :插入重复的记录 
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

   var clName = COMMCLNAME + "_ES_14373";
   dropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   //插入多条全文索引字段重复的记录
   var records = new Array();
   for( var i = 0; i < 3; i++ )
   {
      records[i] = { about: "my name is zsan", content: "this is my college" };
   }
   dbcl.insert( records );

   //创建全文索引，再次插入多条全文索引字段重复的记录
   var textIndexName = "textIndexName_ES_14373";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 6 );

   //记录插入成功，固定集合中记录正确，ES中记录正确
   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } }
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var expCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );
   checkRecords( expCLRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
