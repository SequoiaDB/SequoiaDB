/************************************
*@Description:
*@author:      zhaoyu
*@createdate:  2017.4.13
*@testlinkCase:seqDB-11335/seqDB-11337
**************************************/

main( test );

function test () 
{
   var localhost = toolGetLocalhost();

   var remotehost = toolGetRemotehost();
   var remote = new Remote( remotehost, CMSVCNAME );
   var remoteFile = remote.getFile();

   var remoteCmd = remote.getCmd();
   var remoteInstallPath = commGetRemoteInstallPath( COORDHOSTNAME, CMSVCNAME );
   var readFileName = remoteInstallPath + "/bin/sdbdpsdump";

   if( !remoteFile.exist( WORKDIR ) )
   {
      remoteFile.mkdir( WORKDIR, 0777 );
   }

   var writeFileName = WORKDIR + "/writeFile_11337";
   var emptyFileName = WORKDIR + "/emptyFile_11337";
   //0 size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 0 );

   //default size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile );

   //size = 4M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 4194304 );

   //size = fileLength
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   var fileSize = parseInt( readFile.stat( readFileName ).toObj().size );
   readWriteContentAndCheck( readFile, writeFile, fileSize );

   //size > fileLength
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   var overSize = fileSize + 104857600;
   readWriteContentAndCheck( readFile, writeFile, overSize, fileSize );

   //read empty file
   try
   {
      if( remoteFile.exist( emptyFileName ) )
      {
         remoteFile.remove( emptyFileName );
      }
      var emptyFile = remote.getFile( emptyFileName );
      var content = emptyFile.readContent();
      throw new Error( "EXPECT GET AN ERROR" );
   }
   catch( e )
   {
      if( e.message != SDB_EOF )
      {
         throw e;
      }
   }
   remoteFile.remove( emptyFileName );

   //many times read and write, size 1M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 102400 );

   //many times read and write, size 100M
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 104857600 );

   //mode test
   //SDB_FILE_READONLY
   var readFile = remote.getFile( readFileName, 0777, SDB_FILE_READONLY );
   var content = readFile.readContent();

   assert.tryThrow( SDB_PERM, function()
   {
      var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READONLY );
      writeFile.writeContent( content );
   } );

   //SDB_FILE_WRITEONLY
   assert.tryThrow( SDB_PERM, function()
   {
      var readFile = remote.getFile( readFileName, 0777, SDB_FILE_WRITEONLY );
      var content = readFile.readContent();
   } );

   var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_WRITEONLY );
   writeFile.writeContent( content );
   var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
   assert.equal( writeLength, 1024 );
   writeFile.remove( writeFileName );

   //SDB_FILE_READWRITE
   var readFile = remote.getFile( readFileName, 0777, SDB_FILE_READWRITE );
   var content = readFile.readContent();
   var writeFile = remote.getFile( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READWRITE );
   writeFile.writeContent( content );
   var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
   assert.equal( writeLength, 1024 );

   //SEQUOIADBMAINSTREAM-2661, clear()
   content.clear();
   var readLength = content.getLength();
   assert.equal( readLength, 0 );

   writeFile.remove( writeFileName );

   //argument check
   //float size
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 1024.88 )

   //string
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   checkArgumentRead( readFile, "a" );

   //negative int; 
   checkArgumentRead( readFile, -10 );
   checkArgumentRead( readFile, -1023 );

   //long
   checkArgumentRead( readFile, 9007199254740992, -2 );

   //writeContent argument illegal
   //miss argument
   assert.tryThrow( SDB_INVALIDARG, function()
   {
      var readFile = remote.getFile( readFileName );
      var content = readFile.readContent();
      if( remoteFile.exist( writeFileName ) )
      {
         remoteFile.remove( writeFileName );
      }
      var writeFile = remote.getFile( writeFileName );
      writeFile.writeContent();
   } );
   remoteFile.remove( writeFileName );

   //type illegal
   var readFile = remote.getFile( readFileName );
   var writeFile = remote.getFile( writeFileName );
   checkArgumentWrite( readFile, writeFile, "content" );

   //_getPermission
   getPermission( remoteFile );

   //toBase64Code(), set length = 3 multiples
   var actualFileName = WORKDIR + "/tobase64File_11337";
   var expectFileName = WORKDIR + "/base64File_11337";
   var length = 3000000;

   if( remoteFile.exist( actualFileName ) )
   {
      remoteFile.remove( actualFileName );
   }
   if( remoteFile.exist( expectFileName ) )
   {
      remoteFile.remove( expectFileName );
   }

   var readFile = remote.getFile( readFileName );
   var actualFile = remote.getFile( actualFileName );
   var expectFile = remote.getFile( expectFileName );
   toBase64CodeTest( readFile, actualFile, expectFile, length, remoteCmd );

}
