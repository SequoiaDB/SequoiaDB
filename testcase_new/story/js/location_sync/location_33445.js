/******************************************************************************
 * @Description   : seqDB-33445:单个节点启动运维模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.20
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   // 获取一个备节点名
   var groupName = commGetDataGroupNames( db )[0];
   var group = db.getRG( groupName );
   var slaveNodeName = group.getSlave().getHostName() + ":" + group.getSlave().getServiceName();

   // 获取主节点名
   var masterNodeName = group.getMaster().getHostName() + ":" + group.getMaster().getServiceName();

   // 主节点启动运维模式
   var options = { NodeName: masterNodeName, MinKeepTime: 10, MaxKeepTime: 20 };
   group.startMaintenanceMode( options );

   // 检查没有节点启动运维模式
   checkGroupStopMode( db, groupName );

   // 指定备节点启动运维模式
   options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 2 };
   group.startMaintenanceMode( options );

   // 获取运维模式MinKeepTime和MaxKeepTime
   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   var keepTime1 = {};
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      keepTime1["MinKeepTime"] = obj["Properties"][0]["MinKeepTime"];
      keepTime1["MaxKeepTime"] = obj["Properties"][0]["MaxKeepTime"];
   }
   cursor.close();

   // 校验节点运维模式
   var mode = "maintenance";
   checkGroupNodeNameMode( db, groupName, slaveNodeName, mode );

   // 指定备节点再次启动运维模式，keepTime指定不同
   options = { NodeName: slaveNodeName, MinKeepTime: 3, MaxKeepTime: 10 };
   group.startMaintenanceMode( options );

   // 校验节点运维模式
   checkGroupNodeNameMode( db, groupName, slaveNodeName, mode );

   // 获取运维模式MinKeepTime和MaxKeepTime
   var cursor = db.list( SDB_LIST_GROUPMODES, { GroupName: groupName } );
   var keepTime2 = {};
   while( cursor.next() )
   {
      var obj = cursor.current().toObj();
      keepTime2["MinKeepTime"] = obj["Properties"][0]["MinKeepTime"];
      keepTime2["MaxKeepTime"] = obj["Properties"][0]["MaxKeepTime"];
   }
   cursor.close();

   assert.notEqual( keepTime1, keepTime2 );

   // 停止运维模式
   group.stopMaintenanceMode();
}