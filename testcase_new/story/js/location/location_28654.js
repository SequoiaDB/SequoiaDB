/******************************************************************************
 * @Description   : seqDB-28654:data节点使用setAttributes设置Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.15
 * @LastEditTime  : 2022.11.19
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28654_1";
   var location2 = "location_28654_2";

   // 获取一个data节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getSlave();
   var nodeName = data.getHostName() + ":" + data.getServiceName();

   // 设置location
   data.setAttributes( { Location: location1 } );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, nodeName, location1 );
   var groupVersion1 = getGroupVersion( db, dataGroupName );
   var locationID1 = getLocationID( db, dataGroupName, location1 );

   // 删除location
   data.setAttributes( { Location: "" } );
   checkLocationDeatil( db, dataGroupName, nodeName, undefined );
   var groupVersion2 = getGroupVersion( db, dataGroupName );
   compareSize( groupVersion1, groupVersion2 );

   // 再次设置location
   data.setAttributes( { Location: location2 } );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, nodeName, location2 );
   var groupVersion3 = getGroupVersion( db, dataGroupName );
   var locationID2 = getLocationID( db, dataGroupName, location2 );
   compareSize( groupVersion2, groupVersion3 );
   compareSize( locationID1, locationID2 );

   // 设置相同location
   data.setAttributes( { Location: location2 } );
   checkLocationDeatil( db, dataGroupName, nodeName, location2 );
   var groupVersion4 = getGroupVersion( db, dataGroupName );
   var locationID3 = getLocationID( db, dataGroupName, location2 );
   assert.equal( locationID2, locationID3 );
   assert.equal( groupVersion3, groupVersion4 );

   // 删除location
   data.setAttributes( { Location: "" } );
   var cursor = db.listReplicaGroups();
   checkLocationToGroup( cursor, dataGroupName, nodeName, undefined );
   var groupVersion5 = getGroupVersion( db, dataGroupName );
   compareSize( groupVersion4, groupVersion5 );
}