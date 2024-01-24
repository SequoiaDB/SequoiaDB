/******************************************************************************
*@Description : test js object Remote initialization
*               TestLink : 10636 初始化Remote对象，端口不存在
*                          10637 初始化Remote对象，端口不是cm端口
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   // 获取远程主机
   var remotehost = toolGetRemotehost();

   // 测试使用不存在的主机初始化
   var obj = { "hostname": "IllegalHost", "isLocal": true };
   var rt = new RemoteTest( obj, CMSVCNAME, false, true );
   rt.testInit();

   // 获取空闲端口
   var svcname = toolGetIdleSvcName( remotehost["hostname"], CMSVCNAME );
   if( svcname === undefined )
   {
      return;
   }

   // 测试使用不存在的端口初始化
   rt = new RemoteTest( remotehost, svcname, true, false );
   rt.testInit();

   // 测试使用非cm端口初始化
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      new Remote( remotehost, COORDSVCNAME );
   } );
}


