/******************************************************************************
 * @Description   : seqDB-32301:使用--hosts指定连接地址列表
 * @Author        : liuli
 * @CreateTime    : 2023.07.07
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32301_" + generateRandomString( 5 );
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

   // 创建临时集合用于恢复数据
   var clNameTmp = "cl_32301_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );
   var coordUrl = getCoordUrl( db );

   // --hosts 指定单个地址恢复
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + coordUrl[0];

   cmd.run( command );

   // 校验临时集合数据，不校验lsn和source字段
   var expResult = [{ entry: { _id: 1, a: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // --hosts 指定多个地址恢复
   tmpCL.truncate();
   if( coordUrl.length > 1 )
   {
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + coordUrl[0] + "," + coordUrl[1];
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );

      // --hosts 指定多个地址恢复，并发数指定不为1
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + coordUrl[0] + "," + coordUrl[1] + " --jobs 4";
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );

      // --hosts 指定多个地址恢复，中间存在地址不可用
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + coordUrl[0] + ","
         + "host_32301:32301" + "," + coordUrl[1];
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );
   }

   // --hosts 指定多个地址恢复，其中第一个地址不可用
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + "host_32301:32301" + "," + coordUrl[0];
   cmd.run( command );
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // --hosts 指定多个地址恢复，全部地址不可用
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hosts " + "host_32301_1:32301" + ","
      + "host_32301_1:32301";
   // 连接地址不可用，返回退出码135
   assert.tryThrow( 135, function()
   {
      cmd.run( command );
   } );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
}
