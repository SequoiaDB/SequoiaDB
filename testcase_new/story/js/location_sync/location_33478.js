/******************************************************************************
 * @Description   : seqDB-33478:运维模式中指定查询lob访问备节点
 * @Author        : liuli
 * @CreateTime    : 2023.09.22
 * @LastEditTime  : 2023.10.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33478";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;
   var slaveNodeNames = getGroupSlaveNodeName( db, srcGroupName );

   var filePath = WORKDIR + "/lob23807/";
   var fileName = "filelob_23807";
   var fileSize = 1024 * 1024;

   deleteTmpFile( filePath );
   makeTmpFile( filePath, fileName, fileSize );
   var lobOid = dbcl.putLob( filePath + fileName );

   // 设置会话属性
   db.setSessionAttr( { PreferredInstance: "S", PreferredConstraint: "secondaryonly" } );

   // 所有备节点启动运维模式
   var group = db.getRG( srcGroupName );
   for( var i in slaveNodeNames )
   {
      var options = { NodeName: slaveNodeNames[i], MinKeepTime: 10, MaxKeepTime: 20 };
      group.startMaintenanceMode( options );
   }

   // 获取lob详细信息
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.getLobDetail( lobOid );
   } )

   // 停止备节点后查看访问计划
   for( var i in slaveNodeNames )
   {
      var slaveNode = group.getNode( slaveNodeNames[i] );
      slaveNode.stop();
   }
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.getLobDetail( lobOid );
   } )

   // 启动备节点
   group.start();
   commCheckBusinessStatus( db );

   // 一个节点解除运维模式
   var options = { NodeName: slaveNodeNames[0] };
   group.stopMaintenanceMode( options );

   // 查看访问计划
   dbcl.getLobDetail( lobOid );

   group.stopMaintenanceMode();
}