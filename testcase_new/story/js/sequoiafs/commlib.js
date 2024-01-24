/* ******************************************************************************
* @Description :  common function
* @Author : 2020/07/15 XiaoNi Huang Init
****************************************************************************** */
import( "../lib/main.js" );
import( "../lib/basic_operation/commlib.js" )

var cmd = initRemoteCmd();
var tmpFileDir = WORKDIR + '/sequoiafs/';
initTmpDir();
var installDir = commGetInstallPath();
var toolsDir = installDir + '/tools/sequoiafs/';

var fsCollection = 'sequoiafs.sequoiafs';

/********************************************************
 * @description: new Remote Cmd
 * @return: cmd
********************************************************/
function initRemoteCmd ()
{
   var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
   var rtCmd = remote.getCmd();
   return rtCmd;
}

/********************************************************
 * @description: ready tmp director
********************************************************/
function initTmpDir ()
{
   cmd.run( "mkdir -p " + tmpFileDir );
}

/********************************************************
 * @description mountpoint
 * @parameter
 *    alias e.g guestdir
 *    confDir e.g /opt/sequoiadb/tools/sequoiafs/guestdir
 *    collection e.g mountcs.mountcl
********************************************************/
function readyMountpointAndConf ( alias, collection )
{
   if( typeof ( collection ) == "undefined" ) { collection = fsCollection; }
   if( typeof ( alias ) === 'string' )
   {
      alias = [alias];
   }

   for( var i = 0; i < alias.length; i++ )
   {
      var mountpoint = tmpFileDir + alias[i];

      // 创建挂载目录
      cmd.run( 'mkdir -p ' + mountpoint );

      // 创建配置文件
      confDir = toolsDir + 'conf/local/' + alias[i] + '/';
      cmd.run( 'mkdir -p ' + confDir );
      var confSampleDir = toolsDir + 'conf/sample/sequoiafs.conf';
      var confDir = confDir + 'sequoiafs.conf';
      cmd.run( 'cp ' + confSampleDir + ' ' + confDir );
      // 配置sequoiafs.conf
      cmd.run( 'sed -i "s%^mountpoint=%mountpoint=' + mountpoint + '%g" ' + confDir );
      cmd.run( 'sed -i "s%^alias=%alias=' + alias[i] + '%g" ' + confDir );
      cmd.run( 'sed -i "s%^collection=%collection=' + collection + '%g" ' + confDir );
   }
}

/********************************************************
 * @description: sequoiafs挂载节点是后台执行
 *    在cmd没有返回值，会卡住，需要设置超时时间
********************************************************/
function fsstart ( param, timeout )
{
   if( typeof ( timeout ) == "undefined" ) { timeout = 1000; }
   try
   {
      var rc = cmd.run( toolsDir + 'bin/fsstart.sh ' + param, "", timeout );
      return rc;
   }
   catch( e )
   {
      if( e.message != SDB_TIMEOUT )
      {
         throw e;
      }
   }
}

function fsstop ( param )
{
   cmd.run( toolsDir + 'bin/fsstop.sh ' + param );
}

function fslist ( param )
{
   if( typeof ( param ) == "undefined" ) { param = '-m run'; }
   var rc = cmd.run( toolsDir + 'bin/fslist.sh ' + param );
   return rc;
}

/********************************************************
 * @description: 
 *    1、flist先检查挂载目录正确性
 *    2、linux系统命令mount检查挂载目录正确性
 *    3、已挂载目录下写文件，检查挂载目录可用
********************************************************/
function checkResults ( alias, isMount )
{
   if( typeof ( isMount ) == "undefined" ) { isMount = true; }
   if( typeof ( alias ) === 'string' )
   {
      alias = [alias];
   }

   if( isMount )
   {
      for( var i = 0; i < alias.length; i++ )
      {
         // fslist
         // SEQUOIADBMAINSTREAM-6086, fslist在ubuntu会coredump，暂时屏蔽该方法
         /*
         var rc = fslist();
         if( rc.indexOf( alias[i] ) === -1 )
         {
            throw new Error( 'expect ' + alias[i] + ' mount, bug actual fslist.sh rc: \n' + rc );
         }
         */

         // linux mount
         var rc = cmd.run( 'mount |grep ' + alias[i] );
         if( rc.indexOf( alias[i] ) === -1 )
         {
            throw new Error( 'expect ' + alias[i] + ' mount, bug actual mount rc: \n' + rc );
         }

         // write file
         var mountpoint = tmpFileDir + alias[i];
         var fileDir = mountpoint + '/' + alias[i] + 'txt';
         cmd.run( 'echo sequoiafs > ' + fileDir );
         var rc = cmd.run( 'cat ' + fileDir );
         if( rc.indexOf( 'sequoiafs' ) === -1 )
         {
            throw new Error( 'Failed to write file in the ' + mountpoint );
         }
      }
   }
   else
   {
      for( var i = 0; i < alias.length; i++ )
      {
         // fslist
         // SEQUOIADBMAINSTREAM-6086, fslist在ubuntu会coredump，暂时屏蔽该方法
         /*
         try
         {
            var rc = fslist( '-m run' );
            if( rc.indexOf( alias[i] ) !== -1 )
            {
               throw new Error( 'expect ' + alias[i] + ' unmount, bug actual fslist.sh rc: \n' + rc );
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
            var rc = cmd.run( 'mount |grep ' + alias[i] );
            if( rc.indexOf( alias[i] ) !== -1 )
            {
               throw new Error( 'expect ' + alias[i] + ' unmount, bug actual mount rc: \n' + rc );
            }
         }
         catch( e )
         {
            if( e.message !== '1' ) // linux rc: 1
            {
               throw e;
            }
         }
      }
   }
}

/********************************************************
 * @description: clean fs pid
********************************************************/
function cleanFsPid ( alias )
{
   if( typeof ( alias ) === 'string' )
   {
      alias = [alias];
   }

   for( var i = 0; i < alias.length; i++ )
   {
      fsstop( '--alias ' + alias[i] );
   }
}

/********************************************************
 * @description: clean dir
********************************************************/
function cleanFsDir ( aliasSubStr, confDir )
{
   if( typeof ( confDir ) == "undefined" ) { confDir = toolsDir + 'conf/local/' + aliasSubStr + '*'; }
   var mountpoints = tmpFileDir + aliasSubStr + '*';
   cmd.run( "rm -rf " + confDir );
   cmd.run( "rm -rf " + mountpoints );
}
