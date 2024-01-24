/******************************************************************************
*@Description : seqDB-13190:使用ssh推送拉取文件，目标文件已存在（有权限）
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

   var remoteFile = "/tmp/pullsrc_13190_2.txt";
   var localFile = "/tmp/pulldst_13190_2.txt";
   var srcContent = "testPullDstExisted_src";
   var dstContent = "testPullDstExisted_dst";
   var dstMode = 0755;
   var mode = "rwxr-xr-x";

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var remote = getRemote( hostName, CMSVCNAME );
   var file = remote.getFile( remoteFile );
   file.write( srcContent );
   file.close();

   file = new File( localFile, dstMode );
   file.write( dstContent );
   file.close();

   var ssh = new Ssh( hostName, user, password, port );
   ssh.pull( remoteFile, localFile );
   ssh.close();

   checkLocalFile( localFile, mode, srcContent );

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );
}
