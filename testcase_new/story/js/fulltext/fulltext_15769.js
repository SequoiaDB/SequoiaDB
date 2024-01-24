/***************************************************************************
@Description :seqDB-15769 :非全文索引字段为唯一索引，更新记录使唯一索引重复 
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

   var clName = COMMCLNAME + "_ES_15769";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引并创建其他字段为唯一索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15769";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );
   commCreateIndex( dbcl, "nameIndex", { name: 1 }, { Unique: true } );

   //插入两条包含全文索引和唯一索引字段的记录
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

   //记录插入成功，原始集合、固定集合及ES端记录正确
   checkRecords( expESRecords, actESRecords );

   checkRecords( expCLRecords, actCLRecords );

   //更新其中一条记录的唯一索引字段与集合中其它记录的唯一索引字段重复
   updateRecords( dbcl );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var actCLRecords = dbOperator.findFromCL( dbcl, findConf, { about: "", content: "" }, null, null );

   //更新失败，报错-38，原始集合、固定集合及ES端记录无变化
   checkRecords( expESRecords, actESRecords );
   checkRecords( expCLRecords, actCLRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function updateRecords ( dbcl )
{
   try
   {
      dbcl.update( { $set: { name: "zsan" } }, { name: "lisi" } );
      throw new Error( 'update duplicate records should fail!' );
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
