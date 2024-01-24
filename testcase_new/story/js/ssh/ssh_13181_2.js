/******************************************************************************
*@Description : seqDB-13181: 使用ssh推送拉取文件，源文件不存在
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

   var remoteFile = "/tmp/pullsrc_13181.txt";
   var localFile = "/tmp/pulldst_13181.txt";

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var ssh = new Ssh( hostName, user, password, port );
   assert.tryThrow( SDB_SYS, function()
   {
      ssh.pull( remoteFile, localFile );
   } );
   ssh.close();
}
