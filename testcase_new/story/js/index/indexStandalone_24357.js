/******************************************************************************
 * @Description   : seqDB-24357:分区表创建/删除本地索引(指定nodeID)   
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24357";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24357";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var nodes = commGetCLNodes( db, fullclName );
   //从cl所在节点任意取一个节点创建/删除本地索引
   var serialNum = Math.floor( Math.random() * nodes.length );
   var nodeID = nodes[serialNum].NodeID;
   var nodeName = nodes[serialNum].HostName + ":" + nodes[serialNum].svcname;
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeID: nodeID } );
   insertBulkData( dbcl, recordNum );

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

