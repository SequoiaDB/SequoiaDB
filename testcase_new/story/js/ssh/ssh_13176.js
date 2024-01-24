/******************************************************************************
*@Description : seqDB-13176:获取本地IP和远程IP
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   if( !isUserExist( COORDHOSTNAME, user ) )
   {
      return;
   }

   var ssh = new Ssh( COORDHOSTNAME, user, password, port );
   try
   {
      var localIp = ssh.getLocalIP();
      var cmd = new Cmd();
      var hostName = cmd.run( "hostname" ).split( "\n" )[0];
      var expect = getIPAddr( hostName );
      if( localIp !== expect && localIp !== "127.0.0.1" )
      {
         throw new Error( "localIp = " + localIp + ", expect = " + expect );
      }
      var peerIp = ssh.getPeerIP();
      expect = getIPAddr( COORDHOSTNAME );
      if( peerIp !== expect )
      {
         throw new Error( "peerIp = " + peerIp + ", expect = " + expect );
      }
   }
   finally
   {
      ssh.close();
   }
}
