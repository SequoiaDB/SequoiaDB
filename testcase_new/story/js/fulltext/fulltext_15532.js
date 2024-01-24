/***************************************************************************
@Description :seqDB-15532 :插入_id字段重复的记录 
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

   var clName = COMMCLNAME + "_ES_15532";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建集合并创建全文索引
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var textIndexName = "textIndexName_ES_15532";
   commCreateIndex( dbcl, textIndexName, { about: "text", content: "text" } );

   //指定_id插入记录，覆盖：包含全文索引字段、不包含全文索引字段，检查结果
   var records = new Array();
   records[0] = { _id: 1001, about: "about for you", content: "this is my college", name: "zsan", age: 18 };
   records[1] = { _id: 1002, name: "lisi", age: 20, addr: "JinagXi" };
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, textIndexName );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 1 );

   var esOperator = new ESOperator();
   var queryCond = '{"query" : {"exists" : {"field" : "content"}}}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );

   var expESRecords = new Array();
   expESRecords.push( { about: "about for you", content: "this is my college" } );

   //记录插入成功，固定集合及ES端记录正确
   checkRecords( expESRecords, actESRecords );

   //再次执行步骤1，检查结果
   insertRecordsAgain( dbcl, records );

   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );

   //记录插入失败，固定集合及ES端记录无变化
   checkRecords( expESRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function insertRecordsAgain ( dbcl, records )
{
   assert.tryThrow( SDB_IXM_DUP_KEY, function()
   {
      dbcl.insert( records );
   } );
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "about" ) );
   actRecords.sort( compare( "about" ) );
   checkResult( expRecords, actRecords )
}
