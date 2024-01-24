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

   var srcFile = "/tmp/pushsrc_13191_1.txt";
   var dstFile = "/tmp/pushdst_13191_1.txt";
   var srcContent = "testPushDstPermission_src";
   var dstContent = "testPushDstPermission_dst";
   var dstModes = [0333, 0555, 0666];
   var modes = ["-wx-wx-wx", "r-xr-xr-x", "rw-rw-rw-"];

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var file = new File( srcFile );
   file.write( srcContent );
   file.close();

   var remote = getRemote( hostName, CMSVCNAME );
   var ssh = new Ssh( hostName, user, password, port );
   for( var i = 0; i < dstModes.length; i++ )
   {
      var file = remote.getFile( dstFile );
      file.write( dstContent );
      file.chmod( dstFile, dstModes[i] );
      file.close();
      try
      {
         ssh.push( srcFile, dstFile );
         if( dstModes[i] === 0555 )
         {
            throw new Error( "NEED_ERROR" );
         }
         checkRemoteFile( hostName, dstFile, modes[i], srcContent );
      }
      catch( e )
      {
         if( dstModes[i] === 0555 && !commCompareErrorCode( e, SDB_SYS ) )
         {
            commThrowError( e );
         }
      }
      cleanRemoteFile( hostName, CMSVCNAME, dstFile );
   }
   ssh.close();
   remote.close();

   cleanLocalFile( srcFile );
}
