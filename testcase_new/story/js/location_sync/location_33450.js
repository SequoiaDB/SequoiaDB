/******************************************************************************
 * @Description   : seqDB-33450:2个节点启动运维模式，节点已恢复停止运维模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33450";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;

   var slaveNodeNames = getGroupSlaveNodeName( db, srcGroupName );
   var group = db.getRG( srcGroupName );

   try
   {
      // 指定所有备节点启动运维模式，并停止启动运维模式的备节点
      for( var i in slaveNodeNames )
      {
         var options = { NodeName: slaveNodeNames[i], MinKeepTime: 10, MaxKeepTime: 20 };
         group.startMaintenanceMode( options );
         var slaveNode = group.getNode( slaveNodeNames[i] );
         slaveNode.stop();
      }

      // ReplSize为0的集合插入数据
      var docs = [];
      for( var i = 0; i < 1000; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 启动节点，等待节点恢复正常
      group.start();
      commCheckBusinessStatus( db );

      // 停止运维模式，ReplSize为0的集合插入数据
      group.stopMaintenanceMode();
      dbcl.insert( docs );
   } finally
   {
      // 启动节点，恢复集群
      group.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );
}