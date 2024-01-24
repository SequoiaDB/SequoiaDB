/******************************************************************************
*@Description : test oma svcname boundary number
*               TestLink: 10605 Oma创建节点，端口号为边界值   65535，1 
*                         10606 Oma创建节点，端口号超过边界值 65536，0
*@author      : Liang XueWang
******************************************************************************/

// 测试创建节点时端口号为边界值及边界值以内( 0, 65536 )
OmaTest.prototype.testSvcnameBoundary = function()
{
   this.testInit();
   var ErrorSvcname = [0, 65536, "0", "65536"];
   var CorrSvcname = [1, 65535, "1", "65535"];
   for( var i = 0; i < ErrorSvcname.length; i++ )
   {
      try
      {
         var svcname = ErrorSvcname[i];
         var dbpath = RSRVNODEDIR + svcname;
         this.oma.createData( svcname, dbpath );
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
   for( var i = 0; i < CorrSvcname.length; i++ )
   {
      var svcname = CorrSvcname[i];
      var dbpath = RSRVNODEDIR + svcname;
      this.oma.createData( svcname, dbpath );
      this.oma.removeData( svcname );
   }
   this.oma.close();
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localOma = new OmaTest( localhost, CMSVCNAME );
   var remoteOma = new OmaTest( remotehost, CMSVCNAME );
   var omas = [localOma, remoteOma];

   for( var i = 0; i < omas.length; i++ )
   {
      // 测试端口号取边界值及超出边界时创建节点（0,65536）（1,65535）
      omas[i].testSvcnameBoundary();
   }
}

