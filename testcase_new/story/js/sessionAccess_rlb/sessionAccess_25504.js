/******************************************************************************
 * @Description   : seqDB-25504:设置PerferredConstraint为SecondaryOnly，指定多个instance实例包含主备节点 
 * @Author        : liuli
 * @CreateTime    : 2022.03.15
 * @LastEditTime  : 2022.06.21
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25504";
testConf.clName = COMMCLNAME + "_25504";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   insertBulkData( dbcl, 1000 );

   var srcGroup = args.srcGroupName;

   var nodes = commGetGroupNodes( db, srcGroup );
   var nodeNames = [];
   for( var i in nodes )
   {
      nodeNames.push( nodes[i].HostName + ":" + nodes[i].svcname );
   }

   try
   {
      // 节点配置instanceid
      var instanceids = [11, 17, 32];
      setInstanceids( db, srcGroup, instanceids );

      // 获取cl所在组的备节点
      var slaveNodeName = getGroupSlaveNodeName( db, srcGroup );
      var instanceids = getNodeNameInstanceids( db, nodeNames );

      // 设置会话属性preferredconstraint和preferredperiod,instanceids指定cl所在group的所有节点
      var options = { "PreferredConstraint": "secondaryonly", "PreferredInstance": instanceids };
      db.setSessionAttr( options );

      // 查看访问计划，访问节点为备节点
      explainAndCheckAccessNodes( dbcl, slaveNodeName );
   }
   finally
   {
      deleteConf( db, { instanceid: 1 }, { GroupName: srcGroup }, SDB_RTN_CONF_NOT_TAKE_EFFECT );

      db.getRG( srcGroup ).stop();
      db.getRG( srcGroup ).start();
      commCheckBusinessStatus( db );
   }
}