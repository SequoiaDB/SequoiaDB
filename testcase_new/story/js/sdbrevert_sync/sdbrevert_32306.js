/******************************************************************************
 * @Description   : seqDB-32306:使用--targetcl指定需要恢复的集合
 * @Author        : liuli
 * @CreateTime    : 2023.07.07
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.skipStandAlone = true;
main( test );

function test ()
{
   var groupName = commGetDataGroupNames( db )[0];
   var clName = "cl_32306";

   // 获取主节点
   var masterNode = db.getRG( groupName ).getMaster();
   var hostName = masterNode.getHostName();
   var scvName = masterNode.getServiceName();
   var masterNodeName = hostName + ":" + scvName;

   // 获取同步日志路径
   var replicalogPath = getReplicalogPath( db, masterNodeName );

   var remoteObj = new Remote( hostName, CMSVCNAME );
   var cmd = remoteObj.getCmd();

   // 创建临时集合用于恢复数据
   var clNameTmp = "cl_32306_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 指定恢复集合在同步日志中不存在
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   // 校验临时集合数据
   var expResult = [];
   var actResult = tmpCL.find();
   commCompareResults( actResult, expResult );

   // 创建集合并插入数据
   commDropCL( db, COMMCSNAME, clName );
   var dbcl = commCreateCL( db, COMMCSNAME, clName );
   dbcl.insert( { a: 1 } );

   // 指定集合没有可恢复数据
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验临时集合数据
   var expResult = [];
   var actResult = tmpCL.find();
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
   commDropCL( db, COMMCSNAME, clName );
}
