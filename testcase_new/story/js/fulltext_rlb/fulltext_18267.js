/************************************
*@Description: 正常停止所有编目节点并重启所有节点，创建全文索引
*@author:      liuxiaoxuan
*@createdate:  2019.05.08
*@testlinkCase: seqDB-18267
**************************************/

main( test );

function test ()
{
   if( commIsStandalone( db ) ) { return; }

   var clName = COMMCLNAME + "_ES_18267";
   commDropCL( db, COMMCSNAME, clName, true, true );

   var dbcl = commCreateCL( db, COMMCSNAME, clName );

   var objs = new Array();
   for( var i = 0; i < 20000; i++ )
   {
      objs.push( { a: "test_18267 " + i, b: i } );
   }
   dbcl.insert( objs );

   // 重启所有节点
   var node1 = db.getRG( "SYSCatalogGroup" ).getSlave( 1 );
   var node2 = db.getRG( "SYSCatalogGroup" ).getSlave( 2 );
   var node3 = db.getRG( "SYSCatalogGroup" ).getSlave( 3 );

   try
   {
      node1.stop();
      node2.stop();
      node3.stop();
   }
   finally
   {
      node1.start();
      node2.start();
      node3.start();
   }

   // 检查主备节点LSN是否一致
   checkCatalogBusiness( 120 );

   // 创建全文索引，检查数据同步
   var textIndexName = "textIndex_18267";
   dbcl.createIndex( textIndexName, { "a": "text" } );
   checkFullSyncToES( COMMCSNAME, clName, textIndexName, 20000 );

   // 全文检索
   var findConf = { "$not": [{ "b": { "$gte": 10000 } }, { "": { "$Text": { "query": { "match": { "a": "test_18267" } } } } }] };
   var actResult = dbOpr.findFromCL( dbcl, findConf, { 'a': '' } );
   var expResult = dbOpr.findFromCL( dbcl, { "b": { "$lt": 10000 } }, { 'a': '' } );
   actResult.sort( compare( "a" ) );
   expResult.sort( compare( "a" ) );
   checkResult( expResult, actResult );

   var esIndexNames = dbOpr.getESIndexNames( COMMCSNAME, clName, textIndexName );
   commDropCL( db, COMMCSNAME, clName, true, true );
   //SEQUOIADBMAINSTREAM-3983
   checkIndexNotExistInES( esIndexNames );
}
