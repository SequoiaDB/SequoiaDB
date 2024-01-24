/******************************************************************************
*@Description : test js object cmd function: start
*               seqDB-9905:后台执行命令
*               seqDB-13020:后台执行无权限命令 
*@author      : Liang XueWang 
******************************************************************************/

// 测试后台运行命令
CmdTest.prototype.testStart = function()
{
   this.init();

   var pid = this.cmd.start( "sleep", "3" );
   var command = "ps aux | awk '{print $2}' | grep " + pid;
   var tasks = this.cmd.run( command ).split( "\n" );
   var found = false;
   for( var i = 0; i < tasks.length - 1; i++ )
   {
      if( tasks[i] === "" + pid )
      {
         found = true;
         break;
      }
   }
   assert.equal( found, true );

   this.release();
}

// 测试后台运行错误命令
CmdTest.prototype.testStartAbnormal = function()
{
   this.init();

   try
   {
      this.cmd.start( "led", ".", 1, 3 * 1000 );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message == 0 )
      {
         throw e;
      }
   }

   this.release();
}

// 测试后台运行无权限命令
CmdTest.prototype.testStartNoPermission = function()
{
   this.init();

   var user = this.cmd.run( "whoami" ).split( "\n" )[0];
   if( user === "root" )
   {
      this.release();
      return;
   }
   try
   {
      this.cmd.start( "groupadd", "liangxw", 1, 3 * 1000 );
   }
   catch( e )
   {
      println( "test start groupadd with user " + user + " " + this + "," + "catch exception " + e );
   }

   var info = this.cmd.run( "cat /etc/group" );
   assert.equal( info.indexOf( "liangxw" ), -1 );

   this.release();
}

main( test );

function test ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localCmd = new CmdTest( localhost, CMSVCNAME );
   var remoteCmd = new CmdTest( remotehost, CMSVCNAME );
   var cmds = [localCmd, remoteCmd];

   for( var i = 0; i < cmds.length; i++ )
   {
      // 测试后台运行指令
      cmds[i].testStart();
      cmds[i].testStartAbnormal();
      cmds[i].testStartNoPermission();
   }
}

