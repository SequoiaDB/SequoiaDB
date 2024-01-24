/***************************************************************************
@Description :seqDB-12037 :在全文索引字段上执行普通查询 
@Modify list :
              2018-9-29  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12037";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含全文索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12037";
   commCreateIndex( dbcl, textIndexName, { content: "text" } );
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 20, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20 );

   //在全文索引字段上执行普通查询，查询时全文索引字段用于：条件、选择、排序，检查结果 
   var dbOperator = new DBOperator();
   var actRecords = dbOperator.findFromCL( dbcl, { content: { $exists: 1 } }, { content: "" }, { content: 1 }, null );

   var expRecords = new Array();
   for( var i in records )
   {
      var obj = new Object();
      obj.content = records[i].content;
      expRecords.push( obj );
   }
   expRecords.sort( compare( "content" ) );

   //在原集合上执行查询，查询结果正确
   checkRecords( expRecords, actRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   checkIndexNotExistInES( esIndexNames );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "content" ) );
   actRecords.sort( compare( "content" ) );
   checkResult( expRecords, actRecords )
}
