/******************************************************************************
 * @Description   : seqDB-33447:1个节点启动运维模式，超过最低运行窗口时间自动解除运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.20
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33447";
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

   // 指定一个备节点启动运维模式
   var minKeepTime = 1;
   var options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 5 };
   group.startMaintenanceMode( options );

   var beginTime = new Date();

   // 停止启动运维模式的节点
   slaveNode.stop();

   try
   {
      // ReplSize为0的集合插入数据
      var docs = [];
      for( var i = 0; i < 100; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 等待超过最小运行窗口时间
      var waitTime = minKeepTime + 1;
      validateWaitTime( beginTime, waitTime );
      slaveNode.start();
      commCheckBusinessStatus( db );

      // 校验节点运维模式
      checkGroupStopMode( db, srcGroupName );

   }
   finally
   {
      group.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );
}