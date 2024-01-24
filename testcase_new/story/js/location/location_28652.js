/******************************************************************************
 * @Description   : seqDB-28652:coord节点使用setAttributes设置Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.15
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28652_1";
   var location2 = "location_28652_2";
   var groupName = "SYSCoord";

   // 获取一个coord节点
   var coord = db.getCoordRG().getSlave();
   var nodeName = coord.getHostName() + ":" + coord.getServiceName();

   // 设置location
   coord.setAttributes( { Location: location1 } );
   checkLocationDeatil( db, groupName, nodeName, location1 );
   var groupVersion1 = getGroupVersion( db, groupName );
   var locationID1 = getLocationID( db, groupName, location1 );

   // 再次设置相同location
   coord.setAttributes( { Location: location1 } );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, groupName, nodeName, location1 );
   var locationID2 = getLocationID( db, groupName, location1 );
   var groupVersion2 = getGroupVersion( db, groupName );
   assert.equal( locationID1, locationID2 );
   assert.equal( groupVersion1, groupVersion2 );

   // 修改节点的location
   coord.setAttributes( { Location: location2 } );
   checkLocationDeatil( db, groupName, nodeName, location2 );
   var groupVersion3 = getGroupVersion( db, groupName );
   var locationID3 = getLocationID( db, groupName, location2 );
   compareSize( locationID2, locationID3 );
   compareSize( groupVersion2, groupVersion3 );

   // 删除节点的location
   coord.setAttributes( { Location: "" } );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, groupName, nodeName, undefined );
   var groupVersion4 = getGroupVersion( db, groupName );
   compareSize( groupVersion3, groupVersion4 );

   // 再次删除节点的location
   coord.setAttributes( { Location: "" } );
   checkLocationDeatil( db, groupName, nodeName, undefined );
   var groupVersion5 = getGroupVersion( db, groupName );
   assert.equal( groupVersion4, groupVersion5 );
}