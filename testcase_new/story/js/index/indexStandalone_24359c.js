/******************************************************************************
 * @Description   : seqDB-24359:切分表创建/删除本地索引（指定多个组中部分节点）   
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2022.01.29
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24359c";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24359c";
   var recordNum = 5000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   var expRecs = insertBulkData( dbcl, recordNum );
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );

   var srcNodeName = getOneNodeName( db, srcGroupName );
   var masterNodeInfo = db.getRG( dstGroupName ).getMaster();
   var dstMasterNodeName = masterNodeInfo["_nodename"];
   var slaveNodeInfo = db.getRG( dstGroupName ).getSlave();
   var dstSlaveNodeName = slaveNodeInfo["_nodename"];
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: [srcNodeName, dstMasterNodeName, dstSlaveNodeName] } );

   //检查索引信息
   checkStandaloneIndexOnNodes( db, COMMCSNAME, testConf.clName, indexName, [srcNodeName, dstMasterNodeName, dstSlaveNodeName], true );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, srcNodeName, indexName );
   checkStandaloneIndexTask( "Create index", fullclName, dstMasterNodeName, indexName );
   checkStandaloneIndexTask( "Create index", fullclName, dstSlaveNodeName, indexName );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, srcNodeName, { a: 4000, b: 4000 }, "ixscan", indexName );

   //删除本地索引
   dbcl.dropIndex( indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, srcNodeName, indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, dstMasterNodeName, indexName );
   checkStandaloneIndexTask( "Drop index", fullclName, dstSlaveNodeName, indexName );
   checkStandaloneIndexOnNodes( db, COMMCSNAME, testConf.clName, indexName, [srcNodeName, dstMasterNodeName, dstSlaveNodeName], false );
   checkExplainUseStandAloneIndex( COMMCSNAME, testConf.clName, srcNodeName, { a: 4000, b: 4000 }, "tbscan", "" );
}



