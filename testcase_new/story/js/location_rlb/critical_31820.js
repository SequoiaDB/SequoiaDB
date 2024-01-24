/******************************************************************************
 * @Description   : seqDB-31820:startCriticalMode接口参数校验
 * @Author        : tangtao
 * @CreateTime    : 2023.06.05
 * @LastEditTime  : 2023.10.12
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.skipOneGroup = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_31820";

main( test );
function test ( args )
{
   var srcGroup = commGetDataGroupNames( db )[0];
   var srcGroup2 = commGetDataGroupNames( db )[1];

   var location1 = "guangzhou_31820";
   var location2 = "shenzhen_31820";

   // 获取group中的主备节点
   var slaveNodes = getGroupSlaveNodeName( db, srcGroup );

   // 获取主节点
   var rg = db.getRG( srcGroup );
   var masterNode = rg.getMaster();
   var masterNodeName = masterNode.getHostName() + ":" + masterNode.getServiceName();

   var slaveNode1 = rg.getNode( slaveNodes[0] );
   var slaveNode2 = rg.getNode( slaveNodes[1] );
   var salveNodeID1 = slaveNode1.getNodeDetail().split( ":" )[0];
   var salveNodeID2 = slaveNode2.getNodeDetail().split( ":" )[0];

   // 节点设置Location
   masterNode.setLocation( location1 );
   slaveNode1.setLocation( location1 );
   slaveNode2.setLocation( location2 );

   // 获取 group2 nodeName
   var rg2 = db.getRG( srcGroup2 );
   var masterNode2 = rg2.getMaster();
   var group2NodeName = masterNode2.getHostName() + ":" + masterNode2.getServiceName();
   try
   {
      // 1-1 校验option ，有效值
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 1-2 校验option ，无效值，非object类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         rg.startCriticalMode( "options" );
      } );

      // 1-3 校验option ，无效值，不指定任何参数
      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         rg.startCriticalMode();
      } );

      // 1-4 校验option ，无效值，同时不指定NodeName和Location
      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         var options = { MinKeepTime: 5, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 1-5 校验option ，无效值，不指定MinKeepTime
      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         var options = { NodeName: masterNodeName, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 1-6 校验option ，无效值，不指定MaxKeepTime
      assert.tryThrow( SDB_OUT_OF_BOUND, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5 };
         rg.startCriticalMode( options );
      } );

      // 2-1 校验NodeName ，有效值，string类型，复制组内节点
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 2-2 校验NodeName ，无效值，非string类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: 123, MinKeepTime: 5, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 2-3 校验NodeName ，无效值，指定的节点不在复制组内
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: group2NodeName, MinKeepTime: 5, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 3-1 校验Location ，有效值，string类型，复制组内location
      var options = { Location: location1, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 3-2 校验Location ，无效值，非string类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { Location: 123, MinKeepTime: 5, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 3-3 校验Location ，无效值，指定的location不在复制组内
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { Location: "shanghai_31819", MinKeepTime: 5, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 4-1 校验MinKeepTime ，有效值，int类型，范围(1,10080)
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 4-2 校验MinKeepTime ，有效值，int类型，边界值
      var options = { NodeName: masterNodeName, MinKeepTime: 1, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();
      var options = { NodeName: masterNodeName, MinKeepTime: 10080, MaxKeepTime: 10080 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 4-3 校验MinKeepTime ，无效值，非int类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: "5", MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 4-4 校验MinKeepTime ，无效值，取值大于MaxKeepTime
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 20, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 4-5 校验MinKeepTime ，无效值，取值为边界值以外的值
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 0, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: -1, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 10081, MaxKeepTime: 15 };
         rg.startCriticalMode( options );
      } );

      // 5-1 校验MaxKeepTime ，有效值，int类型，范围(1,10080)
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 5-2 校验MaxKeepTime ，有效值，int类型，边界值
      var options = { NodeName: masterNodeName, MinKeepTime: 1, MaxKeepTime: 1 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 10080 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 5-3 校验MaxKeepTime ，无效值，非int类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: "15" };
         rg.startCriticalMode( options );
      } );

      // 5-4 校验MaxKeepTime ，无效值，取值为边界值以外的值
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 0 };
         rg.startCriticalMode( options );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: -1 };
         rg.startCriticalMode( options );
      } );
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 10081 };
         rg.startCriticalMode( options );
      } );

      // 6-1 校验Enforced ，有效值，bool类型，true/false/不指定
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15, Enforced: true };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15, Enforced: false };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();
      var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15 };
      rg.startCriticalMode( options );
      rg.stopCriticalMode();

      // 6-2 校验Enforced ，无效值，非bool类型
      assert.tryThrow( SDB_INVALIDARG, function()
      {
         var options = { NodeName: masterNodeName, MinKeepTime: 5, MaxKeepTime: 15, Enforced: "true" };
         rg.startCriticalMode( options );
      } );


      // 恢复环境
      rg.stopCriticalMode();

      masterNode.setLocation( "" );
      slaveNode1.setLocation( "" );
      slaveNode2.setLocation( "" );
   }
   finally
   {
      rg.start();
      commCheckBusinessStatus( db );
   }
}