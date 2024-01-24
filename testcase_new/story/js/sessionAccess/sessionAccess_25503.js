/******************************************************************************
 * @Description   : seqDB-25503:设置PerferredConstraint为SecondaryOnly，指定优先访问实例为A 
 * @Author        : liuli
 * @CreateTime    : 2022.03.15
 * @LastEditTime  : 2022.03.16
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.skipExistOneNodeGroup = true;
testConf.csName = COMMCSNAME + "_25503";
testConf.clName = COMMCLNAME + "_25503";
testConf.clOpt = { ReplSize: -1 };
testConf.useSrcGroup = true;

main( test );
function test ( args )
{
   var dbcl = args.testCL;
   var srcGroup = args.srcGroupName;

   // 修改会话属性
   db.setSessionAttr( { "PreferredConstraint": "SECONDARYONLY" } );
   // 插入数据
   insertBulkData( dbcl, 1000 );

   // 获取cl所在组的备节点
   var slaveNodeName = getGroupSlaveNodeName( db, srcGroup );

   // 查看访问计划，访问节点为备节点
   explainAndCheckAccessNodes( dbcl, slaveNodeName );
}