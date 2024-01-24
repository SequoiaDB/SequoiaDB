/******************************************************************************
 * @Description   : seqDB-28653:catalog节点使用setAttributes设置Location
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.15
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location1 = "location_28653_1";
   var location2 = "location_28653_2";
   var groupName = "SYSCatalogGroup";

   // 获取一个catalog节点
   var catalog = db.getCatalogRG().getSlave();
   var nodeName = catalog.getHostName() + ":" + catalog.getServiceName();

   // 设置location为""
   catalog.setAttributes( { Location: "" } );
   var expLocation = undefined;
   checkNodeLocation( catalog, expLocation );
   var groupVersion1 = getGroupVersion( db, groupName );

   // 设置location
   catalog.setAttributes( { Location: location1 } );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location1 );
   var groupVersion2 = getGroupVersion( db, groupName );
   var locationID1 = getLocationID( db, groupName, location1 );
   compareSize( groupVersion1, groupVersion2 );

   // 修改location
   catalog.setAttributes( { Location: location2 } );
   checkNodeLocation( catalog, location2 );
   var groupVersion3 = getGroupVersion( db, groupName );
   var locationID2 = getLocationID( db, groupName, location2 );
   compareSize( groupVersion2, groupVersion3 );
   compareSize( locationID1, locationID2 );

   // 设置相同location
   catalog.setAttributes( { Location: location2 } );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   checkLocationToGroup( cursor, groupName, nodeName, location2 );
   var groupVersion4 = getGroupVersion( db, groupName );
   var locationID3 = getLocationID( db, groupName, location2 );
   assert.equal( locationID2, locationID3 );
   assert.equal( groupVersion3, groupVersion4 );

   // 删除location
   catalog.setAttributes( { Location: "" } );
   var expLocation = undefined;
   checkNodeLocation( catalog, expLocation );
   var groupVersion5 = getGroupVersion( db, groupName );
   compareSize( groupVersion4, groupVersion5 );
}