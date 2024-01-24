/***************************************************************************
@Description :seqDB-12031 :使用find.remove删除记录 
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

   var clName = COMMCLNAME + "_ES_12031";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12031";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   //插入包含全文索引字段的记录 
   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records[i] = { about: "about" + i, content: "content" + i };
   }
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   var cappedCLs = dbOperator.getCappedCLs( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 10 );

   //使用find.remove接口删除记录，检查结果 
   var cursor = dbcl.find( { about: { "$gt": "about1" } } ).remove(); //dbcl.find({about : {"$gt": "about1"}}).remove(); can not remove records
   while( cursor.next() ) { }

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 2 );

   //记录更新成功，固定集合中记录正确，操作类型正确，es中记录最终与原集合一致
   var esOperator = new ESOperator();
   var count = cappedCLs[0].find( { Type: 2 } ).count();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );
   var queryCond = '{"query" : {"exists" : {"field" : "about"}}, "size" : 20}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );

   if( count > 8 )
   {
      throw new Error( "expect record less than num: 8, actual record num: " + count );
   }

   checkRecords( actESRecords, actCLRecords );

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
