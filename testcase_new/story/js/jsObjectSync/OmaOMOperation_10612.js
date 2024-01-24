/******************************************************************************
*@Description : test js object oma function: createOM removeOM
*               TestLink: 10612 Oma创建、删除sdbom服务进程
*                         10613 Oma创建sdbom服务进程，服务进程已存在
*@author      : Liang XueWang
******************************************************************************/

// 测试创建删除om
OmaTest.prototype.testOMOperation = function( svcname, isOmExist )
{
   this.testInit();
   var dbpath = RSRVNODEDIR + svcname;
   if( isOmExist )
   {
      try
      {
         this.oma.createOM( svcname, dbpath );
         throw new Error( "should error" );
      }
      catch( e )
      {
         if( e.message != SDBCM_NODE_EXISTED )
         {
            throw e;
         }
      }
   }
   else
   {
      this.oma.createOM( svcname, dbpath );
      this.oma.removeOM( svcname );
   }
   this.oma.close();
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

   // 判断OM是否存在
   var OmExist1 = isOmExist( localhost["hostname"], CMSVCNAME );
   var OmExist2 = isOmExist( remotehost["hostname"], CMSVCNAME );

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );

   var omas = [localOma, remoteOma];
   var svcnames = [svcname1, svcname2];
   var OmExists = [OmExist1, OmExist2];

   for( var i = 0; i < omas.length; i++ )
   {
      // 测试OM的正常操作
      omas[i].testOMOperation( svcnames[i], OmExists[i] );
   }
}


