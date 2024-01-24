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

   var srcFile = "/tmp/pushsrc_13181_1.txt";
   var dstFile = "/tmp/pushdst_13181_1.txt";

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var ssh = new Ssh( hostName, user, password, port );
   assert.tryThrow( SDB_FNE, function()
   {
      ssh.push( srcFile, dstFile );
   } );
   ssh.close();
}
