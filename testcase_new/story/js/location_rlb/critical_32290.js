/******************************************************************************
 * @Description   : seqDB-32290:集群指定节点开启critical模式，组内重选举
 * @Author        : tangtao
 * @CreateTime    : 2023.07.04
 * @LastEditTime  : 2023.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_32290";

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;

   // 获取group中的主备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );

   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   var salveNodeID1 = slaveNode1.getNodeDetail().split( ":" )[0];
   var salveNodeID2 = slaveNode2.getNodeDetail().split( ":" )[0];

   // 备节点启动Critical模式并检查Critical模式
   var options = { NodeName: slaveNodes[0], MinKeepTime: 5, MaxKeepTime: 15 };
   rg.startCriticalMode( options );
   var properties1 = { NodeName: slaveNodes[0] };
   checkStartCriticalMode( db, srcGroup, properties1 );

   // 重选举并校验主节点
   for( var i = 0; i < 3; i++ )
   {
      rg.reelect();
      var newMasterNode = rg.getMaster();
      var nodeID = newMasterNode.getNodeDetail().split( ":" )[0];
      assert.equal( nodeID, salveNodeID1 );

      sleep( 1000 );
   }

   // 另一个备节点启动Critical模式并检查Critical模式
   var options = { NodeName: slaveNodes[1], MinKeepTime: 5, MaxKeepTime: 15 };
   rg.startCriticalMode( options );
   var properties2 = { NodeName: slaveNodes[1] };
   checkStartCriticalMode( db, srcGroup, properties2 );

   // 重选举并校验主节点
   for( var i = 0; i < 3; i++ )
   {
      rg.reelect();
      var newMasterNode = rg.getMaster();
      var nodeID = newMasterNode.getNodeDetail().split( ":" )[0];
      assert.equal( nodeID, salveNodeID2 );
      sleep( 1000 );
   }

   // 恢复环境
   rg.stopCriticalMode();
   commCheckBusinessStatus( db );
}