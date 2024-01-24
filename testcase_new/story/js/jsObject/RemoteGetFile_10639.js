/******************************************************************************
*@Description : test js object Remote function : get file
*               TestLink : 10639 初始化Remote对象，获取File对象，文件无权限
*                          10640 初始化Remote对象，获取File对象，文件为正在
*                                使用的sdb可执行文件
*@author      : Liang XueWang
******************************************************************************/

// 测试获取无权限文件
RemoteTest.prototype.testGetNoPermitFile = function()
{
   this.testInit();
   var user = toolGetSdbcmUser( this.hostname, this.svcname );
   if( user === "root" ) return;
   // make no permission dir
   var file = this.remote.getFile();
   var dirName = "/tmp/noPerDir/";
   file.mkdir( dirName );
   file.chmod( dirName, 0000 );

   try
   {
      this.remote.getFile( dirName + "/test" );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_PERM )
      {
         throw e;
      }
   }

   var cmd = this.remote.getCmd();
   cmd.run( "rm -rf " + dirName );
   this.remote.close();
}

// 测试获取sdb可执行文件
RemoteTest.prototype.testGetSdbFile = function( hostname )
{
   this.testInit();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   if( this.hostname === hostname )
   {
      try
      {
         this.remote.getFile( sdbDir[0] + "/bin/sdb" );
         throw new Error( "should error" );
      } catch( e )
      {
         if( e.message != SDB_IO )
         {
            throw e;
         }
      }
   }
   else
   {
      this.remote.getFile( sdbDir[0] + "/bin/sdb" );
   }

   this.remote.close();
}

main( test );

function test ()
{
   // 获取远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   // 测试获取无权限的文件
   var remote1 = new RemoteTest( localhost, CMSVCNAME );
   var remote2 = new RemoteTest( remotehost, CMSVCNAME );
   remote1.testGetNoPermitFile();
   remote2.testGetNoPermitFile();

   // 测试获取sdb文件（当前正在使用的文件，手工验证）
   // rt1.testGetSdbFile( localhost ) ;
   // rt2.testGetSdbFile( localhost ) ;
}

