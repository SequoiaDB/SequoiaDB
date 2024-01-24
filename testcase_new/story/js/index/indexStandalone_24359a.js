/******************************************************************************
 * @Description   : seqDB-24359:切分表创建/删除本地索引(指定一个组中的节点)    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24359a";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24359a";
   var recordNum = 10000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   var expRecs = insertBulkData( dbcl, recordNum );
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );

   var masterNodeInfo = db.getRG( dstGroupName ).getMaster();
   var nodeName = masterNodeInfo["_nodename"];
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, nodeName, indexName );

   //检查索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 10, b: 10 }, "ixscan", indexName );

   //删除本地索引
   dbcl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, nodeName, indexName );
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, false );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, nodeName, { a: 10, b: 10 }, "tbscan", "" );
}

