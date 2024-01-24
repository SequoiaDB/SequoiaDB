/******************************************************************************
*@Description : test js object oma function: createCoord removeCoord createData
*                              Normal        removeData startNode stopNode 
*               TestLink: 10602 Oma创建、删除、启动、停止协调节点和数据节点
*@author      : Liang XueWang
******************************************************************************/
// 测试正常创建启动停止删除协调节点
OmaTest.prototype.testCoordNodeOperationNormal = function( svcname )
{
   var isNodeCreated = false;
   this.testInit();
   try
   {
      var dbpath = RSRVNODEDIR + svcname;
      this.oma.createCoord( svcname, dbpath );
      isNodeCreated = true;
      this.oma.startNode( svcname );
      this.oma.stopNode( svcname );
      this.oma.removeCoord( svcname );
      isNodeCreated = false;
   }
   catch( e )
   {
      throw e;
   }
   finally
   {
      if( isNodeCreated )
      {
         this.oma.removeCoord( svcname );
      }
      this.oma.close();
   }
}

// 测试正常创建启动停止删除数据节点
OmaTest.prototype.testDataNodeOperationNormal = function( svcname )
{
   var isNodeCreated = false;
   this.testInit();
   try
   {
      var dbpath = RSRVNODEDIR + svcname;
      this.oma.createData( svcname, dbpath, { diaglevel: 5 } );
      isNodeCreated = true;
      this.oma.startNode( svcname );
      checkDataNodeValid( this.hostname, svcname );
      this.oma.stopNode( svcname );
      this.oma.removeData( svcname );
      isNodeCreated = false;
   }
   catch( e )
   {
      var backupDir = "/tmp/ci/rsrvnodelog/10602";
      var srcLogPath = this.hostname + ":" + CMSVCNAME + "@" + dbpath + "/diaglog/sdbdiag.log";
      File.mkdir( backupDir );
      File.scp( srcLogPath, backupDir + "/sdbdiag.log" );
      throw e;
   }
   finally
   {
      if( isNodeCreated )
      {
         this.oma.removeData( svcname );
      }
      this.oma.close();
   }
}

/******************************************************************************
*@Description : check node is valid ( create cs cl in node )
*@author      : Liang XueWang              
******************************************************************************/
function checkDataNodeValid ( hostname, svcname )
{
   var db = new Sdb( hostname, svcname );
   var CsName = "testDataNodeValidCs";
   var cs = db.createCS( CsName );
   cs.createCL( "bar" );
   db.dropCS( CsName );
   db.close();
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

   for( var i = 0; i < omas.length; i++ )
   {
      // 测试协调节点的正常操作
      omas[i].testCoordNodeOperationNormal( svcnames[i] );

      // 测试数据节点的正常操作
      omas[i].testDataNodeOperationNormal( svcnames[i] );
   }
}

