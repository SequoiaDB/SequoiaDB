/******************************************************************************
 * @Description   : seqDB-32325:集合rename后恢复数据
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32325_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var clName = "cl_32325_" + generateRandomString( 5 );
   var dbcs = testPara.testCS;
   var dbcl = testPara.testCL;

   // 插入超长记录
   var docs = [{ _id: 1, a: 1, b: 1 }, { _id: 2, a: 2, b: 2 }];
   dbcl.insert( docs );

   // 删除记录
   dbcl.remove( { a: 1 } );

   // 重命名集合
   commDropCL( db, COMMCSNAME, clName );
   dbcs.renameCL( testConf.clName, clName );

   // 再次删除数据
   dbcl = dbcs.getCL( clName );
   dbcl.remove( { a: 2 } );

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
   var clNameTmp = "cl_32325_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 指定集合原始名称恢复记录
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验恢复的记录
   var expResult = [{ entry: { _id: 1, a: 1, b: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // 指定集合新名称恢复记录
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验恢复的记录
   var expResult = [{ entry: { _id: 2, a: 2, b: 2 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
   commDropCL( db, COMMCSNAME, clName );
}