/******************************************************************************
 * @Description   : seqDB-33477:运维模式中指定查询访问备节点
 * @Author        : liuli
 * @CreateTime    : 2023.09.21
 * @LastEditTime  : 2023.10.11
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33477";
testConf.clOpt = { ReplSize: 0 };

main( test );
function test ()
{
   var dbcl = testPara.testCL;
   var srcGroupName = testPara.srcGroupName;

   // 集合插入数据
   var docs = insertBulkData( dbcl, 1000 );

   // 设置会话属性
   db.setSessionAttr( { PreferredInstance: "S", PreferredConstraint: "secondaryonly" } );

   // 查询数据，查看访问计划
   var slaveNodeNames = getGroupSlaveNodeName( db, srcGroupName );
   explainAndCheckAccessNodes( dbcl, slaveNodeNames );

   // 所有备节点启动运维模式
   var group = db.getRG( srcGroupName );
   for( var i in slaveNodeNames )
   {
      var options = { NodeName: slaveNodeNames[i], MinKeepTime: 10, MaxKeepTime: 20 };
      group.startMaintenanceMode( options );
   }

   // 查看访问计划
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.find().explain();
   } )

   // 停止备节点后查看访问计划
   for( var i in slaveNodeNames )
   {
      var slaveNode = group.getNode( slaveNodeNames[i] );
      slaveNode.stop();
   }
   assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
   {
      dbcl.find().explain();
   } )

   // 启动备节点
   group.start();
   commCheckBusinessStatus( db );

   // 一个节点解除运维模式
   var options = { NodeName: slaveNodeNames[0] };
   group.stopMaintenanceMode( options );

   // 查看访问计划
   explainAndCheckAccessNodes( dbcl, slaveNodeNames[0] );

   group.stopMaintenanceMode();
}

function explainAndCheckAccessNodes ( cl, expAccessNodes )
{
   if( typeof ( expAccessNodes ) == "string" ) { expAccessNodes = [expAccessNodes]; }
   var cursor = cl.find().explain();
   var actAccessNodes = [];
   while( cursor.next() )
   {
      var actAccessNode = cursor.current().toObj().NodeName;
      if( actAccessNodes.indexOf( actAccessNode ) === -1 )
      {
         actAccessNodes.push( actAccessNode );
      }
   }
   cursor.close();


   //实际结果与预期结果比较
   for( var i in actAccessNodes )
   {
      if( expAccessNodes.indexOf( actAccessNodes[i] ) === -1 )
      {
         println( "actAccessNodes: " + actAccessNodes + "\nexpAccessNodes: " + expAccessNodes );
         throw new Error( "The actAccessNodes do not include the node: " + expAccessNodes[i] );
      }
   }
}