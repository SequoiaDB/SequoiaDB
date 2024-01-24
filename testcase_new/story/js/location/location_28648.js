/******************************************************************************
 * @Description   : seqDB-28648:catalog节点使用setLocation设置Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.14
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28648_1";
   var location2 = "location_28648_2";
   var groupName = "SYSCatalogGroup";

   // 获取一个catalog节点
   var catalog = db.getCatalogRG().getSlave();
   var nodeName = catalog.getHostName() + ":" + catalog.getServiceName();
   // 删除location
   catalog.setLocation( "" );
   var expLocation = undefined;
   checkNodeLocation( catalog, expLocation );
   var groupVersion1 = getGroupVersion( db, groupName );

   // 删除location
   catalog.setLocation( "" );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, expLocation );
   var groupVersion2 = getGroupVersion( db, groupName );
   assert.equal( groupVersion1, groupVersion2 );

   // 设置location
   catalog.setLocation( location1 );
   checkNodeLocation( catalog, location1 );
   var groupVersion3 = getGroupVersion( db, groupName );
   var locationID1 = getLocationID( db, groupName, location1 );
   compareSize( groupVersion2, groupVersion3 );

   // 修改为同名location
   catalog.setLocation( location1 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location1 );
   var groupVersion4 = getGroupVersion( db, groupName );
   var locationID2 = getLocationID( db, groupName, location1 );
   assert.equal( locationID1, locationID2 );
   assert.equal( groupVersion3, groupVersion4 );

   // 修改location
   catalog.setLocation( location2 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location2 );
   var groupVersion5 = getGroupVersion( db, groupName );
   var locationID3 = getLocationID( db, groupName, location2 );
   compareSize( groupVersion4, groupVersion5 );
   compareSize( locationID2, locationID3 );

   // 清理location
   catalog.setLocation( "" );
}