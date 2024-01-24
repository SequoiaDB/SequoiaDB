/******************************************************************************
 * @Description   : seqDB-31772:Critical模式中修改Location
 * @Author        : liuli
 * @CreateTime    : 2023.05.29
 * @LastEditTime  : 2023.05.29
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;

main( test );
function test ()
{
   var location1 = "location_31772_1";
   var location2 = "location_31772_2";

   var dataGroupName = commGetDataGroupNames( db )[0];

   // 获取group中的备节点
   var rg = db.getRG( dataGroupName );
   var slaveNode = rg.getSlave();

   // 给节点设置Location
   slaveNode.setLocation( location1 );

   try
   {
      // 备节点所在Location启动Critical模式
      var minKeepTime = 10;
      var maxKeepTime = 20;
      var options = { Location: location1, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );

      // 校验Critical模式启动成功
      checkStartCriticalMode( db, dataGroupName, options );

      // 节点修改Location为相同Location
      slaveNode.setLocation( location1 );

      // 校验Critical模式启动成功
      checkStartCriticalMode( db, dataGroupName, options );

      // 节点修改Location为不同Location
      slaveNode.setLocation( location2 );

      // 校验Critical模式自动停止
      sleep( 2000 );
      checkStopCriticalMode( db, dataGroupName );

      // Location2启动Critical模式
      var minKeepTime = 10;
      var maxKeepTime = 20;
      var options = { Location: location2, MinKeepTime: minKeepTime, MaxKeepTime: maxKeepTime };
      rg.startCriticalMode( options );
      checkStartCriticalMode( db, dataGroupName, options );

      // 节点删除Location
      slaveNode.setLocation( "" );
      // 校验删除Location后Critical模式自动停止
      sleep( 2000 );
      checkStopCriticalMode( db, dataGroupName );
   }
   finally
   {
      // 节点删除Location
      slaveNode.setLocation( "" );
   }
}