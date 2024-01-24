/******************************************************************************
*@Description : seqDB-13179:使用ssh推送拉取文件
*@author      : Liang XueWang
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

   var remoteFile = "/tmp/pullsrc_13179_2.txt";
   var localFile = "/tmp/pulldst_13179_2.txt";
   var mode = "rw-r-----";
   var content = "testPull";

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var remote = getRemote( hostName, CMSVCNAME );
   var file = remote.getFile( remoteFile );
   file.write( content );
   file.close();

   //使用ssh拉取文件
   var ssh = new Ssh( hostName, user, password, port );
   ssh.pull( remoteFile, localFile );
   ssh.close();

   checkLocalFile( localFile, mode, content )

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );
}
