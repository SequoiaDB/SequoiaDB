/************************************************************************
 * @Description : seqDB-22270:通过sequoiafs配置fuse参数
 * @Author : 2020/07/15  xiaoni huang init
************************************************************************/

//main( test ); //CI主机需要先安装fuse，待安装后放开
function test ()
{
   var alias = 'sequoiafs_22270';
   var mountpoint = tmpFileDir + alias;
   var hosts = COORDHOSTNAME + ':' + COORDSVCNAME;
   var confpath = toolsDir + 'conf/local/' + alias + '/sequoiafs.conf';
   var metadircollection = fsCollection + '_FS_SYS_DirMeta';
   var metafilecollection = fsCollection + '_FS_SYS_FileMeta';

   // 清理
   cleanFsPid( alias );
   cleanFsDir( alias );

   // 准备目标挂载目录和配置文件
   readyMountpointAndConf( alias );

   // 在目标挂载目录写文件，构造目录非空
   cmd.run( 'echo "test" > ' + mountpoint + '/test.txt' );
   var rc = cmd.run( 'ls ' + mountpoint );
   if( rc.length === 0 )
   {
      throw new Error( 'expect ' + mountpoint + ' not empty, but actual rc: ' + rc );
   }

   // sequoiafs挂载目录，配置--fuse_nonempty false
   var command = toolsDir + 'bin/sequoiafs --alias ' + alias + ' -m ' + mountpoint + ' -c ' + confpath + ' -l ' + fsCollection + ' --fuse_nonempty false &';
   try
   {
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         println( command );
         throw e;
      }
   }

   // sequoiafs挂载目录，配置--fuse_nonempty true，及其他所有sequoiafs参数
   // 并配置未定义的fuse参数，如：kernel_cache
   var hosts = COORDHOSTNAME + ':' + COORDSVCNAME;
   var confpath = toolsDir + 'conf/local/' + alias + '/sequoiafs.conf';
   var metadircollection = fsCollection + '_FS_SYS_DirMeta';
   var metafilecollection = fsCollection + '_FS_SYS_FileMeta';
   var command = toolsDir + 'bin/sequoiafs --alias ' + alias + ' -m ' + mountpoint + ' -c ' + confpath
      + ' --fuse_allow_other true --fuse_big_writes true --fuse_max_write 100000 --fuse_max_read 100000 --fuse_nonempty true'
      + ' --fuse_kernel_cache true'
      + ' -l ' + fsCollection + ' -d ' + metadircollection + ' -f ' + metafilecollection
      + ' -i ' + hosts + ' u "" -p "" -n 110 -s 3 -g 5 -r 2 --diagnum 20 '
      + '&';
   try
   {
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         println( command );
         throw e;
      }
   }

   // 检查结果
   checkResults( alias );
   // linux mount
   var rc = cmd.run( 'mount |grep ' + alias );
   if( rc.indexOf( alias ) === -1 || rc.indexOf( 'allow_other' ) === -1
      || rc.indexOf( 'max_read=100000' ) === -1 )
   {
      throw new Error( 'expect ' + alias[i] + ' mount, bug actual mount rc: \n' + rc );
   }

   // 清理   
   cleanFsPid( alias );
   cleanFsDir( alias );
}