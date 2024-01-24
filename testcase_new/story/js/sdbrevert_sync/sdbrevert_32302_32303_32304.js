/******************************************************************************
 * @Description   : seqDB-32302:集群未设鉴权进行鉴权连接
 *                : seqDB-32303:使用--user/--password指定鉴权信息
 *                : seqDB-32304:使用--copherfile指定鉴权
 * @Author        : liuli
 * @CreateTime    : 2023.07.11
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32302_32303_32304_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;
   var user = "user_32302";
   var password = "password_32302";
   var cipherfile1 = WORKDIR + "/cipherfile_32302_1";
   var cipherfile2 = WORKDIR + "/cipherfile_32302_2";
   var cipherfile3 = WORKDIR + "/cipherfile_32302_3";
   var token = "sequoiadb_32302";

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
   var clNameTmp = "cl_32302_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 未设置鉴权，指定鉴权信息
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --user " + user + " --password " + password;

   cmd.run( command );

   // 校验临时集合数据，不校验lsn和source字段
   var expResult = [{ entry: { _id: 1, a: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile1;
   dropUsr( cmd, command );
   var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile2;
   dropUsr( cmd, command );
   var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile3;
   dropUsr( cmd, command );

   // 生成加密文件
   var command = installDir + "/bin/sdbpasswd --adduser " + user + " --password " + password + " --file " + cipherfile1;
   cmd.run( command );

   // 生成加密文件代token
   var command = installDir + "/bin/sdbpasswd --adduser " + user + " --password " + password + " --token " + token + " --file " + cipherfile2;
   cmd.run( command );

   // 生成一个不正确的加密文件
   var command = installDir + "/bin/sdbpasswd --adduser noneuser_32302" + " --password " + password + " --file " + cipherfile3;
   cmd.run( command );

   // 未设置鉴权，指定--cipherfile参数
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --user " + user + " --cipherfile " + cipherfile1;
   cmd.run( command );
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // 指定--cipherfile，不指定--user
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --cipherfile " + cipherfile1;
   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   // 设置鉴权
   db.createUsr( user, password );

   try
   {
      // 指定正确的用户名和密码
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user " + user + " --password " + password;
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );

      // 指定不存在的用户名进行恢复
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user noneuser_32302" + " --password " + password;
      // 鉴权失败，返回退出码8
      assert.tryThrow( 8, function()
      {
         cmd.run( command );
      } );

      // 指定错误的密码进行恢复
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user " + user + " --password none_32302";
      // 鉴权失败，返回退出码8
      assert.tryThrow( 8, function()
      {
         cmd.run( command );
      } );

      // 指定--cipherfile进行恢复
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user " + user + " --cipherfile " + cipherfile1;
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );

      // 指定--cipherfile和--token参数进行恢复
      tmpCL.truncate();
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user " + user + " --cipherfile " + cipherfile2 + " --token " + token;
      cmd.run( command );
      var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
      commCompareResults( actResult, expResult );

      // 指定--cipherfile参数，其中文件内容不正确
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --user noneuser_32302" + " --cipherfile " + cipherfile3;
      // 鉴权失败，返回退出码8
      assert.tryThrow( 8, function()
      {
         cmd.run( command );
      } );

      // 指定--cipherfile需要token，指定的token不正确
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --cipherfile " + cipherfile2 + " --token " + "token_32302";
      // 指定命令错误，返回退出码127
      assert.tryThrow( 127, function()
      {
         cmd.run( command );
      } );

      // 指定--cipherfile需要token，不指定token
      var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
         + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
         + " --svcname " + COORDSVCNAME + " --cipherfile " + cipherfile2;
      // 指定命令错误，返回退出码127
      assert.tryThrow( 127, function()
      {
         cmd.run( command );
      } );
   }
   finally
   {
      db.dropUsr( user, password );
      // 删除密码文件
      var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile1;
      dropUsr( cmd, command );
      var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile2;
      dropUsr( cmd, command );
      var command = installDir + "/bin/sdbpasswd --removeuser " + user + " --file " + cipherfile3;
      dropUsr( cmd, command );
      remoteObj.close();
   }

   commDropCL( db, COMMCSNAME, clNameTmp );
}

function dropUsr ( cmd, command )
{
   try
   {
      cmd.run( command );
   } catch( e ) { }
}
