/******************************************************************************
 * @Description   : seqDB-32322:恢复分区表删除数据
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32322_" + generateRandomString( 5 );
// 设置ReplSize为0，保证同一主机上所有节点日志都写入完成
testConf.clOpt = { ReplSize: 0, ShardingKey: { a: 1 } };
testConf.useSrcGroup = true;
testConf.useDstGroup = true;
testConf.skipOneGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dstGroupNames = testPara.dstGroupNames;
   var dbcl = testPara.testCL;

   // 插入数据
   var docs = [];
   for( var i = 0; i < 1000; i++ )
   {
      docs.push( { _id: i, a: i, b: i } );
   }
   dbcl.insert( docs );

   // 执行split
   dbcl.split( srcGroupName, dstGroupNames[0], 50 );

   // 删除数据
   dbcl.remove( { a: { $gt: 500 } } );

   db.sync( { GroupName: srcGroupName } );
   db.sync( { GroupName: dstGroupNames[0] } );

   // 获取目标组和原组相同机器的节点
   var srcGroupNodes = commGetGroupNodes( db, srcGroupName );
   var dstGroupNodes = commGetGroupNodes( db, dstGroupNames[0] );

   var nodeInfo = getCommonHostname( srcGroupNodes, dstGroupNodes );

   if( nodeInfo == null )
   {
      // 两组没有节点在相同机器，不执行恢复
      return;
   }
   var srcNodeName = nodeInfo.HostName + ":" + nodeInfo.srcSvcname;
   var dstNodeName = nodeInfo.HostName + ":" + nodeInfo.dstSvcname;

   // 获取同步日志路径
   var srcReplicalogPath = getReplicalogPath( db, srcNodeName );
   var dstReplicalogPath = getReplicalogPath( db, dstNodeName );

   var remoteObj = new Remote( nodeInfo.HostName, CMSVCNAME );
   var cmd = remoteObj.getCmd();

   // 创建临时集合用于恢复数据
   var clNameTmp = "cl_32322_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 同时指定原组合目标组恢复所有删除的记录
   var installDir = commGetRemoteInstallPath( nodeInfo.HostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + srcReplicalogPath + "," + dstReplicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验恢复的记录
   var expResult = [];
   for( var i = 501; i < 1000; i++ )
   {
      expResult.push( { entry: { _id: i, a: i, b: i }, optype: "DOC_DELETE", "label": "" } );
   }
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } ).sort( { "entry.a": 1 } );
   commCompareResults( actResult, expResult );

   remoteObj.close();
   commDropCL( db, COMMCSNAME, clNameTmp );
}

function getCommonHostname ( srcGroupNodes, dstGroupNodes )
{
   for( var i in srcGroupNodes )
   {
      for( var j in dstGroupNodes )
      {
         if( srcGroupNodes[i].HostName == dstGroupNodes[j].HostName )
         {
            return {
               HostName: srcGroupNodes[i].HostName,
               srcSvcname: srcGroupNodes[i].svcname,
               dstSvcname: dstGroupNodes[j].svcname
            };
         }
      }
   }
   return null;
}