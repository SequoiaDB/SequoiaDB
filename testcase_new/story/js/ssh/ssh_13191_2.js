/******************************************************************************
*@Description : seqDB-13191:使用ssh推送拉取文件，目标文件已存在（无权限）
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

   var remoteFile = "/tmp/pullsrc_13191_2.txt";
   var localFile = "/tmp/pulldst_13191_2.txt";
   var srcContent = "testPullDstPermission_src";
   var dstContent = "testPullDstPermission_dst";
   var dstModes = [0333, 0555, 0666];
   var modes = ["-wx-wx-wx", "r-xr-xr-x", "rw-rw-rw-"];
   var isRoot = System.getCurrentUser().toObj().user === "root" ? true : false;

   cleanLocalFile( localFile );
   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );

   var remote = getRemote( hostName, CMSVCNAME );
   var file = remote.getFile( remoteFile );
   file.write( srcContent );
   file.close();
   remote.close();

   var ssh = new Ssh( hostName, user, password, port );
   for( var i = 0; i < dstModes.length; i++ )
   {
      var file = new File( localFile );
      file.write( dstContent );
      file.chmod( localFile, dstModes[i] );
      file.close();
      try
      {
         ssh.pull( remoteFile, localFile );
         if( !isRoot && dstModes[i] === 0555 )
         {
            throw new Error( "NEED_ERROR" );
         }
         checkLocalFile( localFile, modes[i], srcContent );
      }
      catch( e )
      {
         if( dstModes[i] === 0555 && !commCompareErrorCode( e, SDB_PERM	 ) )
         {
            commThrowError( e );
         }
      }
      cleanLocalFile( localFile );
   }
   ssh.close();

   cleanRemoteFile( hostName, CMSVCNAME, remoteFile );
}
