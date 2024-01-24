/************************************
*@Description: seqDB-22241:指定--alias挂载目录
*              seqDB-22250:指定--alias卸载目录
*@Author:      2020/07/21  liuli
**************************************/

// main( test );
function test ()
{
   // aliasArr为挂载alias
   var aliasArr = ['asequoiafs_22241_01', 'bsequoiafs_22241_01', 'csequoiafs_22241_01',
      'sequoiafs_22241_02', 'sequoiafs_22241_03'];
   // mountPointArr为挂载alias前一级的目录
   var mountPointArr = ['sequoiafs_22241_020/', 'sequoiafs_22241_021/', 'sequoiafs_22241_022/',
      'sequoiafs_22241_03/sequoiafs_22241_03/'];

   // 清理
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22241' );

   // 准备多个后缀相同的挂载目录和配置文件,如：/opt/sequoiadb/atest、/opt/sequoiadb/btest
   readyMountpointAndConf( aliasArr[0] );
   readyMountpointAndConf( aliasArr[1] );
   readyMountpointAndConf( aliasArr[2] );
   // 准备alias存在多个不同路径下，如/opt/sequoiadb/fstest1/test、/opt/sequoiadb/fstest2/test
   readyMountpointAndConf( aliasArr[3], mountPointArr[0] );
   readyMountpointAndConf( aliasArr[3], mountPointArr[1] );
   readyMountpointAndConf( aliasArr[3], mountPointArr[2] );
   // 准备多级同名alias目录和配置文件,如/opt/sequoiadb/fstest/fstest/fstest
   readyMountpointAndConf( aliasArr[4], mountPointArr[3] );

   // 挂载后缀相同的alias
   fsstart( '--alias ' + aliasArr[0] );
   fsstart( '--alias ' + aliasArr[1] );
   fsstart( '--alias ' + aliasArr[2] );
   checkResults( aliasArr[0] );
   checkResults( aliasArr[1] );
   checkResults( aliasArr[2] );

   // 卸载其中一个目录
   fsstop( '--alias ' + aliasArr[0] );

   // 卸载不存在的目录
   fsstop( '--alias ' + 'dsequoiafs_22241' );

   // 挂载alias存在多个不同路径下
   fsstart( '--alias ' + aliasArr[3] + ' -c ' + toolsDir + 'conf/local/' + mountPointArr[0] + aliasArr[3] );
   fsstart( '--alias ' + aliasArr[3] + ' -c ' + toolsDir + 'conf/local/' + mountPointArr[1] + aliasArr[3] );
   fsstart( '--alias ' + aliasArr[3] + ' -c ' + toolsDir + 'conf/local/' + mountPointArr[2] + aliasArr[3] );
   checkResults( aliasArr[3], mountPointArr[0] );
   checkResults( aliasArr[3], mountPointArr[1] );
   checkResults( aliasArr[3], mountPointArr[2] );

   // 挂载存在多级同名alias目录
   fsstart( '--alias ' + aliasArr[4] + ' -c ' + toolsDir + 'conf/local/' + mountPointArr[3] + aliasArr[4] );
   checkResults( aliasArr[4], mountPointArr[3] );

   // 清理
   cleanFsPid( aliasArr );
   cleanFsDir( 'sequoiafs_22241' );
}

function readyMountpointAndConf ( alias, mountPoint )
{

   collection = fsCollection;
   var mountpoint = tmpFileDir + mountPoint + alias;

   // 创建挂载目录
   cmd.run( 'mkdir -p ' + mountpoint );

   // 创建配置文件
   confDir = toolsDir + 'conf/local/' + mountPoint + alias + '/';
   cmd.run( 'mkdir -p ' + confDir );
   var confSampleDir = toolsDir + 'conf/sample/sequoiafs.conf';
   var confDir = confDir + 'sequoiafs.conf';
   cmd.run( 'cp ' + confSampleDir + ' ' + confDir );
   // 配置sequoiafs.conf
   cmd.run( 'sed -i "s%^mountpoint=%mountpoint=' + mountpoint + '%g" ' + confDir );
   cmd.run( 'sed -i "s%^alias=%alias=' + alias + '%g" ' + confDir );
   cmd.run( 'sed -i "s%^collection=%collection=' + collection + '%g" ' + confDir );
}

function checkResults ( alias, mountPoint, isMount )
{
   if( typeof ( isMount ) == "undefined" ) { isMount = true; }

   if( isMount )
   {
      // fslist
      // SEQUOIADBMAINSTREAM-6086, fslist在ubuntu会coredump，暂时屏蔽该方法
      /*
      var rc = fslist();
      if( rc.indexOf( alias ) === -1 )
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
      var mountpoint = tmpFileDir + mountPoint + alias;
      var fileDir = mountpoint + '/' + alias + 'txt';
      cmd.run( 'echo sequoiafs > ' + fileDir );
      var rc = cmd.run( 'cat ' + fileDir );
      if( rc.indexOf( 'sequoiafs' ) === -1 )
      {
         throw new Error( 'Failed to write file in the ' + mountpoint );
      }
   }
   else
   {
      // fslist
      // SEQUOIADBMAINSTREAM-6086, fslist在ubuntu会coredump，暂时屏蔽该方法
      /*
      try
      {
         var rc = fslist( '-m run' );
         if( rc.indexOf( alias ) !== -1 )
         {
            throw new Error( 'expect ' + alias + ' unmount, bug actual fslist.sh rc: \n' + rc );
         }
      }
      catch( e )
      {
         if( e.message !== '1' ) // linux rc: "Error: 1"
         {
            throw e;
         }
      }
      */

      // linux mount
      try
      {
         var rc = cmd.run( 'mount |grep ' + alias );
         if( rc.indexOf( alias ) !== -1 )
         {
            throw new Error( 'expect ' + alias + ' unmount, bug actual mount rc: \n' + rc );
         }
      }
      catch( e )
      {
         if( e.message != 1 ) // linux rc: 1
         {
            throw e;
         }
      }
   }
}