/***************************************************************************
@Description :seqDB-12041 :查询在备节点进行全文检索 
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

   var clName = COMMCLNAME + "_ES_12041";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引，并插入包含索引字段的记录 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_12041";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   var dataGenerator = new commDataGenerator();
   var records = dataGenerator.getRecords( 30, "string", ["about", "content"] );
   dbcl.insert( records );

   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 30 );

   //指定索引字段进行全文检索，指定查询覆盖：走主节点、走备节点，检查结果 
   var masterDB = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   masterDB.setSessionAttr( { PreferedInstance: "M" } );
   var dbOperator = new DBOperator();
   var masActRecords = getActRecords( textIndexName, dbOperator, dbcl );
   var expRecords = records;
   masterDB.close();
   checkRecords( expRecords, masActRecords );

   var slaveDB = new Sdb( COORDHOSTNAME, COORDSVCNAME );
   slaveDB.setSessionAttr( { PreferedInstance: "S" } );
   var slaActRecords = getActRecords( textIndexName, dbOperator, dbcl );
   slaveDB.close();
   checkRecords( expRecords, slaActRecords );

   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function getActRecords ( textIndexName, dbOperator, dbcl )
{
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } };
   var actRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );
   return actRecords;
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
