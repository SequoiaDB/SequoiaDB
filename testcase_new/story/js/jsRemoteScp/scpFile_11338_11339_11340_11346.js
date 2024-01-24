/************************************
*@Description:
*@author:      zhaoyu
*@createdate:  2017.4.14
*@testlinkCase:seqDB-11338/seqDB-11339/seqDB-11340/seqDB-11346
**************************************/
main( test );

function test () 
{
   var localhost = toolGetLocalhost();
   var localFileName = "/tmp/test_11338";
   var localFile = new File( localFileName, 0777 );

   var remotehost = toolGetRemotehost();
   var remote = new Remote( remotehost, CMSVCNAME );
   var remoteFile = remote.getFile();

   if( !localFile.exist( WORKDIR ) )
   {
      localFile.mkdir( WORKDIR, 0777 );
   }

   var localCmd = new Cmd();
   var localInstallPath = commGetInstallPath();
   var localSrcFileName = localInstallPath + "/bin/sdbdpsdump";
   var localDstFileName = WORKDIR + "/dstFile_11338";

   if( !remoteFile.exist( WORKDIR ) )
   {
      remoteFile.mkdir( WORKDIR, 0777 );
   }

   var remoteCmd = remote.getCmd();
   var remoteInstallPath = commGetRemoteInstallPath( COORDHOSTNAME, CMSVCNAME );
   var remoteSrcFileName = remoteInstallPath + "/bin/sdbdpsdump";
   var remoteDstFileName = WORKDIR + "/dstFile_11338";

   //clear env
   if( remoteFile.exist( remoteDstFileName ) )
   {
      remoteFile.remove( remoteDstFileName );
   }
   if( localFile.exist( localDstFileName ) )
   {
      localFile.remove( localDstFileName );
   }

   //seqDB-11338
   scpTest( localSrcFileName, localDstFileName, localFile, localFile );

   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, remoteFile );

   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      localDstFileName,
      remoteFile, localFile );

   scpTest( localSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      localFile, remoteFile );

   //seqDB-11339
   //remote to remote
   assert.tryThrow( SDB_FE, function()
   {
      replaceFile = remote.getFile( remoteDstFileName );
      File.scp( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
         remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
         false );
   } )
   replaceFile.remove( remoteDstFileName );

   //local to remote
   assert.tryThrow( SDB_FE, function()
   {
      replaceFile = remote.getFile( remoteDstFileName );
      File.scp( localSrcFileName,
         remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
         false );
   } )
   replaceFile.remove( remoteDstFileName );

   //remote to local
   assert.tryThrow( SDB_FE, function()
   {
      replaceFile = new File( localDstFileName );
      File.scp( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
         localDstFileName,
         false );
   } )
   replaceFile.remove( localDstFileName );

   //local to local
   assert.tryThrow( SDB_FE, function()
   {
      replaceFile = new File( localDstFileName );
      File.scp( localSrcFileName,
         localDstFileName,
         false );
   } )
   replaceFile.remove( localDstFileName );

   if( remoteFile.exist( remoteDstFileName ) )
   {
      remoteFile.remove( remoteDstFileName );
   }
   var srcMode = 447;
   replaceFile = remote.getFile( remoteDstFileName, srcMode );
   File.scp( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      true, 0711 );

   //check size
   var expectMd5 = remoteFile.md5( remoteSrcFileName );
   var actualMd5 = remoteFile.md5( remoteDstFileName );
   assert.equal( expectMd5, actualMd5 );

   //check mode
   var umask = remoteFile.getUmask();
   var expectMode = srcMode & ~umask;
   var actualMode = remoteFile._getPermission( remoteDstFileName );
   assert.equal( expectMode, actualMode );
   remoteFile.remove( remoteDstFileName );

   //seqDB-11340
   //local to remote
   scpTest( localSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      localFile, remoteFile, 448 );

   scpTest( localSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      localFile, remoteFile, 0 );

   //remote to local
   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      localDstFileName,
      remoteFile, localFile, 448 );

   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      localDstFileName,
      remoteFile, localFile, 0 );

   //local to local
   scpTest( localSrcFileName,
      localDstFileName,
      localFile, localFile, 448 );

   scpTest( localSrcFileName,
      localDstFileName,
      localFile, localFile, 0 );

   //remote to remote
   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, remoteFile, 448 );

   scpTest( remotehost + ":" + CMSVCNAME + "@" + remoteSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, remoteFile, 0 );

   var readOnlyFileName = WORKDIR + "/readOnly_11338";

   //clear env
   if( localFile.exist( readOnlyFileName ) )
   {
      localFile.remove( readOnlyFileName );
   }
   if( remoteFile.exist( readOnlyFileName ) )
   {
      remoteFile.remove( readOnlyFileName );
   }

   var readOnlylocalFile = new File( readOnlyFileName, 0755, SDB_FILE_CREATE | SDB_FILE_READONLY );
   var readOnlyremoteFile = remote.getFile( readOnlyFileName, 0755, SDB_FILE_CREATE | SDB_FILE_READONLY );
   File.scp( localSrcFileName, readOnlyFileName, true, 0444 );

   //local to remote
   scpTest( readOnlyFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      readOnlylocalFile, readOnlyremoteFile, 0444 );

   //remote to local
   scpTest( remotehost + ":" + CMSVCNAME + "@" + readOnlyFileName,
      localDstFileName,
      readOnlyremoteFile, readOnlylocalFile, 0444 );

   //local to local
   scpTest( readOnlyFileName,
      localDstFileName,
      readOnlylocalFile, localFile, 0444 );

   //remote to remote
   scpTest( remotehost + ":" + CMSVCNAME + "@" + readOnlyFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      readOnlyremoteFile, readOnlyremoteFile, 0444 );
   File.remove( readOnlyFileName );
   if( remoteFile.exist( readOnlyFileName ) )
   {
      remoteFile.remove( readOnlyFileName );
   }

   var user = System.getCurrentUser().toObj().user;
   if( user !== "root" )
   {
      //src only read, only for user adbadmin
      var readOnlyFileName = WORKDIR + "/readOnly_11338";
      //local
      File.scp( localSrcFileName, readOnlyFileName, true, 0444 );
      File.scp( readOnlyFileName, remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName, true, 0444 );

      File.remove( readOnlyFileName );
      if( remoteFile.exist( remoteDstFileName ) )
      {
         remoteFile.remove( remoteDstFileName );
      }

      //remote
      File.scp( localSrcFileName, remotehost + ":" + CMSVCNAME + "@" + readOnlyFileName,
         true, 0444 );
      File.scp( remotehost + ":" + CMSVCNAME + "@" + readOnlyFileName,
         remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
         true, 0444 );
      File.remove( readOnlyFileName );
      if( remoteFile.exist( readOnlyFileName ) )
      {
         remoteFile.remove( readOnlyFileName );
      }
      if( remoteFile.exist( remoteDstFileName ) )
      {
         remoteFile.remove( remoteDstFileName );
      }

      //src only write, only for user sdbadmin
      var writeOnlyFileName = WORKDIR + "/writeOnly_11338";

      //clear env
      if( File.exist( writeOnlyFileName ) )
      {
         File.remove( writeOnlyFileName );
      }
      if( remoteFile.exist( writeOnlyFileName ) )
      {
         remoteFile.remove( writeOnlyFileName );
      }

      //local
      assert.tryThrow( SDB_PERM, function()
      {
         File.scp( localSrcFileName, writeOnlyFileName, true, 0222 );
         File.scp( writeOnlyFileName, remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName, true, 0222 );
      } );

      //remote
      assert.tryThrow( SDB_PERM, function()
      {
         File.scp( localSrcFileName, remotehost + ":" + CMSVCNAME + "@" + writeOnlyFileName,
            true, 0222 );
         File.scp( remotehost + ":" + CMSVCNAME + "@" + writeOnlyFileName,
            remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
            true, 0222 );
      } );
      File.remove( writeOnlyFileName );
   }

   //seqDB-11346
   checkArgumentScp( localInstallPath, remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, 0755, true );

   checkArgumentScp( remotehost + ":" + CMSVCNAME + remoteSrcFileName, remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, 0755, true );

   checkArgumentScp( "", remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName, remoteFile, 0755, true );

   checkArgumentScp( localSrcFileName, remotehost + CMSVCNAME + "@" + remoteDstFileName,
      remoteFile, 0755, true );

   checkArgumentScp( localSrcFileName, WORKDIR, remoteFile, 0755, true );

   checkArgumentScp( localSrcFileName, "", remoteFile, 0755, true );

   scpTest( localSrcFileName,
      remotehost + ":" + CMSVCNAME + "@" + remoteDstFileName,
      localFile, remoteFile, 0755, "a" );
   File.remove( localFileName );
}

function scpTest ( srcFileName, dstFileName, srcFile, dstFile, mode, isReplace )
{
   File.scp( srcFileName, dstFileName, isReplace, mode );

   srcIndex = srcFileName.indexOf( "@" );
   dstIndex = dstFileName.indexOf( "@" );
   if( srcIndex > 0 )
   {
      md5SrcFileName = srcFileName.substring( srcIndex + 1 );
   }
   else
   {
      md5SrcFileName = srcFileName;
   }
   if( dstIndex > 0 )
   {
      md5DstFileName = dstFileName.substring( dstIndex + 1 );
   }
   else
   {
      md5DstFileName = dstFileName;
   }

   var expectMd5 = srcFile.md5( md5SrcFileName );
   var actualMd5 = dstFile.md5( md5DstFileName );

   var srcMode = srcFile._getPermission( md5SrcFileName );
   var dstMode = dstFile._getPermission( md5DstFileName );
   if( typeof ( mode ) == "undefined" ) { mode = srcMode; }

   var umask = dstFile.getUmask();
   if( mode === 0 )
   {
      mode = 416;
   }
   else
   {
      mode = mode & ~umask;
   }
   dstFile.remove( md5DstFileName );

   if( expectMd5 !== actualMd5 || mode !== dstMode )
   {
      throw new Error( "MD5_MODE_NOT_SAME" );
   }
   //forceGC(); 
}

function checkArgumentScp ( srcFileName, dstFileName, dstFile, mode, isReplace )
{
   assert.tryThrow( [SDB_INVALIDARG, SDB_IO, SDB_FNE, SDB_PERM], function()
   {
      File.scp( srcFileName, dstFileName, isReplace, mode );
   } );

   dstIndex = dstFileName.indexOf( "@" );
   if( dstIndex > 0 )
   {
      md5DstFileName = dstFileName.substring( dstIndex + 1 );
   }
   else
   {
      md5DstFileName = dstFileName;
   }

   //check
   if( dstFile.exist( md5DstFileName ) && dstFile.isFile( md5DstFileName ) )
   {
      throw new Error( "FILE_EXIST" );
   }
}
