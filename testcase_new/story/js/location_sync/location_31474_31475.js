/******************************************************************************
 * @Description   : seqDB-31474:单节点设置location后，查看IsLocationPrimary值
 *                  seqDB-31475:单节点修改location后，查看IsLocationPrimary值
 * @Author        : HuangHaimei
 * @CreateTime    : 2023.05.06
 * @LastEditTime  : 2023.10.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_31474_31475_1";
   var location2 = "location_31474_31475_2";
   // 获取一个data节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getSlave();
   var nodeName = data.getHostName() + ":" + data.getServiceName();
   data.setLocation( "" );

   // 设置location
   data.setLocation( location1 );

   // 获取位置集主节点的NodeName
   var locationPrimary1 = checkAndGetLocationHasPrimary( db, dataGroupName, location1, 30 );
   assert.equal( nodeName, locationPrimary1 );

   // 检查数据库快照位置集主节点的IsLocationPrimary值
   var values1 = getSnapshotDatabase( db, locationPrimary1 );
   assert.equal( values1[0]["Location"], location1 );
   assert.equal( values1[0]["IsLocationPrimary"], true );

   // 检查分区列表中位置集主节点的NodeID与数据库快照中的是否一致
   var noedID = getPrimaryNode( db, dataGroupName );
   assert.equal( values1[0]["NodeID"], noedID );

   // 修改location的值后,位置集主节点不变，IsLocationPrimary的值不变
   data.setLocation( location2 );
   checkAndGetLocationHasPrimary( db, dataGroupName, location2, 30 )
   var values2 = getSnapshotDatabase( db, locationPrimary1 );
   assert.equal( values2[0]["IsLocationPrimary"], true );

   data.setLocation( "" );
}