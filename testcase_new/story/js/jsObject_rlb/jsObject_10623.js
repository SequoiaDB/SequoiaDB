/******************************************************************************
*@Description : seqDB-10623:Oma启动本地cm
                seqDB-10624:Oma启动本地cm，alivetime设置为0 
*@author      : Zhao Xiaoni
******************************************************************************/
main( test );

function test ()
{
   var ssh = new Ssh( COORDHOSTNAME, REMOTEUSER, REMOTEPASSWD );

   var svcName = toolGetIdleSvcName( COORDHOSTNAME, CMSVCNAME );
   if( svcName === undefined )
   {
      return;
   }

   // 先停止当前的cm
   var InstallPath = commGetRemoteInstallPath( COORDHOSTNAME, CMSVCNAME );
   ssh.exec( InstallPath + "/bin/sdbcmtop" );

   try
   {
      //测试启动超时时间为10的cm
      var command = InstallPath + "/bin/sdb \"Oma.start( { port: " + svcName + ", alivetime: 10, standalone: true } );\"";
      ssh.exec( command );

      // 检查cm是否启动
      var oma = new Oma( COORDHOSTNAME, svcName );
      oma.close();

      sleep( 20 * 1000 );
      command = InstallPath + "/bin/sdb \"Sdbtool.listNodes( { type: 'cm', showalone: true } ).toArray();\"";
      var nodeArray = ssh.exec( command );

      if( nodeArray.length != 0 )
      {
         throw new Error( "nodeArray: " + JSON.stringify( nodeArray ) );
      }

      // 测试启动超时时间为0的cm
      command = InstallPath + "/bin/sdb \"Oma.start( { port: " + svcName + ", alivetime: 0, standalone: false } );\"";
      ssh.exec( command );

      oma = new Oma( COORDHOSTNAME, svcName );
      oma.close();

      sleep( 20 * 1000 );
      command = InstallPath + "/bin/sdb \"Sdbtool.listNodes( { type: 'cm' } ).toArray();\"";
      var nodeString = ssh.exec( command );
      var nodeArray = JSON.parse( "[" + nodeString + "]" );
      if( nodeArray.length !== 2 )
      {
         throw new Error( "nodeArray: " + JSON.stringify( nodeArray ) );
      }
   }
   finally
   {
      ssh.exec( InstallPath + "/bin/sdbcmtop --I" );
      ssh.exec( InstallPath + "/bin/sdbcmart" );
      ssh.close();
   }
}