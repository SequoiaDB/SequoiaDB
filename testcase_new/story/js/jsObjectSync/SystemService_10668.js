/******************************************************************************
 * @Description   : seqDB-10668:System对象调用runService启动、停止服务如ssh服务
 * @Author        : Liang Xuewang
 * @CreateTime    : 2016.12.27
 * @LastEditTime  : 2022.09.02
 * @LastEditors   : Lin Yingting
 ******************************************************************************/
// 测试启动停止服务，获取服务状态（SSH服务）
SystemTest.prototype.testRunServiceSSH = function()
{
   this.init();

   // 检查cm用户是否为root
   var user = this.system.getCurrentUser().toObj().user;
   if( user !== "root" )
   {
      println( "Skip this test, it requires root user privilege." );
      return;
   }
   // 检查是否存在ssh服务
   if( !isSSHExist( this.hostname, this.svcname ) )
   {
      println( "Skip this test, it requires ssh service." );
      return;
   }
   // 获取服务状态
   try
   {
      var info = this.system.runService( "sshd", "status", "" );
   }
   catch( e )
   {
      if( e == 3 )
      {
         info = "dead";
      } 
   }
   if( info.indexOf( "running" ) !== -1 )   // 如果服务已启动，则停止再启动服务
   {
      var status;
      try
      {
         this.system.runService( "sshd", "stop", "" );
         status = this.system.runService( "sshd", "status" );
      }
      catch( e )
      {
         if( e.message != 3 )
         {
            throw e;
         }
      }
      finally
      {
         this.system.runService( "sshd", "start", "" );
         status = this.system.runService( "sshd", "status" );
         checkStatus( status, "running" );
      }
   }
   else if( info.indexOf( "dead" ) !== -1 )  // 如果服务已停止，则启动服务
   {
      this.system.runService( "sshd", "start", "" );
      var status = this.system.runService( "sshd", "status" );
      checkStatus( status, "running" );
   }
   else
   {
      throw new Error( "The ssh service status of runService is expected as start/stop, but result is: " + info );
   }
   this.release();
}
/******************************************************************************
*@Description : check service status after stop/start service
*@author      : Liang XueWang            
******************************************************************************/
function checkStatus ( status, msg )
{
   if( status.indexOf( msg ) === -1 )
   {
      throw new Error( "The status of ssh service is not as expected." );
   }
}

/******************************************************************************
*@Description : check ssh service exist or not
*@author      : Liang XueWang            
******************************************************************************/
function isSSHExist ( hostname, svcname )
{
   var remote = new Remote( hostname, svcname );
   var cmd = remote.getCmd();
   var exist = true;
   try
   {
      cmd.run( "ssh -V" );
   }
   catch( e )
   {
      exist = false;
   }
   remote.close();
   return exist;
}

main( test );

function test ()
{
   // 获取本地主机，测试启动停止ssh服务
   var localhost = toolGetLocalhost();
   var localSystem = new SystemTest( localhost, CMSVCNAME );
   localSystem.testRunServiceSSH();   
}
