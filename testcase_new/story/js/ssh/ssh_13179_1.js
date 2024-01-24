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

   var srcFile = "/tmp/pushsrc_13179_1.txt";
   var dstFile = "/tmp/pushdst_13179_1.txt";
   var mode = "rwxr-xr-x";
   var content = "testPush";

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var file = new File( srcFile );
   file.write( content );
   file.close();

   //使用ssh推送文件
   var ssh = new Ssh( hostName, user, password, port );
   ssh.push( srcFile, dstFile );
   checkRemoteFile( hostName, dstFile, mode, content )

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );
}

