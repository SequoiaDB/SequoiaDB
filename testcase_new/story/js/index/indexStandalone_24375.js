/******************************************************************************
 * @Description   : seqDB-24375:异步创建/删除本地索引 
 * @Author        : liuli
 * @CreateTime    : 2021.09.27
 * @LastEditTime  : 2021.09.30
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24375";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var indexName = "index_24375";
   insertBulkData( dbcl, 1000 );

   // 直连数据节点创建本地索引
   var nodeName = getCLOneNodeName( db, COMMCSNAME + "." + testConf.clName );

   // 异步创建本地索引
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.createIndexAsync( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   } );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );

   dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName } );

   // 异步删除索引
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dbcl.dropIndexAsync( indexName );
   } );

   // 校验索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );
}