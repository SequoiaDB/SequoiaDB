/******************************************************************************
*@Description : seqDB-13177:使用ssh执行正常命令，获取输出结果
*               seqDB-13178:使用ssh执行异常命令（不存在命令），获取输出结果
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

   //ssh执行正常命令
   var hostName = ssh.exec( "hostname" ).split( "\n" )[0];
   var expResult = getLocalHostName();
   if( hostName !== expResult )
   {
      throw new Error( "hostName = " + hostName + ", expResult = " + expResult );
   }

   var ret = ssh.getLastRet();
   if( ret !== 0 )
   {
      throw new Error( "The expected result is 0, but the actual result is " + ret );
   }

   var out = ssh.getLastOut();
   if( out.split( "\n" )[0] !== expResult )
   {
      throw new Error( "The expected result is" + COORDHOSTNAME + ", but actual result is " + out.split( "\n" )[0] );
   }

   //ssh执行异常命令
   assert.tryThrow( 127, function()
   {
      ssh.exec( "led" );
   } );

   var ret = ssh.getLastRet();
   if( ret !== 127 )
   {
      throw new Error( "ret = " + ret );
   }

   var out = ssh.getLastOut();
   if( out.indexOf( "not found" ) === -1 && out.indexOf( "未找到命令" ) === -1 )
   {
      throw new Error( "out = " + out );
   }

   ssh.close();
}

