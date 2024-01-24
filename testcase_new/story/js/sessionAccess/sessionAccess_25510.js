/******************************************************************************
 * @Description   : seqDB-25510:设置PerferredConstraint为SecondaryOnly，指定优先访问备节点实例，执行插入操作后查询
 * @Author        : liuli
 * @CreateTime    : 2022.03.15
 * @LastEditTime  : 2022.03.15
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25510";
testConf.clName = COMMCLNAME + "_25510";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;

   // 修改会话属性
   db.setSessionAttr( { "PreferredConstraint": "SECONDARYONLY", "PreferredInstance": "A" } );
   // 插入数据
   insertBulkData( dbcl, 1000 );

   // 获取cl所在组的备节点
   var slaveNodeName = getGroupSlaveNodeName( db, srcGroup );

   // 查看访问计划，访问节点为备节点
   explainAndCheckAccessNodes( dbcl, slaveNodeName );
}