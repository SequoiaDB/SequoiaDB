/******************************************************************************
 * @Description   : seqDB-28647:coord节点设置Location
 * @Author        : liuli
 * @CreateTime    : 2022.11.11
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28647_1";
   var location2 = "location_28647_2";
   var groupName = "SYSCoord";
   // 获取一个coord节点
   var coord = db.getCoordRG().getSlave();
   var nodeName = coord.getHostName() + ":" + coord.getServiceName();

   // 设置location
   coord.setLocation( location1 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location1 );
   var locationID1 = getLocationID( db, groupName, location1 );
   var groupVersion1 = getGroupVersion( db, groupName );

   // 再次设置为相同location
   coord.setLocation( location1 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location1 );
   var locationID2 = getLocationID( db, groupName, location1 );
   var groupVersion2 = getGroupVersion( db, groupName );
   // 校验locationID和groupVersion不变
   assert.equal( locationID1, locationID2 );
   assert.equal( groupVersion1, groupVersion2 );

   // 修改节点的location
   coord.setLocation( location2 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location2 );
   var locationID3 = getLocationID( db, groupName, location2 );
   var groupVersion3 = getGroupVersion( db, groupName );
   // 校验locationID和groupVersion增加
   compareSize( locationID2, locationID3 );
   compareSize( groupVersion2, groupVersion3 );

   // 删除节点location
   coord.setLocation( "" );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, undefined );
   var groupVersion4 = getGroupVersion( db, groupName );
   // 校验locationID和groupVersion增加
   compareSize( groupVersion3, groupVersion4 );

   // 再次删除节点location
   coord.setLocation( "" );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, undefined );
   var groupVersion5 = getGroupVersion( db, groupName );
   // 校验locationID和groupVersion增加
   assert.equal( groupVersion4, groupVersion5 );
}
