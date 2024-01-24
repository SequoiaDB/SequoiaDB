/***************************************************************************
@Description :seqDB-14396 :在findOne中使用全文检索 
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

   var clName = COMMCLNAME + "_ES_14396";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引及普通索引，并插入包含索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_14396";
   commCreateIndex( dbcl, textIndexName, { about: "text" } );
   commCreateIndex( dbcl, "commIndex", { about: 1 } );
   var records = new Array();
   for( var i = 0; i < 5; i++ )
   {
      records[i] = { about: "i am a robot " + i };
   }
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 5 );

   //在findOne中使用全文检索条件，覆盖：只带全文检索条件、混合查询，检查结果 
   var rec = dbcl.findOne( { "": { $Text: { "query": { "match_all": {} } } } }, { about: "" } );
   var actOnlyFullRecords = new Array();
   while( rec.next() )
   {
      actOnlyFullRecords.push( rec.current().toObj() );
   }

   var rec = dbcl.findOne( { $and: [{ "": { $Text: { "query": { "match_all": {} } } } }, { about: { $exists: 1 } }] }, { about: "" } );
   var actMixRecords = new Array();
   while( rec.next() )
   {
      actMixRecords.push( rec.current().toObj() );
   }

   var expRecords = new Array();
   expRecords.push( records[0] );

   checkRecords( expRecords, actOnlyFullRecords );
   checkRecords( expRecords, actMixRecords );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
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
