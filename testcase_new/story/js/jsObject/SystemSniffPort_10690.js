/******************************************************************************
*@Description : test js object System function: sniffPort
*               TestLink : 10690 System对象获取指定端口状态               
*@author      : Liang XueWang
******************************************************************************/

SystemTest.prototype.testSniffPort = function( svcname )
{
   this.init();

   // 测试已使用端口的状态
   var useable = this.system.sniffPort( COORDSVCNAME * 1 ).toObj().Usable;
   assert.equal( useable, false );

   // 测试未使用端口的状态
   useable = this.system.sniffPort( svcname * 1 ).toObj().Usable;
   assert.equal( useable, true );

   this.release();
}

SystemTest.prototype.testSniffPortBoundary = function()
{
   this.init();

   var ErrorPort = [0, 65536, "0", "65536"];
   var CorrePort = [1, 65535, "1", "65535"];  // 1是保留端口，普通用户不能占用
   var user = this.system.getCurrentUser().toObj().user;
   var result;
   if( user === "root" )
      result = [true, true, true, true];
   else
      result = [false, true, false, true];

   for( var i = 0; i < ErrorPort.length; i++ )
   {
      try
      {
         this.system.sniffPort( ErrorPort[i] );
         throw new Error( "should error" );
      } catch( e )
      {
         if( e.message != SDB_INVALIDARG )
         {
            throw e;
         }
      }
   }
   for( var i = 0; i < CorrePort.length; i++ )
   {
      var useable = this.system.sniffPort( CorrePort[i] ).toObj().Usable;
      assert.equal( useable, result[i] );
   }

   this.release();
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   // 获取空闲端口号
   var svcname1 = toolGetIdleSvcName( localhost["hostname"], CMSVCNAME );
   var svcname2 = toolGetIdleSvcName( remotehost["hostname"], CMSVCNAME );
   var svcnames = [svcname1, svcname2];

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试端口状态
      systems[i].testSniffPort( svcnames[i] );
      // 测试端口状态，端口号为边界值
      systems[i].testSniffPortBoundary();
   }
}


