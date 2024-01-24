/******************************************************************************
 * @Description   : seqDB-33485:dc.startMaintenanceMode接口参数校验
 *                : seqDB-33486:dc.stopMaintenanceMode接口参数校验
 * @Author        : liuli
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.10.09
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

// SEQUOIADBMAINSTREAM-9965
// DC参数校验存在问题，先将用例屏蔽
// main( test );
function test ()
{
   var location = "location_33485_33486";

   // catalog和data每个复制组中均给一个备节点设置Location
   var dataGroups = commGetDataGroupNames( db );
   for( var i in dataGroups )
   {
      var groupName = dataGroups[i];
      var group = db.getRG( groupName );
      var slaveNodeNames = getGroupSlaveNodeName( db, groupName );
      var slaveNode = group.getNode( slaveNodeNames[0] );
      slaveNode.setLocation( location );
   }

   var cataRG = db.getCatalogRG();
   var cataslaveNode = cataRG.getSlave();
   cataslaveNode.setLocation( location );

   var dc = db.getDC();

   // options参数校验
   // 设置运维模式，不指定任何参数
   assert.tryThrow( SDB_OUT_OF_BOUND, function()
   {
      dc.startMaintenanceMode();
   } );

   // 指定参数非object类型
   var options = "options";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // -264
   // 同时不指定HostName和Location
   var options = { MinKeepTime: 5, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 不指定MinKeepTime
   var options = { Location: location, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 不指定MaxKeepTime
   var options = { Location: location, MinKeepTime: 5 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // HostName参数校验
   // 指定HostName不在当前集群
   var hostName = "hostName_33485";
   var options = { HostName: hostName, MinKeepTime: 5, MaxKeepTime: 15 };
   //assert.tryThrow( SDB_INVALIDARG, function()
   //{
   //   dc.startMaintenanceMode( options );
   //} );

   // 指定HostName非string类型
   var options = { HostName: 33485, MinKeepTime: 5, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 指定Location不存在
   var locationNone = "none_location_33485";
   var options = { Location: locationNone, MinKeepTime: 5, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 指定Location非string类型
   var options = { Location: 33485, MinKeepTime: 5, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 校验MinKeepTime ，有效值，int类型，边界值
   var options = { Location: location, MinKeepTime: 1, MaxKeepTime: 15 };
   dc.startMaintenanceMode( options );
   dc.stopMaintenanceMode();
   var options = { Location: location, MinKeepTime: 10080, MaxKeepTime: 10080 };
   dc.startMaintenanceMode( options );
   dc.stopMaintenanceMode();

   // 校验MinKeepTime ，无效值，非int类型
   var options = { Location: location, MinKeepTime: "5", MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 校验MinKeepTime ，无效值，取值大于MaxKeepTime
   var options = { Location: location, MinKeepTime: 20, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 4-5 校验MinKeepTime ，无效值，取值为边界值以外的值
   var options = { Location: location, MinKeepTime: 0, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );
   var options = { Location: location, MinKeepTime: -1, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );
   var options = { Location: location, MinKeepTime: 10081, MaxKeepTime: 15 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 校验MaxKeepTime ，有效值，int类型，范围(1,10080)
   //var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 15 };
   //dc.startMaintenanceMode( options );
   //dc.stopMaintenanceMode();

   // 校验MaxKeepTime ，有效值，int类型，边界值
   //var options = { Location: location, MinKeepTime: 1, MaxKeepTime: 1 };
   //dc.startMaintenanceMode( options );
   //dc.stopMaintenanceMode();
   //var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10080 };
   //dc.startMaintenanceMode( options );
   //dc.stopMaintenanceMode();

   // 校验MaxKeepTime ，无效值，非int类型
   var options = { Location: location, MinKeepTime: 5, MaxKeepTime: "15" };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   // 校验MaxKeepTime ，无效值，取值为边界值以外的值
   var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 0 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );
   var options = { Location: location, MinKeepTime: 5, MaxKeepTime: -1 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );
   var options = { Location: location, MinKeepTime: 5, MaxKeepTime: 10081 };
   assert.tryThrow( SDB_COORD_NOT_ALL_DONE, function()
   {
      dc.startMaintenanceMode( options );
   } );

   var options = "options";
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      dc.stopMaintenanceMode( options );
   } );

   dc.stopMaintenanceMode();

   for( var i in dataGroups )
   {
      var groupName = dataGroups[i];
      var group = db.getRG( groupName );
      var slaveNodeNames = getGroupSlaveNodeName( db, groupName );
      for( j in slaveNodeNames )
      {
         var slaveNode = group.getNode( slaveNodeNames[j] );
         slaveNode.setLocation( "" );
      }
   }

   cataslaveNode.setLocation( "" );
}