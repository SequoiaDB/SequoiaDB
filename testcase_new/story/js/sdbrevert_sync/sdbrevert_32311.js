/******************************************************************************
 * @Description   : seqDB-32311:使用--datatype指定恢复数据类型
 * @Author        : liuli
 * @CreateTime    : 2023.07.11
 * @LastEditTime  : 2023.07.25
 * @LastEditors   : liuli
 ******************************************************************************/
testConf.clName = COMMCLNAME + "_32311_" + generateRandomString( 5 );
testConf.useSrcGroup = true;
testConf.skipStandAlone = true;

main( test );

function test ( testPara )
{
   var filePath = WORKDIR + "/lob32311/";
   var fileName = "filelob_32311";
   var fileSize = 1024 * 1024;
   var srcGroupName = testPara.srcGroupName;
   var dbcl = testPara.testCL;

   // 插入记录和lob
   var docs = { _id: 1, a: 1 };
   dbcl.insert( docs );
   deleteTmpFile( filePath );
   var fileMD5 = makeTmpFile( filePath, fileName, fileSize );
   var lobOid = dbcl.putLob( filePath + fileName );

   // 删除lob
   dbcl.remove();
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
   var clNameTmp = "cl_32311_tmp";
   var tmpCL = commCreateCL( db, COMMCSNAME, clNameTmp, {}, true, true );

   // 不指定--datatype
   var installDir = commGetRemoteInstallPath( hostName, CMSVCNAME );
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME;

   cmd.run( command );
   // 校验恢复的记录和lob
   tmpCL.getLob( lobOid, filePath + "checkputlob32311", true );
   var actMD5 = File.md5( filePath + "checkputlob32311" );
   assert.equal( fileMD5, actMD5 );
   var expResult = [{ entry: { _id: 1, a: 1 }, optype: "DOC_DELETE", "label": "" }];
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // 指定--datatype为doc
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --datatype doc";
   cmd.run( command );
   // 校验恢复的记录
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );
   // 校验不存在lob
   var cursor = tmpCL.listLobs();
   if( cursor.next() )
   {
      throw new Error( "expect cursor to be empty,cusror : " + JSON.stringify( cursor.current().toObj() ) );
   }
   cursor.close();

   // 指定--datatype为lob
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --datatype lob";
   cmd.run( command );
   // 校验恢复的lob
   tmpCL.getLob( lobOid, filePath + "checkputlob32311", true );
   var actMD5 = File.md5( filePath + "checkputlob32311" );
   assert.equal( fileMD5, actMD5 );
   // 校验不存在记录
   assert.equal( tmpCL.count(), 0 );

   // 指定--datatype为all
   tmpCL.truncate();
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --datatype all";
   cmd.run( command );
   // 校验恢复的记录和lob
   tmpCL.getLob( lobOid, filePath + "checkputlob32311", true );
   var actMD5 = File.md5( filePath + "checkputlob32311" );
   assert.equal( fileMD5, actMD5 );
   var actResult = tmpCL.find( {}, { "lsn": { "$include": 0 }, "source": { "$include": 0 } } );
   commCompareResults( actResult, expResult );

   // 指定--datatype为非法值
   var command = installDir + "/bin/sdbrevert --targetcl " + COMMCSNAME + "." + testConf.clName + " --logpath "
      + replicalogPath + " --output " + COMMCSNAME + "." + clNameTmp + " --hostname " + COORDHOSTNAME
      + " --svcname " + COORDSVCNAME + " --datatype none";
   // 指定命令错误，返回退出码127
   assert.tryThrow( 127, function()
   {
      cmd.run( command );
   } );

   remoteObj.close();
   deleteTmpFile( filePath );
   commDropCL( db, COMMCSNAME, clNameTmp );
}