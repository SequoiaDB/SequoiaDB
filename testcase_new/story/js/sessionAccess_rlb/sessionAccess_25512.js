/******************************************************************************
 * @Description   : seqDB-25512:设置PerferredConstraint为SecondaryOnly，指定instanceid包含不同组主备节点，其中主节点异常降备 
 * @Author        : liuli
 * @CreateTime    : 2022.03.16
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipOneGroup = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25512";
testConf.clName = COMMCLNAME + "_25512";
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

      // 修改会话属性，访问备节点，指定节点instanceid为主节点
      var options = { "PreferredConstraint": "SecondaryOnly", "PreferredInstance": [slaveInstanceid, masterInstanceid] };
      db.setSessionAttr( options );

      // 查询数据在所有组
      assert.tryThrow( SDB_CLS_NOT_SECONDARY, function()
      {
         dbcl.find().toArray();
      } );

      // 查询数据只在原组
      dbcl.find( { a: 100 } ).toArray();

      // 目标组重新选主
      masterChangToSlave( db, dstGroup );

      // 查看访问计划，访问节点为对应节点
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