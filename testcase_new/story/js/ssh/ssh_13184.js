/******************************************************************************
*@Description : seqDB-13184:重复执行close关闭ssh连接
                seqDB-13185:关闭ssh连接后执行操作
*@author      : Liang XueWang
******************************************************************************/
main( test );

function test ()
{
   if( !checkCmUser( COORDHOSTNAME, user ) )
   {
      return;
   }

   var ssh = new Ssh( COORDHOSTNAME, user, password, port );
   ssh.close();
   ssh.close();

   ssh.getLocalIP();
   ssh.getPeerIP();
   ssh.getLastRet();
   ssh.getLastOut();

   assert.tryThrow( SDB_NETWORK, function()
   {
      ssh.exec( "hostname" );
   } );

   assert.tryThrow( SDB_NETWORK, function()
   {
      ssh.push( "/tmp/src.txt", "/tmp/dst.txt" );
   } );

   assert.tryThrow( SDB_NETWORK, function()
   {
      ssh.pull( "/tmp/src.txt", "/tmp/dst.txt" );
   } );

}


