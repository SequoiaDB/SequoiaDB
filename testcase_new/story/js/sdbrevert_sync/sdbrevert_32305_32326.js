/******************************************************************************
 * @Description   : seqDB-32305:使用--ssl指定使用ssl连接
 *                : seqDB-32326:恢复普通表removeLob
 * @Author        : liuli
 * @CreateTime    : 2023.07.11
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32305_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var filePath = WORKDIR + "/lob32305/";
   var fileName = "filelob_32305";
   var fileSize = 1024 * 1024;
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;

   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOid = dbcl.putLob( filePath + fileName );

   // 删除lob
   dbcl.deleteLob( lobOid );
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
   var clNameTmp = "cl_32305_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 集群未开启ssl，指定--ssl为true
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --ssl true";
   // 无法连接，返回退出码135
   assert.tryThrow( 135, function()
   {
      cmd.run( command );
   } );

   // 集群未开启ssl，指定--ssl为false
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --ssl false";
   cmd.run( command );
   // 校验恢复的lob
   tmpCL.getLob( lobOid, filePath + "checkputlob32305", true );
   var actMD5 = File.md5( filePath + "checkputlob32305" );
   assert.equal( fileMD5, actMD5 );

   // 集群开启ssl连接
   db.updateConf( { usessl: true } );

   try
   {
      // 集群开启ssl，指定--ssl为true
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --ssl true";
      cmd.run( command );
      tmpCL.getLob( lobOid, filePath + "checkputlob32305", true );
      var actMD5 = File.md5( filePath + "checkputlob32305" );
      assert.equal( fileMD5, actMD5 );

      // 集群开启ssl，指定--ssl为false
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --ssl false";
      cmd.run( command );
      tmpCL.getLob( lobOid, filePath + "checkputlob32305", true );
      var actMD5 = File.md5( filePath + "checkputlob32305" );
      assert.equal( fileMD5, actMD5 );
   }
   finally
   {
      db.deleteConf( { usessl: false } );
      remoteObj.close();
   }

   deleteTmpFile( filePath );
   commDropCL( db, COMMCSNAME, clNameTmp );
}