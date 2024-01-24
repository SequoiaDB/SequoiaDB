/******************************************************************************
*@Description : test js object System function: getPID getTID getEWD
*               TestLink : 10641 System对象获取sdb的进程ID
*                          10642 System对象获取sdb的线程ID
*                          10643 System对象获取sdb可执行程序的路径          
*@author      : Liang XueWang
******************************************************************************/

// 测试获取sdb shell进程ID
SystemTest.prototype.testGetPID = function()
{
   this.init();

   var pid = this.system.getPID();
   var command;
   if( this.system === System )
      command = "pgrep '^sdb$'";
   else
      command = "pgrep '^sdbcm$'";
   var pids = this.cmd.run( command );
   if( pids.indexOf( pid ) === -1 )
   {
      throw new Error( "testGetPID fail,get PID " + this + pids + pid );
   }

   this.release();
}

// 测试获取sdb shell线程ID
SystemTest.prototype.testGetTID = function()
{
   this.init();

   var pid = this.system.getPID();
   var tid = this.system.getTID();
   var command = "ps -T -p " + pid + " | awk '{print $2}'";
   var tids = this.cmd.run( command ).split( "\n" );
   if( tids.indexOf( "" + tid ) === -1 )
   {
      throw new Error( "testGetTID fail,get TID " + this + tid + tids );
   }

   this.release();
}

// 测试获取sdb所在的工作目录   
SystemTest.prototype.testGetEWD = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var WorkDir = this.system.getEWD();
   var found = false;
   for( var i = 0; i < sdbDir.length; i++ )
   {
      if( sdbDir[i] + "/bin" === WorkDir )
      {
         found = true;
         break;
      }
   }
   if( found === false )
   {
      throw new Error( "testGetEWD fail,get EWD " + this + sdbDir + WorkDir );
   }

   this.release();
}

main( test );

function test ()
{
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试获取sdb进程ID
      systems[i].testGetPID();

      // 测试获取sdb线程ID
      systems[i].testGetTID();

      // 测试获取sdb目录
      systems[i].testGetEWD();
   }
}

