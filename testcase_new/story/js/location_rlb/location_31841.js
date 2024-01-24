/******************************************************************************
 * @Description   : seqDB-31841:集群已经设置ActiveLocation，使用reelect重新选举
 * @Author        : tangtao
 * @CreateTime    : 2023.05.24
 * @LastEditTime  : 2023.05.24
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "guangzhou.nansha_31841";
   var location2 = "guangzhou.panyu_31841";

   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );

   // 给一个主节点和一个备节点设置相同的location,另一个备节点设置不同的location
   var rg = db.getRG( dataGroupName );
   var masterNode = rg.getMaster();
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );
   rg.setActiveLocation( location1 );

   // 主节点设置weight为ActiveLocation中最大值
   db.updateConf( { "weight": 100 }, { "NodeName": masterNode.getHostName() + ":" + masterNode.getServiceName() } );
   db.updateConf( { "weight": 90 }, { "NodeName": slaveNodes[0] } );


   // reelect指定节点进行重选举
   nodeID = slaveNode2.getDetailObj().toObj()["NodeID"];
   rg.reelect( { "NodeID": nodeID } );
   var newMasterNode = rg.getMaster();
   assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(), slaveNodes[1] );


   // reelent不指定节点进行重选举
   rg.reelect();
   var newMasterNode = rg.getMaster();
   assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(),
                 masterNode.getHostName() + ":" + masterNode.getServiceName() );


   // 设置weight最大的节点为为ActiveLocation外
   db.updateConf( { "weight": 80 }, { "NodeName": masterNode.getHostName() + ":" + masterNode.getServiceName() } );
   db.updateConf( { "weight": 90 }, { "NodeName": slaveNodes[0] } );
   db.updateConf( { "weight": 100 }, { "NodeName": slaveNodes[1] } );

   rg.reelect();
   var newMasterNode = rg.getMaster();
   assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(), slaveNodes[0] );


   // 删除节点设的weight
   db.deleteConf( { weight: "" }, { GroupName: dataGroupName } );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}