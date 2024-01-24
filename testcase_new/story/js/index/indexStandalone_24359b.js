/******************************************************************************
 * @Description   : seqDB-24359:切分表创建/删除本地索引（指定多个组中1个节点）   
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2021.12.18
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24359b";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24359b";
   var recordNum = 10000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   insertBulkData( dbcl, recordNum );
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );

   var srcNodeInfo = db.getRG( srcGroupName ).getMaster();
   var srcNodeName = srcNodeInfo["_nodename"];
   var srcNodeID = srcNodeInfo["_nodeid"];
   var dstNodeInfo = db.getRG( dstGroupName ).getSlave();
   var dstNodeName = dstNodeInfo["_nodename"];
   var dstNodeID = dstNodeInfo["_nodeid"];
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeID: [srcNodeID, dstNodeID] } );
   //检查索引信息
   checkStandaloneIndexOnNodes( db, COMMCSNAME, testConf.clName, indexName, [srcNodeName, dstNodeName], true );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, srcNodeName, indexName );
   checkStandaloneIndexTask( "Create index", fullclName, dstNodeName, indexName );

   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, srcNodeName, { a: 10, b: 10 }, "ixscan", indexName );

   //删除本地索引
   dbcl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, srcNodeName, indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, dstNodeName, indexName );
   checkStandaloneIndexOnNodes( db, COMMCSNAME, testConf.clName, indexName, [srcNodeName, dstNodeName], false );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, srcNodeName, { a: 10, b: 10 }, "tbscan", "" );
}



