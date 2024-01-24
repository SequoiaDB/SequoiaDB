/******************************************************************************
*@Description : test js object cmd function: getInfo
*               TestLink : 9893 获取Cmd对象信息
*@author      : Liang XueWang 
******************************************************************************/

// 测试获取cmd对象信息
CmdTest.prototype.testGetInfo = function()
{
   this.init();

   var info = this.cmd.getInfo().toObj();
   if( this.isLocal )  // 测试本地cmd对象信息
   {
      if( info.type !== "Cmd" || info.isRemote !== false )
      {
         throw new Error( "testGetInfo fail,get local cmd info" + this + JSON.stringify( info ) );
      }
   }
   else                // 测试远程cmd对象信息
   {
      if( info.type !== "Cmd" || info.hostname !== this.hostname ||
         info.svcname !== this.svcname || info.isRemote !== true )
      {
         throw new Error( "testGetInfo get remote cmd info" + this + JSON.stringify( info ) );
      }
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
      // 测试获取cmd对象信息
      cmds[i].testGetInfo();
   }
}

