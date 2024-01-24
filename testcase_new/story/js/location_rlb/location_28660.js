/******************************************************************************
 * @Description   : seqDB-28660:catalog节点设置Location后，分离节点
 * @Author        : HuangHaimei
 * @CreateTime    : 2022.11.16
 * @LastEditTime  : 2022.11.19
 * @LastEditors   : HuangHaimei
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var location = "location_28660";
   var groupName = "SYSCatalogGroup";
   var groupsArray = commGetGroups( db );
   var hostName = groupsArray[0][1].HostName;
   var port1 = parseInt( RSRVPORTBEGIN ) + 10;
   var port2 = parseInt( RSRVPORTBEGIN ) + 20;
   var dbpath1 = RSRVNODEDIR + "cata/" + port1;
   var dbpath2 = RSRVNODEDIR + "cata/" + port2;

   try
   {
      var catalogRG = db.getCatalogRG();
      var catalog1 = catalogRG.createNode( hostName, port1, dbpath1, { diaglevel: 5 } );
      var catalog2 = catalogRG.createNode( hostName, port2, dbpath2, { diaglevel: 5 } );
      catalogRG.start();

      var nodeName1 = catalog1.getHostName() + ":" + catalog1.getServiceName();
      var nodeName2 = catalog2.getHostName() + ":" + catalog2.getServiceName();

      // 设置备节点1的location
      catalog1.setLocation( location );
      var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
      checkLocationToGroup( cursor, groupName, nodeName1, location );
      var groupVersion1 = getGroupVersion( db, groupName );
      var locationID1 = getLocationID( db, groupName, location );

      // 分离备节点1
      catalogRG.detachNode( hostName, port1, { KeepData: false } );
      var groupVersion2 = getGroupVersion( db, groupName );
      compareSize( groupVersion1, groupVersion2 );

      // 设置备节点2为同名的location
      catalog2.setLocation( location );
      var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
      checkLocationToGroup( cursor, groupName, nodeName2, location );
      var groupVersion3 = getGroupVersion( db, groupName );
      var locationID2 = getLocationID( db, groupName, location );
      compareSize( groupVersion2, groupVersion3 );
      compareSize( locationID1, locationID2 );

      // 分离备节点2
      catalogRG.detachNode( hostName, port2, { KeepData: false } );
      var groupVersion4 = getGroupVersion( db, groupName );
      compareSize( groupVersion3, groupVersion4 );
   }
   finally
   {
      attachNode( catalogRG, hostName, port1, { KeepData: false } );
      attachNode( catalogRG, hostName, port2, { KeepData: false } );
      removeNode( catalogRG, hostName, port1 );
      removeNode( catalogRG, hostName, port2 );
   }
}