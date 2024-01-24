/******************************************************************************
 * @Description   : seqDB-32312:使用--matcher指定匹配条件
 * @Author        : liuli
 * @CreateTime    : 2023.07.11
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32312_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;

   // 插入记录和lob
   var docs = [{ _id: 1, a: 1, b: { c: 1 }, d: 1 }, { _id: 2, a: 2, d: 1 }];
   dbcl.insert( docs );

   // 删除lob
   dbcl.remove( { a: 1 } );
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
   var clNameTmp = "cl_32312_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 指定--matcher可以匹配部分记录
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var matcher = "{a:1}";
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --matcher " + matcher;

   cmd.run( command );
   // 校验恢复的记录
   var expResult = [{ entry: { _id: 1, a: 1, b: { c: 1 }, d: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // 指定--matcher可以匹配所有记录
   tmpCL.truncate();
   var matcher = "{d:1}";
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --matcher " + matcher;
   cmd.run( command );
   // 校验恢复的记录
   var expResult = [{ entry: { _id: 1, a: 1, b: { c: 1 }, d: 1 }, optype: "DOC_DELETE", "label": "" },
   { entry: { _id: 2, a: 2, d: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } ).sort( { "lsn": 1 } );
   commCompareResults( actResult, expResult );

   // 指定--matcher匹配不到记录
   tmpCL.truncate();
   var matcher = "{a:3}";
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --matcher " + matcher;
   cmd.run( command );
   assert.equal( 0, tmpCL.count() );

   // 指定--matcher为嵌套的bson
   tmpCL.truncate();
   var matcher = "{b:{c:1}}";
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --matcher " + matcher;
   cmd.run( command );
   // 校验恢复的记录
   var expResult = [{ entry: { _id: 1, a: 1, b: { c: 1 }, d: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
}