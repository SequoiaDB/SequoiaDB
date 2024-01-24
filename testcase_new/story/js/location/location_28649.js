/******************************************************************************
 * @Description   : seqDB-28649:data节点使用setLocation设置Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.14
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28649_1";
   var location2 = "location_28649_2";

   // 获取一个data节点
   var dataGroupName = commGetDataGroupNames( db )[0];
   var data = db.getRG( dataGroupName ).getSlave();
   var nodeName = data.getHostName() + ":" + data.getServiceName();

   // 设置location
   data.setLocation( location1 );
   checkNodeLocation( data, location1 );
   var groupVersion1 = getGroupVersion( db, dataGroupName );
   var locationID1 = getLocationID( db, dataGroupName, location1 );

   // 修改location
   data.setLocation( location2 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName } );
   checkLocationToGroup( cursor, dataGroupName, nodeName, location2 );
   var groupVersion2 = getGroupVersion( db, dataGroupName );
   var locationID2 = getLocationID( db, dataGroupName, location2 );
   compareSize( groupVersion1, groupVersion2 );
   compareSize( locationID1, locationID2 );

   // 再次设置同名location
   data.setLocation( location2 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName } );
   checkLocationToGroup( cursor, dataGroupName, nodeName, location2 );
   var groupVersion3 = getGroupVersion( db, dataGroupName );
   var locationID3 = getLocationID( db, dataGroupName, location2 );
   assert.equal( locationID2, locationID3 );
   assert.equal( groupVersion2, groupVersion3 );

   // 删除location
   data.setLocation( "" );
   var expLocation = undefined;
   checkNodeLocation( data, expLocation );
   var groupVersion4 = getGroupVersion( db, dataGroupName );
   compareSize( groupVersion3, groupVersion4 );

   // 再次删除location
   data.setLocation( "" );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName } );
   checkLocationToGroup( cursor, dataGroupName, nodeName, expLocation );
   var groupVersion5 = getGroupVersion( db, dataGroupName );
   assert.equal( groupVersion4, groupVersion5 );
}