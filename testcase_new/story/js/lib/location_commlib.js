import( "../lib/main.js" );

/******************************************************************************
 * @description: 获取复制组的详细信息进行校验
 * @param {string} groupName  // 复制组名
 * @param {string} nodeName  // 节点名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function checkLocationDeatil ( db, groupName, nodeName, location )
{
   var rg = db.getRG( groupName );
   var obj = rg.getDetailObj().toObj();
   var locations = obj.Locations;
   var groupInfo = obj.Group;

   var actLocations = [];
   for( var i in locations )
   {
      var actLocation = locations[i]["Location"];
      actLocations.push( actLocation );
   }

   if( location != undefined )
   {
      // 当传入的location不在actLacation中时，报错
      if( actLocations.indexOf( location ) == -1 )
      {
         throw new Error( "expect location : " + location + " ,actual locations : " + actLocations );
      }
   }

   var checkNode = false;
   if( nodeName != undefined )
   {
      for( var i in groupInfo )
      {
         var hostName = groupInfo[i]["HostName"];
         var services = groupInfo[i]["Service"];
         for( var j in services )
         {
            if( services[j]["Type"] == 0 )
            {
               var serviceName = services[j]["Name"];
            }
         }
         var actNodeName = hostName + ":" + serviceName;
         if( actNodeName == nodeName )
         {
            var nodeLocation = groupInfo[i]["Location"];
            assert.equal( nodeLocation, location );
            var checkNode = true;
         }
      }
      // 当遍历完所有group没有得到需要匹配的节点，报错
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}

/******************************************************************************
 * @description: 获取复制组节点的详细信息进行校验
 * @param {string} node  // 节点名
 * @param {string} expLocation  // 需要校验的location
 ******************************************************************************/
function checkNodeLocation ( node, expLocation )
{
   var nodeObj = node.getDetailObj().toObj();
   actLocation = nodeObj.Location;
   assert.equal( actLocation, expLocation );
}

/******************************************************************************
 * @description: 获取复制组信息进行校验
 * @param {SdbCursor} cursor  
 * @param {string} groupName  // 复制组名
 * @param {string} nodeName  // 节点名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function checkLocationToGroup ( cursor, groupName, nodeName, location )
{
   while( cursor.next() )
   {
      // 游标遍历复制组
      var actGroupName = cursor.current().toObj().GroupName;
      // 当游标中的复制组与校验复制组相等时，获取出locations内容与Group内容
      if( actGroupName == groupName )
      {
         var locations = cursor.current().toObj().Locations;
         var groupInfo = cursor.current().toObj().Group;
      }
   }
   cursor.close();

   var actLocations = [];
   for( var i in locations )
   {
      // 获取Locations字段中的location存入actLocations中
      var actLocation = locations[i]["Location"];
      actLocations.push( actLocation );
   }

   if( location != undefined )
   {
      // 当传入的location不在actLacation中时，报错
      if( actLocations.indexOf( location ) == -1 )
      {
         throw new Error( "expect location : " + location + " ,actual locations : " + actLocations );
      }
   }

   var checkNode = false;
   if( nodeName != undefined )
   {
      for( var i in groupInfo )
      {
         var hostName = groupInfo[i]["HostName"];
         var services = groupInfo[i]["Service"];
         for( var j in services )
         {
            if( services[j]["Type"] == 0 )
            {
               var serviceName = services[j]["Name"];
            }
         }
         var actNodeName = hostName + ":" + serviceName;
         // 判断传入的节点与需要校验的节点是否相等
         if( actNodeName == nodeName )
         {
            var nodeLocation = groupInfo[i]["Location"];
            assert.equal( nodeLocation, location );
            var checkNode = true;
         }
      }
      // 当遍历完所有group没有得到需要匹配的节点，报错
      assert.equal( checkNode, true, nodeName + " docs node exist, group info " + JSON.stringify( groupInfo ) );
   }
}

/******************************************************************************
 * @description: 获取复制组中locations中的locationID
 * @param {string} groupName  // 复制组名
 * @param {string} location  // 需要校验的location
 ******************************************************************************/
function getLocationID ( db, groupName, location )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName }, { Locations: "" } );
   var locationID = -1;
   while( cursor.next() )
   {
      var locations = cursor.current().toObj().Locations;
      for( var i in locations )
      {
         var actLocation = locations[i]["Location"];
         if( actLocation == location )
         {
            var locationID = locations[i]["LocationID"];
            break;
         }
      }
      if( locationID != -1 )
      {
         break;
      }
   }
   cursor.close();
   assert.notEqual( locationID, -1, "location does not exist" );
   return locationID;
}

/******************************************************************************
 * @description: 获取复制组中的version字段
 * @param {string} groupName  // 复制组名
 ******************************************************************************/
function getGroupVersion ( db, groupName )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName }, { Version: "" } );
   while( cursor.next() )
   {
      var version = cursor.current().toObj().Version;
   }
   cursor.close();
   return version;
}

function compareSize ( minSize, maxSize )
{
   if( minSize >= maxSize )
   {
      throw new Error( "minSize:" + minSize + ", maxSize:" + minSize );
   }
}

/******************************************************************************
 * @description: 移除节点
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 ******************************************************************************/
function removeNode ( rg, hostName, port )
{
   try
   {
      rg.removeNode( hostName, port );
   }
   catch( e )
   {
      if( e != SDB_CLS_NODE_NOT_EXIST )
      {
         throw e;
      }
   }
}

/******************************************************************************
 * @description: 将节点加入当前复制组
 * @param {string} hostName  // 机器的主机名
 * @param {string} port  // 节点的端口号
 * @param {json} option  // 设置是否保留新加节点原有的数据
 ******************************************************************************/
function attachNode ( rg, hostName, port, option )
{
   try
   {
      rg.attachNode( hostName, port, option );
   }
   catch( e )
   {
      if( e != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
}

/************************************************************************
*@Description: 返回指定group中的备节点名
*@input: db            
         groups     指定的group
**************************************************************************/
function getGroupSlaveNodeName ( db, groups )
{
   if( typeof ( groups ) == "string" ) { groups = [groups]; }
   var slaveNodeName = [];
   for( var i in groups )
   {
      var cursor = db.list( SDB_LIST_GROUPS, { "GroupName": groups[i] } );
      while( cursor.next() )
      {
         var groupInfo = cursor.current().toObj();
         var primaryNode = groupInfo.PrimaryNode;
         var nodeInfos = groupInfo.Group;
         for( var i in nodeInfos )
         {
            if( nodeInfos[i].NodeID != primaryNode )
            {
               var service = nodeInfos[i].Service;
               for( var j in service )
               {
                  if( service[j].Type == 0 )
                  {
                     svcname = service[j].Name;
                  }
                  break;
               }
               slaveNodeName.push( nodeInfos[i].HostName + ":" + svcname );
            }
         }
      }
      cursor.close();
   }
   return slaveNodeName;
}

/************************************************************************
*@Description: 返回指定group中的主节点名
*@input: db            
         groups     指定的group
**************************************************************************/
function getGroupMasterNodeName ( db, groups )
{
   if( typeof ( groups ) == "string" ) { groups = [groups]; }
   var masterNodeName = [];
   for( var i in groups )
   {
      var cursor = db.list( SDB_LIST_GROUPS, { "GroupName": groups[i] } );
      while( cursor.next() )
      {
         var groupInfo = cursor.current().toObj();
         var primaryNode = groupInfo.PrimaryNode;
         var nodeInfos = groupInfo.Group;
         for( var i in nodeInfos )
         {
            if( nodeInfos[i].NodeID == primaryNode )
            {
               var service = nodeInfos[i].Service;
               for( var j in service )
               {
                  if( service[j].Type == 0 )
                  {
                     svcname = service[j].Name;
                  }
                  break;
               }
               masterNodeName.push( nodeInfos[i].HostName + ":" + svcname );
            }
         }
      }
      cursor.close();
   }
   return masterNodeName;
}

/************************************************************************
*@Description: 校验复制组中ActiveLocation字段
*@input: db            
         groupName  指定的group
         location   指定的location
**************************************************************************/
function checkGroupActiveLocation ( db, groupName, location )
{
   var cursor = db.list( SDB_LIST_GROUPS, { "GroupName": groupName } );
   while( cursor.next() )
   {
      var groupInfo = cursor.current().toObj();
      var activeLocation = groupInfo.ActiveLocation;
      assert.equal( activeLocation, location );
   }
   cursor.close();
}

/************************************************************************
*@Description: 强杀指定节点，并将节点stop
*@input: node      指定的节点
**************************************************************************/
function killNode ( db, node )
{
   var timeout = 60;
   var nodeName = node.getHostName() + ":" + node.getServiceName();
   var remote = new Remote( node.getHostName(), CMSVCNAME );
   var cmd = remote.getCmd();
   try
   {
      println( "remote -- " + remote );
      cmd.run( "ps -ef | grep sequoiadb | grep -v grep | grep " + node.getServiceName() + " | awk '{print $2}' | xargs kill -9" );
   }
   catch( e )
   {
      //忽略异常
   }
   waitNodeStart( db, nodeName, timeout )
   node.stop();
   println( "node stop -- " + node );
   remote.close();
}

/************************************************************************
*@Description: 等待节点启动
*@input: nodeName      指定的节点名称
         timeout       超时时间
**************************************************************************/
function waitNodeStart ( db, nodeName, timeout )
{
   var doTime = 0;
   while( doTime < timeout )
   {
      var sdbsnapshotOption = new SdbSnapshotOption().cond( { NodeName: nodeName } ).options( { ShowError: "only" } );
      var cursor = db.snapshot( SDB_SNAP_DATABASE, sdbsnapshotOption );
      if( cursor.next() )
      {
         cursor.close();
         doTime++;
         sleep( 1000 );
      } else
      {
         cursor.close();
         break;
      }
   }

   if( doTime >= timeout )
   {
      throw new Error( "waitNodeStart timeout" );
   }
}

/************************************************************************
*@Description: 校验启动Critical模式
*@input: groupName      指定的group
         properties     校验的properties信息
**************************************************************************/
function checkStartCriticalMode ( db, groupName, properties )
{
   var groupMode = "critical";

   // 只校验NodeName和Location字段
   delete properties.MinKeepTime;
   delete properties.MaxKeepTime;
   delete properties.NodeID;
   delete properties.LocationID;

   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   while( cursor.next() )
   {
      var groupModeInfo = cursor.current().toObj();
      var actGroupMode = groupModeInfo["GroupMode"];
      var actProperties = groupModeInfo["Properties"][0];
      delete actProperties.MinKeepTime;
      delete actProperties.MaxKeepTime;
      delete actProperties.UpdateTime;
      delete actProperties.NodeID;
      delete actProperties.LocationID;

      assert.equal( actGroupMode, groupMode );
      assert.equal( actProperties, properties );
   }
   cursor.close();
}

/************************************************************************
*@Description: 校验启动Maintenance模式
*@input: groupName      指定的group
         properties     校验的节点信息
**************************************************************************/
function checkGroupNodeInMaintenanceMode ( db, groupName, nodeNames )
{
   var groupMode = "maintenance";
   var actNodeName = [];
   var masterNode = db.getRG( groupName ).getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   var groupModeInfo = cursor.next().toObj();
   if( groupModeInfo == null )
   {
      assert.faild( "groupMode is not exist" );
   }

   var actGroupMode = groupModeInfo["GroupMode"];
   var actProperties = groupModeInfo["Properties"];
   for( var i in actProperties )
   {
      if( actProperties[i]["NodeName"] != masterNodeName )
      {
         actNodeName.push( actProperties[i]["NodeName"] );
      }
   }

   assert.equal( actGroupMode, groupMode );
   assert.equal( actNodeName.sort(), nodeNames.sort() );

   cursor.close();
}

/************************************************************************
*@Description: 校验停止Critical模式
*@input: groupName      指定的group
**************************************************************************/
function checkStopCriticalMode ( db, groupName )
{
   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   while( cursor.next() )
   {
      var groupModeInfo = cursor.current().toObj();
      var actGroupMode = groupModeInfo["GroupMode"];
      var actProperties = groupModeInfo["Properties"];

      assert.equal( actGroupMode, undefined );
      assert.equal( actProperties, [] );
   }
   cursor.close();
}

/************************************
*@Description: bulk insert data
*@author:      wuyan
*@createDate:  2021.04.02
**************************************/
function insertBulkData ( dbcl, recordNum, recordStart, recordEnd )
{
   if( undefined == recordStart ) { recordStart = 0; }
   if( undefined == recordEnd ) { recordEnd = recordNum; }
   try
   {
      var doc = [];
      for( var i = 0; i < recordNum; i++ )
      {
         var bValue = recordStart + parseInt( Math.random() * ( recordEnd - recordStart ) );
         doc.push( { a: i, b: bValue, c: i, no: i } );
      }
      dbcl.insert( doc );
      println( "---bulk insert data success" );
   }
   catch( e )
   {
      throw buildException( "insertBulkData()", e, "insert", "insert data :" + JSON.stringify( doc ), "insert fail" );
   }
   return doc;
}

/************************************************************************
*@Description: 从beginTime开始等待waitTime分钟
*@input: beginTime      开始时间
         waitTime       等待时间，单位为分钟
**************************************************************************/
function validateWaitTime ( beginTime, waitTime )
{
   // 获取当前时间
   var currentTime = new Date();
   println( "当前时间 -- " + currentTime )
   println( "beginTime -- " + beginTime )
   // 检查 beginTime 是否大于当前时间
   if( beginTime > currentTime )
   {
      throw new Error( "开始时间大于当前时间" );
   }

   // 计算等待时间的结束时间，将分钟转换为毫秒
   var endTime = new Date( beginTime.getTime() + waitTime * 60000 );

   // 等待时间循环检查
   while( true )
   {
      currentTime = new Date(); // 更新当前时间

      // 检查当前时间是否超过等待时间
      if( currentTime >= endTime )
      {
         println( "currentTime -- " + currentTime )
         return;
      }

      sleep( 1000 );
   }
}
/******************************************************************************
 * @description: 设置节点的location
 * @param {array} nodeList
 * @param {string} location
 ******************************************************************************/
function setLocationForNodes ( rg, nodeList, location )
{
   for( var i in nodeList )
   {
      var nodeInfo = nodeList[i];
      var node = rg.getNode( nodeInfo.HostName, nodeInfo.svcname );
      node.setLocation( location );
   }
}

/******************************************************************************
 * @description: 检查该位置集是否有主节点
 * @param {string} groupName
 * @param {string} location
 * @param {int} seconds 
 * @return {string} primary //如果位置集内有主节点，则返回该节点的NodeName
 ******************************************************************************/
function checkAndGetLocationHasPrimary ( db, groupName, location, seconds )
{
   while( seconds-- > 0 )
   {
      sleep( 1000 );
      var primary = getLocationPrimary( db, groupName, location );
      if( primary !== "" )
      {
         break;
      }
   }
   assert.notEqual( "", primary, "Location[" + location + "] doesn't have primary" );
   return primary;
}

/******************************************************************************
 * @description: Get location's primary node name
 * @param {string} groupName
 * @param {string} location
 * @return {string} locationPrimary  // if location has primary, return primary; else return ""
 ******************************************************************************/
function getLocationPrimary ( db, groupName, location )
{
   var groupObj = db.getRG( groupName ).getDetailObj().toObj();

   var locations = groupObj.Locations;
   var groupInfo = groupObj.Group;

   // Get location's primary in SYSCAT.SYSNODES
   var primaryNodeID = 0;
   for( var i in locations )
   {
      var locationItem = locations[i];
      if( location === locationItem["Location"] )
      {
         if( "PrimaryNode" in locationItem )
         {
            primaryNodeID = locationItem["PrimaryNode"];
         } else
         {
            break;
         }
      }
   }
   if( primaryNodeID === 0 )
   {
      return "";
   }

   var hostName, serviceName;
   for( var i in groupInfo )
   {
      var nodeItem = groupInfo[i];
      if( primaryNodeID === nodeItem["NodeID"] )
      {
         hostName = nodeItem["HostName"];
         var services = nodeItem["Service"];
         for( var j in services )
         {
            var service = services[j];
            if( 0 === service["Type"] )
            {
               serviceName = service["Name"];
            }
         }
      }
   }
   var nodeName1 = hostName + ":" + serviceName;

   // Get location's primary in SDB_SNAP_SYSTEM
   var nodeName2 = "";
   var cursor = db.snapshot( SDB_SNAP_SYSTEM, { RawData: true, GroupName: groupName } );
   while( cursor.next() )
   {
      var snapshotObj = cursor.current().toObj();
      if( "ErrNodes" in snapshotObj )
      {
         continue;
      }
      if( location === snapshotObj.Location )
      {
         if( true === snapshotObj.IsLocationPrimary )
         {
            nodeName2 = snapshotObj.NodeName;
            break;
         }
      }
   }
   if( nodeName2 === "" )
   {
      return "";
   }

   // Get location's primary in SDB_SNAP_DATABASE
   var nodeName3 = "";
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, GroupName: groupName } );
   while( cursor.next() )
   {
      var snapshotObj = cursor.current().toObj();
      if( "ErrNodes" in snapshotObj )
      {
         continue;
      }
      if( location === snapshotObj.Location )
      {
         if( true === snapshotObj.IsLocationPrimary )
         {
            nodeName3 = snapshotObj.NodeName;
            break;
         }
      }
   }
   if( nodeName3 === "" )
   {
      return "";
   }

   assert.equal(
      nodeName1,
      nodeName2,
      "Location[" +
      location +
      "] primary in SYSCAT.SYSNODES[" +
      nodeName1 +
      "] is not equal to in SDB_SNAP_SYSTEM[" +
      nodeName2 +
      "]"
   );
   assert.equal(
      nodeName1,
      nodeName3,
      "Location[" +
      location +
      "] primary in SYSCAT.SYSNODES[" +
      nodeName1 +
      "] is not equal to in SDB_SNAP_DATABASE[" +
      nodeName3 +
      "]"
   );

   return nodeName1;
}

/******************************************************************************
 * @description: 获取数据库快照中位置集主节点的部分信息
 * @param {string} NodeName
 * @return {Array} values 包含设置的location、节点中IsLocationPrimary的值、节点的NodeID
 ******************************************************************************/
function getSnapshotDatabase ( db, NodeName )
{
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, NodeName: NodeName } );
   var values = [];
   while( cursor.next() )
   {
      var bson = {};
      bson["Location"] = cursor.current().toObj().Location;
      bson["IsLocationPrimary"] = cursor.current().toObj().IsLocationPrimary;
      bson["NodeID"] = cursor.current().toObj().NodeID[1];
      values.push( bson );
   }
   cursor.close();
   return values;
}

/******************************************************************************
 * @description: 获取分区列表中位置集的主节点 ID
 * @param {string} groupName
 * @return {int} PrimaryNode
 ******************************************************************************/
function getPrimaryNode ( db, groupName )
{
   var cursor = db.list( SDB_LIST_GROUPS, { GroupName: groupName } );
   var obj = cursor.current().toObj();
   var primaryNode = obj["Locations"][0]["PrimaryNode"];
   cursor.close();
   return primaryNode;
}

/******************************************************************************
 * @description: 获取系统快照中位置集主节点的部分信息
 * @param {string} NodeName
 * @return {Array} values 包含设置的location、节点中IsLocationPrimary的值、节点的NodeID
 ******************************************************************************/
function getSnapshotSystem ( db, NodeName )
{
   var cursor = db.snapshot( SDB_SNAP_SYSTEM, { RawData: true, NodeName: NodeName } );
   var values = [];
   while( cursor.next() )
   {
      var bson = {};
      bson["Location"] = cursor.current().toObj().Location;
      bson["IsLocationPrimary"] = cursor.current().toObj().IsLocationPrimary;
      bson["NodeID"] = cursor.current().toObj().NodeID[1];
      values.push( bson );
   }
   cursor.close();
   return values;
}

/******************************************************************************
 * @description: 获取节点的NodeID
 *  @param {string} group  // 节点所在的group
 * @param {string} nodeName  // 节点的NodeName
 ******************************************************************************/
function getNodeId ( db, nodeName )
{
   var cursor = db.snapshot( SDB_SNAP_DATABASE, { RawData: true, NodeName: nodeName } );
   while( cursor.next() )
   {
      var nodeID = cursor.current().toObj().NodeID[1];
   }
   cursor.close();
   return nodeID;
}

/******************************************************************************
 * @description: 检测复制组节点mode
 * @param {Sdb} db
 * @param {string} groupName
 * @param {array} nodeNames
 * @param {string} mode
 ******************************************************************************/
function checkGroupNodeNameMode ( db, groupName, nodeNames, mode )
{
   if( typeof ( nodeNames ) == "string" ) { nodeNames = [nodeNames]; }

   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   var actNodeNames = [];
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      var groupMode = obj.GroupMode;
      assert.equal( groupMode, mode, "group mode is not : " + JSON.stringify( obj ) );
      var properties = obj["Properties"];
      for( var i in properties )
      {
         var nodeName = obj["Properties"][i]["NodeName"];
         actNodeNames.push( nodeName );
      }
   }
   cursor.close();

   assert.equal( actNodeNames.sort(), nodeNames.sort(), JSON.stringify( obj ) );
}

/******************************************************************************
 * @description: 检测复制组没有节点启动mode
 * @param {Sdb} db
 * @param {string} groupName
 * @param {array} nodeNames
 * @param {string} mode
 ******************************************************************************/
function checkGroupStopMode ( db, groupName )
{
   var timeOut = 30;
   var doTime = 0;
   while( doTime < timeOut )
   {
      var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
      var obj = cursor.current().toObj();
      var groupMode = obj.GroupMode;
      if( groupMode == undefined )
      {
         break;
      }
      cursor.close();
      doTime++;
      sleep( 1000 );
   }
   if( doTime >= timeOut )
   {
      throw new Error( "group mode is not : " + JSON.stringify( obj ) );
   }
}