/******************************************************************************
 * @Description   : seqDB-33626:节点设置运维模式后指定集合执行resetSnapshot和analyze
 * @Author        : liuli
 * @CreateTime    : 2023.09.26
 * @LastEditTime  : 2023.10.09
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
testConf.useSrcGroup = true;
testConf.clName = COMMCLNAME + "_33626";

main( test );
function test ()
{
   var srcGroupName = testPara.srcGroupName;

   // 获取一个备节点名
   var group = db.getRG( srcGroupName );
   var slaveNode = group.getSlave();
   var slaveNodeName = slaveNode.getHostName() + ":" + slaveNode.getServiceName();

   // 指定一个备节点启动运维模式
   var options = { NodeName: slaveNodeName, MinKeepTime: 10, MaxKeepTime: 20 };
   group.startMaintenanceMode( options );

   // 指定集合执行重置快照
   db.resetSnapshot( { Type: "collections", Collection: COMMCSNAME + "." + testConf.clName } );

   // 指定集合执行analyze
   db.analyze( { Mode: 4, Collection: COMMCSNAME + "." + testConf.clName } );

   db.analyze( { Mode: 5, Collection: COMMCSNAME + "." + testConf.clName } );

   group.stopMaintenanceMode();
}