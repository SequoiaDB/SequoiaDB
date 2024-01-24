/******************************************************************************
*@Description : test js object cmd function: run getCommand getLastRet 
*                                            getLastOut
*               seqDB-9903:执行命令后获取执行命令、执行结果、返回值
*               seqDB-13019:执行无权限的命令
*@author      : Liang XueWang 
******************************************************************************/

// 测试运行正确指令
CmdTest.prototype.testRunNormal = function()
{
   this.init();

   var result = this.cmd.run( "ls", "/tmp" );
   var command = this.cmd.getCommand();   // 获取上次执行的命令
   assert.equal( command, "ls /tmp" );
   var ret = this.cmd.getLastRet();       // 获取上次命令是否执行正常
   assert.equal( ret, 0 );
   var out = this.cmd.getLastOut();       // 获取上次命令执行的返回结果
   assert.equal( out, result );

   this.release();
}

// 测试运行错误指令
CmdTest.prototype.testRunAbnormal = function()
{
   this.init();

   try
   {
      this.cmd.run( "ledchaojiwudikuxuan", "w" );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != 127 )
      {
         throw e;
      }
   }
   var ret = this.cmd.getLastRet();
   var command = this.cmd.getCommand();
   assert.equal( command, "ledchaojiwudikuxuan w" );
   var ret = this.cmd.getLastRet();
   // TODO 
   assert.equal( ret, 127 );

   // TODO
   var out = this.cmd.getLastOut();
   if( out.indexOf( "not found" ) === -1 &&
      out.indexOf( "未找到命令" ) === -1 )
   {
      throw new Error( "testRunAbnormal fail,test getLastOut " + this + "not found" + out );
   }

   this.release();
}


// 测试运行无权限的命令useradd
CmdTest.prototype.testRunNoPermission = function()
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
      this.cmd.run( "useradd liangxw" );
      throw new Error( "show error" );
   }
   catch( e )
   {
      if( e.message == 0 )
      {
         throw new Error( "testRunNoPermission test run useradd with user " + user + " " + this + "not 0" + e );
      }
   }
   var info = this.cmd.run( "cat /etc/passwd" );
   assert.equal( info.indexOf( "liangxw" ), -1 );

   this.release();
}

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
      // 测试运行指令                                                                       
      cmds[i].testRunNormal();
      cmds[i].testRunAbnormal();
      cmds[i].testRunNoPermission();
   }
}

/**
 * 现有框架存在问题，作为遗留问题解决
 *   当将 basic_operation 下 Remote.js 中 _runCommand 和 __runCommand 使用 try catch 包裹，
 *   连接远程 Cmd 执行 getLastRet 和 getLastOut 无法返回正确的错误值（见45和48行 TODO）
 */
// main( test )