/***************************************************************************
@Description :seqDB-12033 :删除不存在的记录 
@Modify list :
              2018-9-30  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12033";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12033";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 20, "string", ["about", "content"] );
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   //删除不存在的记录，检查结果 
   dbcl.remove( { about: "what about you aaaaaaaaaaaaaaaaaaaaaa", content: "this is my contentaaaaaaaaaaaaaaaaaaaaaaaaaaa" } );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   //命令行执行成功，固定集合中不新增记录，es中最终与原集合数据一致
   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } }
   var queryCond = '{"query" : {"exists" : {"field" : "about"}}, "size" : 30}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var expCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );
   checkRecords( expCLRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about", compare( "content" ) ) );
   actRecords.sort( compare( "about", compare( "content" ) ) );
   checkResult( expRecords, actRecords )
}
