/******************************************************************************
*@Description : seqDB-13658:scp命令结束后，File.scp内部显示调用close关闭File对象 
*               预期执行成功，执行失败可能报"~bash: ./sdblist: Text file busy"
*@Author      : 2019-3-19  XiaoNi Huang
******************************************************************************/

main( test );

function test ()
{
   var installDir = toolGetSequoiadbDir( COORDHOSTNAME, CMSVCNAME );
   var tmpPath = WORKDIR + '/jsObject/13658/';
   var dstFile = tmpPath + '/sdb';
   var cmd = new Cmd();
   cmd.run( "mkdir -p " + tmpPath );

   // TODO 后续优化用例需要考虑本地没有装sequoiadb的情况，这个后面优化用例时统一在公共方法考虑吧，此用例暂不处理
   var localFile = installDir[0] + '/bin/sdb';
   var remoteFile = COORDHOSTNAME + ":" + CMSVCNAME + "@" + installDir[0] + '/bin/sdb';
   var scrFiles = [localFile, remoteFile];
   for( var i = 0; i < scrFiles.length; i++ )
   {
      var srcFile = scrFiles[i];
      File.scp( srcFile, dstFile );

      // check results
      var rc = cmd.run( dstFile + " -v" );
      var actRcCode = rc.indexOf( "SequoiaDB" );
      assert.equal( actRcCode, 0 );
      cmd.run( "rm " + dstFile );
   }
}