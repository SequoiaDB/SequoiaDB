/******************************************************************************
 * @Description   : seqDB-33449:2个节点启动运维模式，节点未恢复停止运维模式
 * @Author        : tangtao
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.10.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33449";
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
      for( var i = 0; i < 100; i++ )
      {
         docs.push( { a: i } );
      }
      dbcl.insert( docs );

      // 停止运维模式，ReplSize为0的集合插入数据
      group.stopMaintenanceMode();

      assert.tryThrow( [SDB_CLS_NODE_NOT_ENOUGH, SDB_CLS_NOT_PRIMARY], function()
      {
         dbcl.insert( docs );
      } );

      var loop = 0;
      // CI搭建集群将sharingbreak调整为20000
      while( loop < 30 )
      {
         try
         {
            var master = group.getMaster();
            loop++;
            if( loop == 30 )
            {
               throw new Error( "stop Maintenance Mode, group has master node. " + master.getHostName() + ":" + master.getServiceName() );
            }
            sleep( 1000 );
         }
         catch( e )
         {
            if( e == SDB_RTN_NO_PRIMARY_FOUND )
            {
               break;
            }
            else
            {
               throw e;
            }
         }
      }

   } finally
   {
      // 启动节点，恢复集群
      group.start();
      group.stopMaintenanceMode();
   }

   commCheckBusinessStatus( db );
}