/******************************************************************************
 * @Description   : seqDB-32321:恢复普通表删除数据
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32321_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;

   // 插入超长记录
   var array = new Array( 1024 * 10 );
   var str = array.join( "a" );
   var docs = [{ _id: 1, a: 1, b: 1 }, { _id: 2, a: 2, b: 2 }, { _id: 3, a: 3, b: str }];
   dbcl.insert( docs );

   // 使用remove删除记录
   dbcl.remove( { a: 1 } );

   // 使用findAndRemove删除记录
   dbcl.find( { a: 2 } ).remove().toArray();

   // 删除超长记录
   dbcl.remove( { a: 3 } );

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
   var clNameTmp = "cl_32321_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 恢复所有删除的记录
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验恢复的记录
   var expResult = [{ entry: { _id: 1, a: 1, b: 1 }, optype: "DOC_DELETE", "label": "" },
   { entry: { _id: 2, a: 2, b: 2 }, optype: "DOC_DELETE", "label": "" },
   { entry: { _id: 3, a: 3, b: str }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } ).sort( { "lsn": 1 } );
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
}