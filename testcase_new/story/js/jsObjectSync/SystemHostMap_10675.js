/******************************************************************************
*@Description : test js object System function: getHostsMap getAHostMap
*               addAHostMap delAHostMap
*               TestLink : 10675 System对象获取hostmap信息
*                          10676 System对象获取特定主机的hostmap信息
*                          10677 System对象获取特定主机的hostmap信息，该主机不存在
*                          10678 System对象增加hostmap信息，isReplace取值为true/false
*                          10679 System对象增加hostmap信息,ip地址格式不合法               
*@author      : Liang XueWang
******************************************************************************/

// 测试获取hostsmap信息 /etc/hosts
SystemTest.prototype.testGetHostsMap = function()
{
   this.init();

   var hostsmap1 = this.system.getHostsMap().toObj().Hosts;
   var hostsmap2 = this.cmd.run( "cat /etc/hosts" ).split( "\n" );
   for( var i = 0; i < hostsmap1.length; i++ )
   {
      var ip = hostsmap1[i].Ip;
      var hostname = hostsmap1[i].HostName;
      var found = false;
      for( var j = 0; j < hostsmap2.length; j++ )
      {
         if( hostsmap2[j].indexOf( ip ) !== -1 &&
            hostsmap2[j].indexOf( hostname ) !== -1 )
         {
            found = true;
            break;
         }
      }
      if( found === false )
      {
         throw new Error( "testGetHostsMap fail,check hostmap " + this + ip + ":" + hostname + hostmap2 );
      }
   }

   this.release();
}

// 测试获取特定主机的hostmap
SystemTest.prototype.testGetAHostMap = function()
{
   this.init();

   // 测试正常getAHostMap
   var hostsmap = this.system.getHostsMap().toObj().Hosts;
   var hostname = hostsmap[0].HostName;
   var ip = hostsmap[0].Ip;
   var res = this.system.getAHostMap( hostname );
   if( res !== ip )
   {
      throw new Error( "testGetAHostMap fail,get a hostmap" + ip + res );
   }

   // 测试getAHostMap，主机不存在
   try
   {
      this.system.getAHostMap( "NotExistHost" );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   this.release();
}

// 测试增加删除hostmap信息  
SystemTest.prototype.testAddDelAHostMap = function()
{
   this.init();

   // 检查用户是否为root
   var user = this.system.getCurrentUser().toObj().user;
   if( user !== "root" )
   {
      return;
   }

   // 测试正常addAHostMap
   testAddAHostMapNormal( this.system, "testhost", "1.2.3.4" );

   // 测试addAHostMap,增加已存在的hostmap,isReplace为false
   testAddAExistHostMapFalse( this.system, "testhost", "1.2.3.5" );

   // 测试addAHostMap，增加已存在的hostmap,isReplace为true
   testAddAExistHostMapTrue( this.system, "testhost", "1.2.3.5" );

   // 测试addAHostMap,ip地址格式不合法
   testAddAHostMapIllegalIp( this.system, "tmphost", "1.2.3" );

   // 测试delAHostMap
   testDelAHostMap( this.system, "testhost" );

   this.release();
}

/******************************************************************************
*@Description : test add a hostmap normal
*@author      : Liang XueWang            
******************************************************************************/
function testAddAHostMapNormal ( system, host, ip )
{
   system.addAHostMap( host, ip );
   var result = system.getAHostMap( host );
   if( result !== ip )
   {
      throw new Error( "testAddAHostMapNormal fail,check add a hostmap" + ip + result );
   }
}

/******************************************************************************
*@Description : test add a existed hostmap with isReplace false
*@author      : Liang XueWang            
******************************************************************************/
function testAddAExistHostMapFalse ( system, host, ip )
{
   try
   {
      system.addAHostMap( host, ip, false );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}

/******************************************************************************
*@Description : test add a existed hostmap with isReplace true
*@author      : Liang XueWang            
******************************************************************************/
function testAddAExistHostMapTrue ( system, host, ip )
{
   system.addAHostMap( host, ip, true );
   var result = system.getAHostMap( host );
   if( result !== ip )
   {
      throw new Error( "testAddAExistHostMapTrue fail,check added hostmap" + ip + result );
   }
}

/******************************************************************************
*@Description : test add a hostmap with illegal ip
*@author      : Liang XueWang            
******************************************************************************/
function testAddAHostMapIllegalIp ( system, host, ip )
{
   try
   {
      system.addAHostMap( host, ip );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
}

/******************************************************************************
*@Description : test del a hostmap
*@author      : Liang XueWang            
******************************************************************************/
function testDelAHostMap ( system, host )
{
   system.delAHostMap( host );
   try
   {
      system.getAHostMap( "testhost" );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }
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
      // 测试获取hostmap
      systems[i].testGetHostsMap();

      // 测试获取特定主机的hostmap
      systems[i].testGetAHostMap();

      // 测试增加删除hostmap
      systems[i].testAddDelAHostMap();
   }
}


