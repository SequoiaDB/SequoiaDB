/******************************************************************************
*@Description : test js object Remote function : close
*               TestLink : 10638 关闭Remote对象后，继续执行操作
*@author      : Liang XueWang
******************************************************************************/

// 测试remote关闭后执行操作
RemoteTest.prototype.testClose = function()
{
   this.testInit();
   var system = this.remote.getSystem();
   this.remote.close();

   assert.tryThrow( SDB_NETWORK, function()
   {
      system.type();
   } );
}

main( test );

function test ()
{
   // 获取远程主机
   var remotehost = toolGetRemotehost();

   // 测试remote关闭后执行操作
   var rt = new RemoteTest( remotehost, CMSVCNAME );
   rt.testClose();
}

