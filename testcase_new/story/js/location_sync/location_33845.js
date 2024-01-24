/******************************************************************************
 * @Description   : seqDB-33845:整组重启后主节点在ActiveLocation中
 * @Author        : liuli
 * @CreateTime    : 2023.10.16
 * @LastEditTime  : 2023.10.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var srcGroupName = commGetDataGroupNames( db )[0];
   var slaveNodeNames = getGroupSlaveNodeName( db, srcGroupName );
   var location1 = "location_33845_1";
   var location2 = "location_33845_2";

   // 主节点和一个备节点设置Location为location1
   var rg = db.getRG( srcGroupName );
   var master = rg.getMaster();
   master.setLocation( location1 );

   // 备节点设置Location为location2
   for( var i in slaveNodeNames )
   {
      rg.getNode( slaveNodeNames[i] ).setLocation( location2 );
   }

   var slave = rg.getSlave();
   slave.setLocation( location1 );

   // 设置location1为ActiveLocation
   rg.setActiveLocation( location1 );

   var primaryLocationNodeNames = [];
   primaryLocationNodeNames.push( master.getHostName() + ":" + master.getServiceName() );
   primaryLocationNodeNames.push( slave.getHostName() + ":" + slave.getServiceName() );

   // 重启所有节点
   rg.stop();
   rg.start();
   commCheckBusinessStatus( db );

   // 获取新的主节点
   var newMaster = rg.getMaster();

   // 新的主节点在location1
   var newMasterNodeName = newMaster.getHostName() + ":" + newMaster.getServiceName();

   if( primaryLocationNodeNames.indexOf( newMasterNodeName ) == -1 )
   {
      throw new Error( "new master node is not in the primary location; newMaster : " + newMasterNodeName
         + ",primaryLocationNodeNames : " + primaryLocationNodeNames );
   }

   // 清除location
   rg.getMaster().setLocation( "" );
   for( var i in slaveNodeNames )
   {
      rg.getNode( slaveNodeNames[i] ).setLocation( "" );
   }
}
