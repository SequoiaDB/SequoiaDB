/******************************************************************************
 * @Description   : seqDB-24370:指定不同节点创建相同本地索引   
 * @Author        : wu yan
 * @CreateTime    : 2021.09.23
 * @LastEditTime  : 2022.01.22
 * @LastEditors   : Wu Yan
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneDuplicatePerGroup = true
testConf.clName = COMMCLNAME + "_standaloneIndex_24370";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ( testPara )
{
   var indexName = "Index_24370";
   var recordNum = 1000;
   var dbcl = testPara.testCL;
   var fullclName = COMMCSNAME + "." + testConf.clName;

   var nodes = commGetCLNodes( db, fullclName );
   if( nodes.length < 2 )
   {
      return;
   }
   var nodeName = nodes[0].HostName + ":" + nodes[0].svcname;
   var nodeName1 = nodes[1].HostName + ":" + nodes[1].svcname;
   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName } );
   insertBulkData( dbcl, recordNum );

   dbcl.createIndex( indexName, { a: 1, b: 1 }, { Standalone: true }, { NodeName: nodeName1 } );

   //检查任务信息    
   checkStandaloneIndexTask( "Create index", fullclName, nodeName, indexName );
   checkStandaloneIndexTask( "Create index", fullclName, nodeName1, indexName );

   //检查索引信息
   checkStandaloneIndexOnNodes( db, COMMCSNAME, testConf.clName, indexName, [nodeName, nodeName1], true );

   //删除本地索引
   dbcl.dropIndex( indexName );
   commCheckIndexConsistent( db, COMMCSNAME, testConf.clName, indexName, false );
}

