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

   var srcFile = "/tmp/pushsrc_13182_1.txt";
   var dstFile = "/tmp/pushdst_13182_1.txt";
   var content = "testPushSrcPermission";
   var modes = [0333, 0555, 0666]; // 无读、写、执行权限
   var isRoot = isRoot = System.getCurrentUser().toObj().user === "root" ? true : false;

   cleanLocalFile( srcFile );
   cleanRemoteFile( hostName, CMSVCNAME, dstFile );

   var ssh = new Ssh( hostName, user, password, port );
   for( var i = 0; i < modes.length; i++ )
   {
      var file = new File( srcFile );
      file.write( content );
      file.chmod( srcFile, modes[i] );
      file.close();

      try
      {
         ssh.push( srcFile, dstFile );
         if( !isRoot && modes[i] === 0333 )
         {
            throw new Error( "NEED_ERROR" );
         }
      }
      catch( e )
      {
         if( i === 0 && !commCompareErrorCode( e, SDB_PERM ) )
         {
            commThrowError( e );
         }
      }

      cleanLocalFile( srcFile );
      cleanRemoteFile( hostName, CMSVCNAME, dstFile );
   }
   ssh.close();
}
