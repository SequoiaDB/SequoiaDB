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

   var srcFile = "/tmp/pushsrc_13190_1.txt";
   var dstFile = "/tmp/pushdst_13190_1.txt";
   var srcContent = "testPushDstExisted_src";
   var dstContent = "testPushDstExisted_dst";
   var dstMode = 0755;
   var mode = "rwxr-xr-x";

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var file = new File( srcFile );
   file.write( srcContent );
   file.close();

   var remote = getRemote( hostName, CMSVCNAME );
   file = remote.getFile( dstFile, dstMode );
   file.write( dstContent );
   file.close();
   remote.close();

   var ssh = new Ssh( hostName, user, password, port );
   ssh.push( srcFile, dstFile );
   ssh.close();

   checkRemoteFile( hostName, dstFile, mode, srcContent );

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );
}
