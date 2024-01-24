/******************************************************************************
 * @Description   : seqDB-31843:主节点不在ActiveLocation重选举
 * @Author        : liuli
 * @CreateTime    : 2023.05.23
 * @LastEditTime  : 2023.05.23
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31843_1";
   var location2 = "location_31843_2";

   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var slaveNodes = getGroupSlaveNodeName( db, dataGroupName );

   // 2个备节点设置相同的location
   var rg = db.getRG( dataGroupName );
   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location1 );

   // 主节点设置不同的location
   var masterNode = rg.getMaster();
   masterNode.setLocation( location2 );

   // 一个备节点设置weight为ActiveLocation中最大值
   db.updateConf( { "weight": 90 }, { "NodeName": slaveNodes[0] } );
   // 主节点设置weight为ActiveLocation中最大值
   db.updateConf( { "weight": 100 }, { "NodeName": masterNode.getHostName() + ":" + masterNode.getServiceName() } );

   // 设置ActiveLocation为location1
   rg.setActiveLocation( location1 );

   // 检查ActiveLocation
   checkGroupActiveLocation( db, dataGroupName, location1 );

   // group 重新选举
   rg.reelect();

   // 获取新的主节点
   var newMasterNode = rg.getMaster();

   // 校验主节点是否为ActiveLocation中weight最大的节点
   assert.equal( newMasterNode.getHostName() + ":" + newMasterNode.getServiceName(), slaveNodes[0] );

   // 删除节点设的weight
   db.deleteConf( { weight: "" }, { GroupName: dataGroupName } );

   // 移除group设置的location
   masterNode.setLocation( "" );
   slaveNode1.setLocation( "" );
   slaveNode2.setLocation( "" );
}