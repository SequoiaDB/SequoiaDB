/******************************************************************************
 * @Description   : seqDB-28650:不同节点属于相同group，设置Location相同
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.14
 * @LastEditTime  : 2022.11.18
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;

main( test );
function test ()
{
   var location1 = "location_28650_1";
   var location2 = "location_28650_2";

   // 获取data节点1
   var dataGroupName1 = commGetDataGroupNames( db )[0];
   var data1 = db.getRG( dataGroupName1 ).getSlave();
   // 获取data节点2
   var dataGroupName2 = commGetDataGroupNames( db )[1];
   var data2 = db.getRG( dataGroupName2 ).getSlave();
   var dataNodeName1 = data1.getHostName() + ":" + data1.getServiceName();
   var dataNodeName2 = data2.getHostName() + ":" + data2.getServiceName();

   // 设置节点1的location
   data1.setLocation( location1 );
   checkLocationDeatil( db, dataGroupName1, dataNodeName1, location1 );
   var groupVersion1 = getGroupVersion( db, dataGroupName1 );
   var locationID1 = getLocationID( db, dataGroupName1, location1 );

   // 设置节点2的location
   data2.setLocation( location1 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName2 } );
   checkLocationToGroup( cursor, dataGroupName2, dataNodeName2, location1 );
   var groupVersion3 = getGroupVersion( db, dataGroupName2 );
   var locationID2 = getLocationID( db, dataGroupName2, location1 );
   assert.equal( locationID1, locationID2 );

   // 再次设置节点1的location
   data1.setLocation( location2 );
   checkLocationDeatil( db, dataGroupName1, dataNodeName1, location2 );
   var groupVersion4 = getGroupVersion( db, dataGroupName1 );
   var locationID3 = getLocationID( db, dataGroupName1, location2 );
   compareSize( groupVersion1, groupVersion4 );
   compareSize( locationID1, locationID3 );

   // 再次设置节点2的location
   data2.setLocation( location2 );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName2 } );
   checkLocationToGroup( cursor, dataGroupName2, dataNodeName2, location2 );
   var groupVersion5 = getGroupVersion( db, dataGroupName2 );
   var locationID4 = getLocationID( db, dataGroupName2, location2 );
   compareSize( groupVersion3, groupVersion5 );
   compareSize( locationID2, locationID4 );

   // 删除节点1的location
   data1.setLocation( "" );
   checkLocationDeatil( db, dataGroupName1, dataNodeName1, undefined );
   var groupVersion6 = getGroupVersion( db, dataGroupName1 );
   compareSize( groupVersion4, groupVersion6 );

   // 删除节点2的location
   data2.setLocation( "" );
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: dataGroupName2 } );
   checkLocationToGroup( cursor, dataGroupName2, dataNodeName2, undefined );
   var groupVersion7 = getGroupVersion( db, dataGroupName2 );
   compareSize( groupVersion5, groupVersion7 );
}