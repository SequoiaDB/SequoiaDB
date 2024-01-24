/************************************************************************
 * @Description : seqDB-22256:通过sequoiafs工具配置参数全新挂载
 * @Author : 2020/07/15  xiaoni huang init
************************************************************************/

//main( test ); //CI主机需要先安装fuse，待安装后放开
function test ()
{
   var alias = 'sequoiafs_22256';
   var mountpoint = tmpFileDir + alias;
   var hosts = COORDHOSTNAME + ':' + COORDSVCNAME;
   var confpath = tmpFileDir + alias + '.conf';

   // 清理
   cleanFsPid( alias );
   cleanFsDir( alias );

   // 准备目标挂载目录和配置文件
   readyMountpointAndConf( alias );
   var tmpConfpath = toolsDir + 'conf/local/' + alias + '/sequoiafs.conf';
   cmd.run( 'mv ' + tmpConfpath + ' ' + confpath );

   // sequoiafs挂载目录
   var command = toolsDir + 'bin/sequoiafs --alias ' + alias + ' -m ' + mountpoint + ' -c ' + confpath + ' -l ' + fsCollection + ' &';
   try
   {
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         println( command );
         throw new Error( e );
      }
   }

   // 检查结果
   // fslist
   // SEQUOIADBMAINSTREAM-6086, fslist在ubuntu会coredump，暂时屏蔽该方法
   /*
   var rc = fslist( '-m run -l' );
   if( rc.indexOf( alias ) === -1 || rc.indexOf( confpath ) === -1 )
   {
      throw new Error( 'expect ' + alias + ' mount, bug actual fslist.sh rc: \n' + rc );
   }
   */

   // linux mount
   var rc = cmd.run( 'mount |grep ' + alias );
   if( rc.indexOf( alias ) === -1 )
   {
      throw new Error( 'expect ' + alias + ' mount, bug actual mount rc: \n' + rc );
   }

   // write file
   var mountpoint = tmpFileDir + alias;
   var fileDir = mountpoint + '/' + alias + 'txt';
   cmd.run( 'echo sequoiafs > ' + fileDir );
   var rc = cmd.run( 'cat ' + fileDir );
   if( rc.indexOf( 'sequoiafs' ) === -1 )
   {
      throw new Error( 'Failed to write file in the ' + mountpoint );
   }

   // 清理   
   cleanFsPid( alias );
   cleanFsDir( alias );
}