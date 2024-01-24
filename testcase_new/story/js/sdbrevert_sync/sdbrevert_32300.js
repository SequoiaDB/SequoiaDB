/******************************************************************************
 * @Description   : seqDB-32300:使用--hostname和--svcname指定连接地址
 * @Author        : liuli
 * @CreateTime    : 2023.07.07
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
// 集合名称增加5个字符的随机字符串，防止在重复执行用例时恢复到前一次执行用例删除的数据
testConf.clName = COMMCLNAME + "_32300_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var docs = { _id: 2, a: 2 };
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

   // 创建临时集合用于恢复数据
   var clNameTmp = "cl_32300_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // --hostname和--svcname为正确地址
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验临时集合数据，不校验lsn和source字段
   var expResult = [{ entry: { _id: 2, a: 2 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // --hostname和--svcname指定地址不可用
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + "host_32300"
      + " --svcname " + "32300";

   // 连接地址不可用，返回退出码135
   assert.tryThrow( 135, function()
   {
      cmd.run( command );
   } );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
}
