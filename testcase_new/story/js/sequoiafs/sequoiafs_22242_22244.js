/************************************
*@Description: seqDB-22242:指定-c/--confpath挂载目录
*              seqDB-22244:同一个confpath多次挂载
*@Author:      2020/07/22  liuli
**************************************/

// main( test );
function test ()
{
   var aliasArr = ['sequoiafs_22242_01', 'sequoiafs_22242_02']

   // 清理
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22242' );

   // 准备挂载目录和配置文件
   readyMountpointAndConf( aliasArr[0] );
   // 使用-c/--confpath挂载目录
   fsstart( '-c ' + toolsDir + 'conf/local/' + aliasArr[0] );
   checkResults( aliasArr[0] );
   // 重复挂载该目录
   fsstart( '-c ' + toolsDir + 'conf/local/' + aliasArr[0] );

   checkResults( aliasArr[0] );
   // 卸载目录，并重新挂载
   fsstop( '--alias ' + aliasArr[0] );
   fsstart( '-c ' + toolsDir + 'conf/local/' + aliasArr[0] );
   checkResults( aliasArr[0] );

   // 删除aliasArr[0]的配置文件
   fsstop( '--alias ' + aliasArr[0] );
   checkResults( aliasArr[0], false );
   cmd.run( 'rm -rf ' + toolsDir + 'conf/local/' + aliasArr[0] + '/sequoiafs.conf' );
   // 指定collection/mountpoin挂载目录
   var mountpoint = tmpFileDir + aliasArr[0];
   var confpath = toolsDir + 'conf/local/' + aliasArr[0];
   var command = toolsDir + 'bin/sequoiafs -c ' + confpath + ' -m ' + mountpoint + ' -l ' + fsCollection;
   try
   {
      cmd.run( command, '', 1000 );
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         throw e;
      }
   }
   checkResults( aliasArr[0] );

   // 准备挂载目录和配置文件，不使用默认路径
   readyMountpointAndConf( aliasArr[1], tmpFileDir + aliasArr[1] );
   fsstart( '-c ' + '/opt/sequoiadb/sequoiafs/conf/local/' + aliasArr[1] );
   checkResults( aliasArr[1] );

   // 清理
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22242' );
}

function readyMountpointAndConf ( alias, confDir )
{
   if( typeof ( confDir ) == "undefined" ) { confDir = toolsDir + 'conf/local/' + alias + '/' }

   var mountpoint = tmpFileDir + alias;

   // 创建挂载目录
   cmd.run( 'mkdir -p ' + mountpoint );

   // 创建配置文件
   collection = fsCollection;
   cmd.run( 'mkdir -p ' + confDir );
   var confSampleDir = installDir + '/tools/sequoiafs/' + 'conf/sample/sequoiafs.conf';
   var confDir = confDir + 'sequoiafs.conf';
   cmd.run( 'cp ' + confSampleDir + ' ' + confDir );
   // 配置sequoiafs.conf
   cmd.run( 'sed -i "s%^mountpoint=%mountpoint=' + mountpoint + '%g" ' + confDir );
   cmd.run( 'sed -i "s%^alias=%alias=' + alias + '%g" ' + confDir );
   cmd.run( 'sed -i "s%^collection=%collection=' + collection + '%g" ' + confDir );
}
