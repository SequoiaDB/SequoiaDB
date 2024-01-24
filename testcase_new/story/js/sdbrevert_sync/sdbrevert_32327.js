/******************************************************************************
 * @Description   : seqDB-32327:分区表恢复lob
 * @Author        : liuli
 * @CreateTime    : 2023.07.12
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32327_" + generateRandomString( 5 );
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

   // 执行split
   dbcl.split( srcGroupName, dstGroupNames[0], 50 );

   // 插入大lob
   var filePath = WORKDIR + "/lob32327/";
   var fileName = "filelob_32327";
   var fileSize = 30;

   deleteTmpFile( filePath );
   var fileMD5 = makeBigTmpFile( filePath, fileName, fileSize );
   var lobOid = dbcl.putLob( filePath + fileName );

   // 删除lob
   dbcl.deleteLob( lobOid );

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
   var clNameTmp = "cl_32327_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 同时指定原组合目标组恢复所有删除的记录
   var installDir = commGetRemoteInstallPath( nodeInfo.HostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + srcReplicalogPath + "," + dstReplicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname "
      + COORDHOSTNAME + " --svcname " + COORDSVCNAME;

   cmd.run( command );

   // 校验恢复的lob
   tmpCL.getLob( lobOid, filePath + "checkputlob32327", true );
   var actMD5 = File.md5( filePath + "checkputlob32327" );
   assert.equal( fileMD5, actMD5 );

   deleteTmpFile( filePath );
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

function makeBigTmpFile ( filePath, fileName, fileSize )
{
   if( fileSize == undefined ) { fileSize = 1; }
   var fileFullPath = filePath + "/" + fileName;
   File.mkdir( filePath );

   var cmd = new Cmd();
   cmd.run( "dd if=/dev/zero of=" + fileFullPath + " bs=1M count=" + fileSize );
   var md5Arr = cmd.run( "md5sum " + fileFullPath ).split( " " );
   var md5 = md5Arr[0];
   return md5
}