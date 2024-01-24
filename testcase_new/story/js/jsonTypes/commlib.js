/******************************************************************************
*@Description : common function for lob import/export/migration tool
*               the testcase about user/password cannot run in concurrent.
*@Modify list :
*               2016-06-20  XueWang Liang  Change
******************************************************************************/
import( "../lib/basic_operation/commlib.js" );
import( "../lib/main.js" );

var hostName = COORDHOSTNAME; // 主机名：'localhost'
var coordPort = COORDSVCNAME; // 端口号：11810
var user = null; // 用户名：
var passwd = null; // 密码：
var db = new Sdb( hostName, coordPort, user, passwd );
var cmd = new Cmd();
var LocalPath = null; // 当前目录
var InstallPath = null; // sequoiadb安装目录

/******************************************************************************
*@Description : initalize the global variable in the begninning.
初始化全局变量LocalPath、InstallPath
******************************************************************************/
function initPath ()
{
   var local = cmd.run( "pwd" ).split( "\n" ); //获得当前目录, cmd.run()方法返回结果会在后面加入一空行
   LocalPath = local[0];
   try
   {
      var install = cmd.run( "grep INSTALL_DIR  /etc/default/sequoiadb" ).split( "=" ); // 命令返回结果为 INSTALL_DIR = /opt/sequoiadb
      var installPath = install[1].split( "\n" );
      InstallPath = installPath[0]; //获得默认安装目录 /opt/sequoiadb
   }
   catch( e )
   {
      if( 2 == e.message )//在sequoiadb shell中运行返回错误0 uncaught exception: 2
         InstallPath = toolGetInstallPath( LocalPath ); // 检验当前目录是否为安装目录
      else
         throw e;
   }
}

/******************************************************************************
*@Description : when run these testcase in sequoiadb or trunk fold that not
*               installed, get home fold. < ?how to get sequoiadb home fold? > 
******************************************************************************/
function toolGetInstallPath ( localPath )
{
   var folder = cmd.run( 'ls ' + localPath ).split( '\n' );
   var fcnt = 0;
   for( var i = 0; i < folder.length; ++i )
   {
      if( "bin" == folder[i] || "SequoiaDB" == folder[i] || "testcase" == folder[i] )
      {
         fcnt++;
      }
   }
   if( 2 <= fcnt )
      InstallPath = localPath;
   return InstallPath;
}

/******************************************************************************
*@Description : check result with expect results
*               find, .....
******************************************************************************/
function checkRec ( rc, expRecs )
{
   //get actual records to array
   var actRecs = [];
   while( rc.next() )
   {
      actRecs.push( rc.current().toObj() );
   }

   //check count
   assert.equal( actRecs.length, expRecs.length )

   //check every records every fields
   for( var i in expRecs )
   {
      var actRec = actRecs[i];
      var expRec = expRecs[i];
      for( var f in expRec )
      {
         assert.equal( actRec[f], expRec[f] );
      }
   }
}
