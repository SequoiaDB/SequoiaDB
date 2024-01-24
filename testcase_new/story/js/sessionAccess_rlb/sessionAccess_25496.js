/******************************************************************************
 * @Description   : seqDB-25496:设置PerferredConstraint为PrimaryOnly，指定instanceid包含不同组主备节点，其中备节点升主 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.06.22
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25496";
testConf.clName = COMMCLNAME + "_25496";
testConf.clOpt = { ShardingKey: { a: 1 }, ShardingType: "range" };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;

main( test );
function test ( args )
{
   var srcGroup = args.srcGroupName;
   var dstGroup = args.dstGroupNames[0];

   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );
   dbcl.split( srcGroup, dstGroup, { a: 500 } );

   try
   {
      // 节点配置instanceid
      var srcGroupInstanceids = [11, 17, 28];
      var dstGroupInstanceids = [32, 43, 49];
      setInstanceids( db, srcGroup, srcGroupInstanceids );
      setInstanceids( db, dstGroup, dstGroupInstanceids );

      // 获取原组备节点和节点对应instanceid
      var slaveNodeName = getGroupSlaveNodeName( db, srcGroup )[0];
      var slaveInstanceid = getNodeNameInstanceid( db, slaveNodeName );

      // 获取目标组主节点和节点对应instanceid
      var masterNodeName = getGroupMasterNodeName( db, dstGroup )[0];
      var masterInstanceid = getNodeNameInstanceid( db, masterNodeName );

      // 修改会话属性，访问主节点
      dbcl.find( { a: 800 } ).toArray();
      var options = { "PreferredConstraint": "PrimaryOnly", "PreferredInstance": [slaveInstanceid, masterInstanceid] };
      db.setSessionAttr( options );

      // 查询数据在所有组
      assert.tryThrow( SDB_CLS_NOT_PRIMARY, function()
      {
         dbcl.find().toArray();
      } );

      // 查询数据只在目标组
      dbcl.find( { a: 800 } ).toArray();

      // 原组slaveInstanceid对应的备节点升为主节点
      var nodeInfo = slaveNodeName.split( ":" );
      var rg = db.getRG( srcGroup );
      rg.reelect( { "HostName": nodeInfo[0], "ServiceName": nodeInfo[1] } );
      commCheckBusinessStatus( db );

      // 查看访问计划，访问节点为对应节点
      dbcl.find().toArray();
      explainAndCheckAccessNodes( dbcl, [masterNodeName, slaveNodeName] );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: srcGroup }, SDB_RTN_CONF_NOT_TAKE_EFFECT );
      deleteConf( db, { instanceid: 1 }, { GroupName: dstGroup }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( srcGroup ).stop();
      db.getRG( srcGroup ).start();
      db.getRG( dstGroup ).stop();
      db.getRG( dstGroup ).start();
      commCheckBusinessStatus( db );
   }
}
