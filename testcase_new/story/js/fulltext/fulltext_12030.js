/***************************************************************************
@Description :seqDB-12030 :使用truncate删除记录 
@Modify list :
              2018-10-08  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12030";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var fullIndex = "fullIndex_ES_12030";
   commCreateIndex( dbcl, fullIndex, { about: "text", content: "text" } );

   //插入包含全文索引字段的记录 
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 10, "string", ["about", "content"] );
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, fullIndex );
   var cappedCLs = dbOperator.getCappedCLs( COMMCSNAME, clName, fullIndex );
   checkFullSyncToES( COMMCSNAME, clName, fullIndex, 10 );

   //使用truncate删除记录，检查结果    
   dbcl.truncate();

   checkFullSyncToES( COMMCSNAME, clName, fullIndex, 0 );

   //记录删除成功，原始集合、固定集合、ES中记录被全文清除，记录数为0
   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } }
   var queryCond = '{"query" : {"exists" : {"field" : "about"}}, "size" : 30}';
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, null, null, null );
   var actCappedCLRecords = dbOperator.findFromCL( cappedCLs[0], null, null, null, null );
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );

   var expRecords = new Array();

   checkRecords( expRecords, actCLRecords );
   checkRecords( expRecords, actCappedCLRecords );
   checkRecords( expRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "abour" ) );
   checkResult( expRecords, actRecords )
}
