/******************************************************************************
 * @Description   : seqDB-33459:故障节点启动运维模式，节点恢复后手动停止运维模式
 * @Author        : liuli
 * @CreateTime    : 2023.09.20
 * @LastEditTime  : 2023.09.26
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;

main( test );
function test ()
{
   var clName = "cl_33459";

   // 获取一个备节点名
   var groupName = commGetDataGroupNames( db )[0];
   var group = db.getRG( groupName );
   var slaveNode = group.getSlave();
   var slaveNodeName = slaveNode.getHostName() + ":" + slaveNode.getServiceName();

   try
   {
      // 备节点故障
      slaveNode.stop();

      // 指定备节点启动运维模式
      var options = { NodeName: slaveNodeName, MinKeepTime: 10, MaxKeepTime: 20 };
      group.startMaintenanceMode( options );

      // 创建集合，指定ReplSize为0
      var clOpt = { Group: groupName, ReplSize: 0 };
      var dbcs = testPara.testCS;
      var dbcl = dbcs.createCL( clName, clOpt );

      // 集合插入数据
      var docs = [];
      for( var i = 0; i < 1000; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 启动节点，等待节点恢复正常
      slaveNode.start();
      commCheckBusinessStatus( db );

      // 停止运维模式，ReplSize为0的集合插入数据
      group.stopMaintenanceMode();
      dbcl.insert( docs );
   } finally
   {
      // 启动节点，恢复集群
      slaveNode.start();
      group.stopMaintenanceMode();
      commCheckBusinessStatus( db );
   }

   commDropCL( db, COMMCSNAME, clName );
}