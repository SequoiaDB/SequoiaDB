/******************************************************************************
 * @Description   : seqDB-33461:故障节点启动运维模式，超过最高运行窗口自动停止运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33461";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;

   // 获取一个备节点名
   var group = db.getRG( srcGroupName );
   var slaveNode = group.getSlave();
   var slaveNodeName = slaveNode.getHostName() + ":" + slaveNode.getServiceName();

   try
   {
      // 备节点故障
      slaveNode.stop();

      // 指定备节点启动运维模式
      var maxKeepTime = 2;
      var options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 2 };
      group.startMaintenanceMode( options );
      var beginTime = new Date();

      // 集合插入数据
      var docs = [];
      for( var i = 0; i < 100; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 等待超过最大运行窗口时间
      var waitTime = maxKeepTime + 1;
      validateWaitTime( beginTime, waitTime );
      assert.tryThrow( SDB_CLS_NODE_NOT_ENOUGH, function()
      {
         dbcl.insert( docs );
      } );

   } finally
   {
      // 启动节点，恢复集群
      group.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );
}