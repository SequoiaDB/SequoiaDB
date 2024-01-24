/******************************************************************************
*@Description: seqDB-13180: 使用ssh推送拉取文件，指定文件权限
*@author: Liang XueWang
******************************************************************************/
testConf.skipStandAlone = true;

main( test );

function test ()
{
   var hostName = getRemoteHostName();
   if( !checkCmUser( hostName, user ) )
   {
      return;
   }

   var remoteFile = "/tmp/pullsrc_13180_2.txt";
   var localFile = "/tmp/pulldst_13180_2.txt";
   var srcMode = 0644;
   var modes = [0755, 0644, 0640];
   var permissions = ["rwxr-xr-x", "rw-r--r--", "rw-r-----"];
   var content = "testPullWithMode";

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var remote = getRemote( hostName, CMSVCNAME );
   var file = remote.getFile( remoteFile );
   file.write( content );
   file.close();

   var ssh = new Ssh( hostName, user, password, port );
   for( var i = 0; i < modes.length; i++ )
   {
      ssh.pull( remoteFile, localFile, modes[i] );
      checkLocalFile( localFile, permissions[i], content );
      cleanLocalFile( localFile );
   }
   ssh.close();
   remote.close();

   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );
}
