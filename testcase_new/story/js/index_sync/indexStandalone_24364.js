/******************************************************************************
 * @Description   : seqDB-24364:指定全部控制参数创建本地索引
 * @Author        : liuli
 * @CreateTime    : 2021.09.29
 * @LastEditTime  : 2022.04.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.clName = COMMCLNAME + "_24364";
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;
   var indexName = "index_24364";

   // 获取CL所在的节点
   var nodes = commGetGroupNodes( db, srcGroup );
   if( nodes.length < 3 )
   {
      return;
   }
   var nodeName1 = nodes[0].HostName + ":" + nodes[0].svcname;
   var nodeName2 = nodes[1].HostName + ":" + nodes[1].svcname;
   var nodeName3 = nodes[2].HostName + ":" + nodes[2].svcname;
   var nodeID2 = nodes[1].NodeID;
   var nodeID3 = nodes[2].NodeID;
   insertBulkData( dbcl, 1000 );

   try
   {
      // 节点配置instanceid
      var instanceid1 = 11;
      var instanceid2 = 17;
      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.updateConf( { instanceid: instanceid1 }, { NodeName: nodeName1 } );
      } );

      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.updateConf( { instanceid: instanceid2 }, { NodeName: nodeName2 } );
      } );

      db.getRG( srcGroup ).getNode( nodeName1 ).stop();
      db.getRG( srcGroup ).getNode( nodeName1 ).start();
      db.getRG( srcGroup ).getNode( nodeName2 ).stop();
      db.getRG( srcGroup ).getNode( nodeName2 ).start();

      commCheckBusinessStatus( db );

      // 创建索引，指定nodeName,instanceid,nodeID分别为不同节点
      assert.tryThrow( SDB_CLS_NODE_NOT_EXIST, function()
      {
         dbcl.createIndex( indexName, { a: 1 }, { Standalone: true }, { NodeName: nodeName1, InstanceID: instanceid2, NodeID: nodeID3 } );
      } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, [nodeName1, nodeName2, nodeName3], false );

      // 创建索引，指定nodeName,instanceid,nodeID交集为nodeName2
      dbcl.createIndex( indexName, { c: 1 }, { Standalone: true }, { NodeName: [nodeName2, nodeName3], InstanceID: [instanceid1, instanceid2], NodeID: [nodeID2, nodeID3] } );
      checkStandaloneIndexOnNode( db, COMMCSNAME, testConf.clName, indexName, nodeName2, true );
   }
   finally
   {
      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.deleteConf( { instanceid: instanceid1 }, { NodeName: nodeName1 } );
      } );

      assert.tryThrow( SDB_RTN_CONF_NOT_TAKE_EFFECT, function()
      {
         db.deleteConf( { instanceid: instanceid2 }, { NodeName: nodeName2 } );
      } );
      db.getRG( srcGroup ).getNode( nodeName1 ).stop();
      db.getRG( srcGroup ).getNode( nodeName1 ).start();
      db.getRG( srcGroup ).getNode( nodeName2 ).stop();
      db.getRG( srcGroup ).getNode( nodeName2 ).start();
      commCheckBusinessStatus( db );
   }
}