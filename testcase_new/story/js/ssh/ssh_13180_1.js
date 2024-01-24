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

   var srcFile = "/tmp/pushsrc_13180_1.txt";
   var dstFile = "/tmp/pushdst_13180_1.txt";
   var content = "testPush";

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var file = new File( srcFile, 0644 );
   file.write( content );
   file.close();

   var modes = [0755, 0644, 0640];
   var index = Math.floor( Math.random() * 3 );
   var permissions = ["rwxr-xr-x", "rw-r--r--", "rw-r-----"];
   var ssh = new Ssh( hostName, user, password, port );
   ssh.push( srcFile, dstFile, modes[index] );
   ssh.close();

   checkRemoteFile( hostName, dstFile, permissions[index], content );

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );
}
