/******************************************************************************
 * @Description   : seqDB-28651:不同节点属于不同group，设置Location相同
 * @Author        : liuli
 * @CreateTime    : 2022.11.14
 * @LastEditTime  : 2022.11.14
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28651_1";
   var location2 = "location_28651_2";

   // 获取一个data备节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getSlave();
   var dataNodeName = data.getHostName() + ":" + data.getServiceName();

   // 获取一个cata主节点
   var cataGroupName = "SYSCatalogGroup";
   var cata = db.getCataRG().getMaster();
   var cataNodeName = cata.getHostName() + ":" + cata.getServiceName();

   // 设置data节点location
   data.setLocation( location1 );
   checkNodeLocation( data, location1 );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, dataNodeName, location1 );
   var locationID1 = getLocationID( db, dataGroupName, location1 );

   // 设置cata节点location
   cata.setLocation( location1 );
   checkNodeLocation( cata, location1 );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, cataGroupName, cataNodeName, location1 );
   var locationID2 = getLocationID( db, cataGroupName, location1 );
   assert.equal( locationID1, locationID2 );

   // 修改data节点location
   data.setLocation( location2 );
   checkNodeLocation( data, location2 );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, dataNodeName, location2 );
   var locationID3 = getLocationID( db, dataGroupName, location2 );

   // 修改cata节点location
   cata.setLocation( location2 );
   checkNodeLocation( cata, location2 );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, cataGroupName, cataNodeName, location2 );
   var locationID4 = getLocationID( db, cataGroupName, location2 );
   assert.equal( locationID3, locationID4 );

   // 删除data节点location
   data.setLocation( "" );
   checkNodeLocation( data, undefined );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, dataNodeName, undefined );

   // 删除cata节点location
   cata.setLocation( "" );
   checkNodeLocation( cata, undefined );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, cataGroupName, cataNodeName, undefined );
}
