/******************************************************************************
 * @Description   : seqDB-33463:超过半数节点故障，故障节点启动运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.09.21
 * @LastEditors   : tangtao
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33463";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;

   // 获取备节点名
   var group = db.getRG( srcGroupName );
   var slaveNodeNames = getGroupSlaveNodeName( db, srcGroupName );

   var slaveNode1 = group.getNode( slaveNodeNames[0] );

   try
   {
      // 备节点故障
      for( var i in slaveNodeNames )
      {
         var options = { NodeName: slaveNodeNames[i], MinKeepTime: 1, MaxKeepTime: 3 };
         group.startMaintenanceMode( options );
         var slaveNode = group.getNode( slaveNodeNames[i] );
         slaveNode.stop();
      }
      var minKeepTime = 1;
      var maxKeepTime = 3;
      var beginTime = new Date();
      checkGroupNodeInMaintenanceMode( db, srcGroupName, slaveNodeNames );

      // 集合插入数据
      var docs = [];
      for( var i = 0; i < 100; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 一个节点恢复，等待超过最小保持时间
      slaveNode1.start();
      var waitTime = minKeepTime + 1;
      validateWaitTime( beginTime, waitTime );

      dbcl.insert( docs );

      // 超过最大保持时间
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