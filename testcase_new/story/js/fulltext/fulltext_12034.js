/***************************************************************************
@Description :seqDB-12034 :重复删除同一条记录 
@Modify list :
              2018-9-28  YinZhen  Create
****************************************************************************/
main( test );

function test ()
{

   if( commIsStandalone( db ) )
   {
      return;
   }

   var clName = COMMCLNAME + "_ES_12034";
   dropCL( db, COMMCSNAME, clName, true, true );

   //创建全文索引 
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   var fullIndex = "fullIndex_ES_12034";
   commCreateIndex( dbcl, fullIndex, { name: "text" } );

   var records = new Array();
   for( var i = 0; i < 10; i++ )
   {
      records[i] = { name: "name" + i };
   }
   dbcl.insert( records );

   var dbOperator = new DBOperator();
   var esIndexNames = dbOperator.getESIndexNames( COMMCSNAME, clName, fullIndex );
   var cappedCLs = dbOperator.getCappedCLs( COMMCSNAME, clName, fullIndex );
   checkFullSyncToES( COMMCSNAME, clName, fullIndex, 10 );

   //重复删除同一条记录，检查结果
   dbcl.remove( { name: "name1" } );
   dbcl.remove( { name: "name1" } );

   checkFullSyncToES( COMMCSNAME, clName, fullIndex, 9 );

   //重复删除时，命令行执行成功，固定集合中新增一条Type:2的记录，es中最终与原集合数据一致
   var count = cappedCLs[0].find( { Type: 2 } ).count();
   checkAllResult( count );

   var esOperator = new ESOperator();
   var findConf = { "": { $Text: { "query": { "match_all": {} } } } }
   var queryCond = '{"query" : {"exists" : {"field" : "name"}}, "size" : 20}';
   var actESRecords = esOperator.findFromES( esIndexNames[0], queryCond );
   var expCLRecords = dbOperator.findFromCL( dbcl, findConf, { name: "" }, null, null );
   checkRecords( expCLRecords, actESRecords );

   dropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}

function checkAllResult ( count )
{
   if( count != 1 )
   {
      throw new Error( "expect record num: 1, actual record num: " + count );
   }
}

function checkRecords ( expRecords, actRecords )
{
   expRecords.sort( compare( "name" ) );
   actRecords.sort( compare( "name" ) );
   checkResult( expRecords, actRecords )
}
