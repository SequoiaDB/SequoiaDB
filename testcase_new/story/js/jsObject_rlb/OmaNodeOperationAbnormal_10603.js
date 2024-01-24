/******************************************************************************
*@Description : test js object oma function: createCoord removeCoord createData
*                              Abnormal      removeData startNode stopNode close             
*               TestLink: 10603 Oma创建节点，端口已存在
*                         10604 Oma创建节点，文件路径无权限                 
*                         10607 Oma创建节点，节点配置信息错误
*                         10608 Oma创建节点，添加不存在的配置项
*                         10609 Oma删除节点，端口号不存在
*                         10610 Oma删除节点，端口号类型不匹配
*                         10611 Oma删除节点，节点配置信息不正确
*                         10614 Oma启动节点，节点不存在 
*                         10615 Oma停止节点，节点不存在
*                         10616 关闭Oma对象
*@author      : Liang XueWang               
******************************************************************************/
// 测试创建已存在的节点
OmaTest.prototype.testCreateExistCoord = function()
{
   this.testInit();

   try
   {
      var svcname = COORDSVCNAME;
      var dbpath = RSRVNODEDIR + svcname;
      this.oma.createCoord( svcname, dbpath );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDBCM_NODE_EXISTED )
      {
         throw e;
      }
   }

   this.oma.close();
}

// 测试删除不存在的节点
OmaTest.prototype.testRemoveNotExistCoord = function( svcname )
{
   this.testInit();
   try
   {
      this.oma.removeCoord( svcname );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
   this.oma.close();
}

// 测试删除节点时使用错误端口
OmaTest.prototype.testRemoveCoordWithWrongSvc = function()
{
   this.testInit();
   try
   {
      this.oma.removeCoord( CMSVCNAME );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
   this.oma.close();
}

// 测试删除节点时使用错误配置项
OmaTest.prototype.testRemoveCoordWithWrongConf = function()
{
   this.testInit();

   if( this.isStandalone )
   {
      return;
   }

   try
   {
      this.oma.removeCoord( COORDSVCNAME, { clustername: "!@#$%^&*" } );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
   this.oma.close();
}

// 测试启动不存在的节点
OmaTest.prototype.testStartNotExistNode = function( svcname )
{
   this.testInit();
   try
   {
      this.oma.startNode( svcname );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDBCM_NODE_NOTEXISTED )
      {
         throw e;
      }
   }
   this.oma.close();
}

// 测试停止不存在的节点
OmaTest.prototype.testStopNotExistNode = function( svcname )
{
   this.testInit();
   this.oma.stopNode( svcname );
   this.oma.close();
}

// 测试创建节点时使用错误配置项
OmaTest.prototype.testCreateCoordWithWrongConf = function( svcname )
{
   var isCoordCreated = false;
   this.testInit();
   try
   {
      var dbpath = RSRVNODEDIR + svcname;
      // 不正确的配置项 role: om 配置文件自动修改为 role: coord
      this.oma.createCoord( svcname, dbpath, { role: "om" } );
      isCoordCreated = true;
      this.oma.startNode( svcname );
      this.oma.stopNode( svcname );
      this.oma.removeCoord( svcname );
      isCoordCreated = false;
      // 不存在的配置项 abc: 123 写入配置文件
      this.oma.createCoord( svcname, dbpath, { abc: "123" } );
      isCoordCreated = true;
      this.oma.startNode( svcname );
      this.oma.stopNode( svcname );
      this.oma.removeCoord( svcname );
      isCoordCreated = false;
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( isCoordCreated )
      {
         this.oma.removeCoord( svcname );
      }
      this.oma.close();
   }
}

// 测试创建节点时节点目录无权限
OmaTest.prototype.testCreateCoordWithNoPermit = function( svcname )
{
   this.testInit();

   var remote = new Remote( this.hostname, this.svcname );
   var system = remote.getSystem();
   user = system.getCurrentUser().toObj().user;
   if( user === "root" ) return;

   // make no permission dir
   var file = remote.getFile();
   var dirName = "/tmp/noPerDir/";
   file.mkdir( dirName );
   file.chmod( dirName, 0000 );

   try
   {
      this.oma.createCoord( svcname, dirName + svcname );
      throw new Error( "should error" );
   }
   catch( e )
   {
      if( e.message != SDB_PERM )
      {
         throw e;
      }
   }

   file.chmod( dirName, 0755 );
   file.remove( dirName );
   remote.close();
   this.oma.close();
}

// 测试oma关闭后执行操作
OmaTest.prototype.testOmaClose = function()
{
   this.testInit();
   this.oma.close();
   try
   {
      this.oma.stopNode( COORDSVCNAME );
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
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   // 获取本地和远程空闲的端口号
   var svcname1 = toolGetIdleSvcName( localhost["hostname"], CMSVCNAME );
   if( svcname1 === undefined )
   {
      return;
   }
   var svcname2 = toolGetIdleSvcName( remotehost["hostname"], CMSVCNAME );
   if( svcname2 === undefined )
   {
      return;
   }

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var omas = [localOma, remoteOma];
   var svcnames = [svcname1, svcname2];

   for( i = 0; i < omas.length; i++ )
   {
      // 测试创建已存在的节点
      omas[i].testCreateExistCoord();

      // 测试删除不存在的节点
      omas[i].testRemoveNotExistCoord( svcnames[i] );

      // 测试删除节点时端口号不匹配
      omas[i].testRemoveCoordWithWrongSvc();

      // 测试删除节点时配置项非法
      omas[i].testRemoveCoordWithWrongConf();

      // 测试启动不存在的节点
      omas[i].testStartNotExistNode( svcnames[i] );

      // 测试停止不存在的节点
      omas[i].testStopNotExistNode( svcnames[i] );

      // 测试创建节点时配置项非法
      omas[i].testCreateCoordWithWrongConf( svcnames[i] );

      // 测试创建节点时路径无权限
      omas[i].testCreateCoordWithNoPermit( svcnames[i] );

      // 测试oma关闭后执行操作
      omas[i].testOmaClose();
   }
}
