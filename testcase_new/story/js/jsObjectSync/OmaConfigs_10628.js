/******************************************************************************
*@Description : test js object oma function: getOmaConfigFile getOmaConfigs 
*                                            setOmaConfigs 
*               TestLink: 10628 Oma获取Oma配置文件路径
*                         10629 Oma获取Oma配置信息
*                         10630 Oma获取Oma配置信息，文件不存在
*                         10631 Oma获取Oma配置信息，文件不是oma配置文件
*                         10632 Oma设置Oma配置信息 
*@author      : Liang XueWang
******************************************************************************/
// 测试正常获取oma配置文件和配置信息
OmaTest.prototype.testGetOmaConfigsNormal = function()
{
   this.testInit();
   var remote = new Remote( this.hostname, this.svcname );
   var cmd = remote.getCmd();

   // 测试getOmaConfigFile
   var configFile = this.oma.getOmaConfigFile();
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var found = false;
   for( var i = 0; i < sdbDir.length; i++ )
   {
      var file = sdbDir[i] + "/conf/sdbcm.conf";
      if( configFile === file )
      {
         found = true;
         break;
      }
   }
   if( found === false )
   {
      throw new Error( "testGetOmaConfigsNormal fail,get oma config file " + this + sdbDir + configFile );
   }

   // 测试getOmaConfigs
   var configs = this.oma.getOmaConfigs().toObj();
   var command = "cat " + configFile + " | tr -s '\r\n' '\n'";
   var configFileContent = cmd.run( command ).split( "\n" );
   checkResult( configs, configFileContent, "getOmaConfigs" );

   if( this.oma.close !== undefined )
      this.oma.close();
   remote.close();
}

// 测试获取oma配置信息异常
OmaTest.prototype.testGetOmaConfigsAbnormal = function()
{
   this.testInit();
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   // 测试getOmaConfigs时，文件不存在
   try
   {
      this.oma.getOmaConfigs( "/opt/notexist" );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_FNE )
      {
         throw e;
      }
   }

   // 测试getOmaConfigs时，文件不是oma配置文件
   try
   {
      this.oma.getOmaConfigs( sdbDir[0] + "/bin/sdb" );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   if( this.oma.close !== undefined )
      this.oma.close();
}

// 测试设置oma配置信息
OmaTest.prototype.testSetOmaConfigs = function()
{
   this.testInit();

   if( this.oma === Oma )
   {
      var user = System.getCurrentUser().user;
      var file = RSRVNODEDIR + "../conf/sdbcm.conf";
      var obj = getFileUsrGrp( file );
      if( user !== obj["user"] && user !== "root" )
      {
         return;
      }
   }

   try
   {
      // 测试setOmaConfigs
      var configs = this.oma.getOmaConfigs().toObj();
      configs["name"] = "lxw";
      this.oma.setOmaConfigs( configs );
      var name = this.oma.getOmaConfigs().toObj().name;
      if( name !== "lxw" )
      {
         throw new Error( "testSetOmaConfigs fail,check set oma configs " + this + "lxw" + name );
      }
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      // 测试完成后删除新加的配置
      delete configs["name"];
      this.oma.setOmaConfigs( configs );
   }

   if( this.oma === Oma )
   {
      if( user !== cmuser )
      {
         File.chown( file, obj );
      }
   }
   else
   {
      this.oma.close();
   }
}

main( test );

function test ()
{
   // 获取本地和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var staticOma = new OmaTest();

   // 执行机和测试机分开时 staticOma 可能会失败
   // var omas = [localOma, remoteOma, staticOma];
   var omas = [localOma, remoteOma];
   for( var i = 0; i < omas.length; i++ )
   {
      // 测试正常获取Oma配置文件和配置信息
      omas[i].testGetOmaConfigsNormal();

      // 测试获取Oma配置信息异常
      omas[i].testGetOmaConfigsAbnormal();

      // 测试设置Oma配置信息
      omas[i].testSetOmaConfigs();
   }
}

