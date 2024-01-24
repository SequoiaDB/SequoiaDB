/******************************************************************************
*@Description : test js object System function: buildTrusty removeTrusty
*               TestLink : 10667 System对象设置、解除信赖关系
*@author      : Liang XueWang
******************************************************************************/
// CI-2134 用例依赖环境，屏蔽这个用例
// main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   localhost = localhost["hostname"];
   var remotehost = toolGetRemotehost();
   remotehost = remotehost["hostname"];
   if( remotehost === localhost )
   {
      return;
   }

   var remote = new Remote( remotehost, CMSVCNAME );
   var system = remote.getSystem();

   // 设置信赖关系
   // 手工验证信赖关系的建立和解除,ssh时是否需要输入密码
   system.buildTrusty();
   system.removeTrusty();
   remote.close();
}


