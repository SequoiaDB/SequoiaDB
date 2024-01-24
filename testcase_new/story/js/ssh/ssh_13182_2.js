/*******************************************************************************
*dcription: seqDB-13182:使用ssh推送拉取文件，源文件无权限
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

   var remoteFile = "/tmp/pullsrc_13182_2.txt";
   var localFile = "/tmp/pulldst_13182_2.txt";
   var srcModes = [0333, 0555, 0666]; // 无读、写、执行权限
   var content = "testPullSrcPermission";
   var mode = "rw-r-----";

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var remote = getRemote( hostName, CMSVCNAME );
   var ssh = new Ssh( hostName, user, password, port );
   for( var i = 0; i < srcModes.length; i++ )
   {
      var file = remote.getFile( remoteFile );
      file.write( content );
      file.chmod( remoteFile, srcModes[i] );
      file.close();
      try
      {
         ssh.pull( remoteFile, localFile );
         if( srcModes[i] === 0333 )
         {
            throw new Error( "NEED_ERROR" );
         }
         checkLocalFile( localFile, mode, content )
      }
      catch( e )
      {
         if( srcModes[i] === 0333 && !commCompareErrorCode( e, SDB_SYS ) )
         {
            commThrowError( e );
         }
      }
      cleanLocalFile( localFile );
      cleanRemoteFile( hostName, CMSVCNAME, remoteFile );
   }
   remote.close();
   ssh.close();
}

