/******************************************************************************
 * @Description   : seqDB-32320:使用恢复工具指定参数有误
 * @Author        : liuli
 * @CreateTime    : 2023.07.11
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32320_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var docs = { _id: 1, a: 1 };
   dbcl.insert( docs );

   // 删除数据
   dbcl.remove();
   db.sync( { GroupName: srcGroupName } );

   // 获取主节点
   var masterNode = db.getRG( srcGroupName ).getMaster();
   var hostName = masterNode.getHostName();
   var scvName = masterNode.getServiceName();
   var masterNodeName = hostName + ":" + scvName;

   // 获取同步日志路径
   var replicalogPath = getReplicalogPath( db, masterNodeName );

   var remoteObj = new Remote( hostName, CMSVCNAME );
   var cmd = remoteObj.getCmd();

   // 恢复时不指定--targetcl
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --logpath " + replicalogPath + " --output " + COMMCSNAME + "." + testConf.clName +
      " --hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME;

   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   // 恢复时不指定--logpath
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName +
      " --output " + COMMCSNAME + "." + testConf.clName + " --hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME;
   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   // 恢复时不指定--output
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --hostname " + COORDHOSTNAME + " --svcname " + COORDSVCNAME;
   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   // 指定非法参数
   var command = installDir + "/bin/sdbrevert --none ";
   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   remoteObj.close();
}
