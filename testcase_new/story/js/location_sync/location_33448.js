/******************************************************************************
 * @Description   : seqDB-33448:1个节点启动运维模式，超过最高运行窗口时间强制解除运维模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33448";
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
   var maxKeepTime = 1;
   var options = { NodeName: slaveNodeName, MinKeepTime: 1, MaxKeepTime: 1 };
   group.startMaintenanceMode( options );

   var beginTime = new Date();

   // 停止启动运维模式的节点
   slaveNode.stop();

   try
   {
      // ReplSize为0的集合插入数据
      var docs = [];
      for( var i = 0; i < 1000; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 等待超过最高运行窗口时间
      var waitTime = maxKeepTime + 1;
      validateWaitTime( beginTime, waitTime );

      // 校验节点运维模式
      checkGroupStopMode( db, srcGroupName );

      // ReplSize为0的集合插入数据
      assert.tryThrow( SDB_CLS_NODE_NOT_ENOUGH, function()
      {
         dbcl.insert( docs );
      } )
   } finally
   {
      // 启动节点，恢复集群
      slaveNode.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );

   // 校验数据
   var actResult = dbcl.find().sort( { a: 1 } );
   commCompareResults( actResult, docs );
}