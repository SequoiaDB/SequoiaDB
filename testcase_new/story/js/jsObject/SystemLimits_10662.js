/******************************************************************************
*@Description : test js object System function: getProcUlimitConfigs 
*                                               setProcUlimitConfigs
*               TestLink : 10662 System对象获取limits配置信息
*                          10663 System对象设置limits信息
*@author      : Liang XueWang
******************************************************************************/

// 测试获取limits信息
SystemTest.prototype.testGetProcUlimitConfigs = function()
{
   this.init();

   var limits = this.system.getProcUlimitConfigs().toObj();
   for( var k in limits )
   {
      var str = k.replace( /_/g, " " );
      if( str === "realtime priority" )
         str = "real-time priority";
      var command = "/bin/bash -c 'ulimit -a' | grep " + "'^" + str + "'";
      var infos = this.cmd.run( command ).split( "\n" );
      var info = ""
      for( var i in infos )
      {
         if( infos[i].indexOf( str ) !== -1 )
         {
            info = infos[i]
         }
      }
      var limit = info.split(") ")[1];
      if( limit === "unlimited" )
         limit = -1;
      else if( info.indexOf( "kbytes" ) !== -1 )
         limit = 1024 * limit;
      else if( info.indexOf( "blocks" ) !== -1 )
         limit = 1024 * limit;
      assert.equal( limits[k], limit * 1, command );
   }

   this.release();
}

// 测试设置limits信息边界值
SystemTest.prototype.testSetProcUlimitConfigs = function()
{
   this.init();

   var oldLimits = this.system.getProcUlimitConfigs().toObj();
   var oldMaxMemorySize = oldLimits.max_memory_size;

   var maxMemSize = [9007199254740992, -9007199254740992,
      "18446744073709551614", "-18446744073709551615",
      "18446744073709551615"];
   var errMemSize = ["18446744073709551616", "-18446744073709551616"];
   var results = ["9007199254740992", "18437736874454810624",
      "18446744073709551614", 1, -1];
   for( var i = 0; i < maxMemSize.length; i++ )
   {
      oldLimits.max_memory_size = maxMemSize[i];
      this.system.setProcUlimitConfigs( oldLimits );
      var newLimits = this.system.getProcUlimitConfigs().toObj();
      assert.equal( newLimits.max_memory_size, results[i] );

   }
   for( var i = 0; i < errMemSize; i++ )
   {
      try
      {
         oldLimits.max_memory_size = errMemSize[i];
         this.system.setProcUlimitConfigs( oldLimits );
         throw new Error( "should error" );
      } catch( e )
      {
         if( e.message != SDB_INVALIDARG )
         {
            throw e;
         }
      }
   }

   oldLimits.max_memory_size = oldMaxMemorySize;
   this.system.setProcUlimitConfigs( oldLimits );

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

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试获取limits
      systems[i].testGetProcUlimitConfigs();

      // 测试设置limits
      systems[i].testSetProcUlimitConfigs();
   }

}


