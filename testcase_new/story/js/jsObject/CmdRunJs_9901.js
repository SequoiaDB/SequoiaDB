/******************************************************************************
*@Description : test js object cmd function: runJS
*               TestLink : 9901 执行JS语句 
*@author      : Liang XueWang 
******************************************************************************/

// 测试运行JS命令
CmdTest.prototype.testRunJS = function()
{
   this.init();

   try
   {
      var result = this.cmd.runJS( "1 + 2 * 3" );
   }
   catch( e )
   {
      if( e.message == SDB_SYS && this.isLocal )
         ;
      else
         throw e;
   }
   if( ( this.isLocal && result !== undefined ) ||
      ( !this.isLocal && result !== "7" ) )
   {
      throw e;
   }

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
      cmds[i].testRunJS();
   }
}

