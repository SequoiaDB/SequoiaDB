/******************************************************************************
 * @Description   :  seqDB-24365 :: 创建本地索引，执行切分    
 * @Author        : wu yan
 * @CreateTime    : 2021.09.26
 * @LastEditTime  : 2021.12.17
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.clName = COMMCLNAME + "_standaloneIndex_24365";
testConf.clOpt = { ShardingKey: { no: 1 }, ShardingType: 'range', ReplSize: 0 };

main( test );
function test ()
{
   var indexName = "Index_24365";
   var recordNum = 5000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;
   var srcGroupName = testPara.srcGroupName;
   var dstGroupName = testPara.dstGroupNames[0];
   insertBulkData( dbcl, recordNum );

   var masterNodeInfo = db.getRG( srcGroupName ).getMaster();
   var nodeName = masterNodeInfo["_nodename"];
   dbcl.createIndex( indexName, { no: 1, a: 1 }, { Standalone: true }, { NodeName: nodeName } );
   testPara.testCL.split( srcGroupName, dstGroupName, 50 );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, nodeName, indexName );

   //检查索引信息
   checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName, true );
}

