/************************************
*@Description:
*@author:      zhaoyu
*@createdate:  2017.4.12
*@testlinkCase:seqDB-11334/seqDB-11336/seqDB-11343/seqDB-11344/seqDB-11345/seqDB-11341/seqDB-11342
**************************************/
main( test );

function test () 
{
   //get path
   var cmd = new Cmd();
   var localInstallPath = commGetInstallPath();
   var readFileName = localInstallPath + "/bin/sdbdpsdump";

   if( !File.exist( WORKDIR ) )
   {
      File.mkdir( WORKDIR, 0777 );
   }

   var writeFileName = WORKDIR + "/writeFile_11336";
   var emptyFileName = WORKDIR + "/emptyFile_11336";

   if( File.exist( writeFileName ) )
   {
      File.remove( writeFileName );
   }

   //0 size
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 0 );

   //default size
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile );

   //size = 4M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 4194304 );

   //size = fileLength
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   var fileSize = parseInt( File.stat( readFileName ).toObj().size );
   readWriteContentAndCheck( readFile, writeFile, fileSize );

   //size > fileLength
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   var overSize = fileSize + 104857600;
   readWriteContentAndCheck( readFile, writeFile, overSize, fileSize );

   //read empty file
   assert.tryThrow( SDB_EOF, function()
   {
      if( File.exist( emptyFileName ) )
      {
         File.remove( emptyFileName );
      }
      var emptyFile = new File( emptyFileName );
      var content = emptyFile.readContent();
   } );
   File.remove( emptyFileName );

   //many times read and write, size 1M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 102400 );

   //many times read and write, size 100M
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentManyTimes( readFile, writeFile, 104857600 );

   //mode test
   //SDB_FILE_READONLY
   var readFile = new File( readFileName, 0777, SDB_FILE_READONLY );
   var content = readFile.readContent();

   assert.tryThrow( SDB_PERM, function()
   {
      var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READONLY );
      writeFile.writeContent( content );
   } );

   //SDB_FILE_WRITEONLY
   assert.tryThrow( SDB_PERM, function()
   {
      var readFile = new File( readFileName, 0777, SDB_FILE_WRITEONLY );
      var content = readFile.readContent();
   } );

   var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_WRITEONLY );
   writeFile.writeContent( content );
   var writeLength = parseInt( writeFile.stat( writeFileName ).toObj().size );
   if( writeLength !== 1024 )
   {
      throw new Error("WRITE_LENGTH_ERROR");
   }
   writeFile.remove( writeFileName );

   //SDB_FILE_READWRITE
   var readFile = new File( readFileName, 0777, SDB_FILE_READWRITE );
   var content = readFile.readContent();
   var writeFile = new File( writeFileName, 0777, SDB_FILE_CREATE | SDB_FILE_READWRITE );
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
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   readWriteContentAndCheck( readFile, writeFile, 1024.88 )

   //string
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
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
      var readFile = new File( readFileName );
      var content = readFile.readContent();
      if( File.exist( writeFileName ) )
      {
         File.remove( writeFileName );
      }
      var writeFile = new File( writeFileName );
      writeFile.writeContent();
   } )
   File.remove( writeFileName );

   //type illegal
   var readFile = new File( readFileName );
   var writeFile = new File( writeFileName );
   checkArgumentWrite( readFile, writeFile, "content" );

   //_getPermission
   getPermission( File );

   //toBase64Code(), set length = 3 multiples
   var actualFileName = WORKDIR + "/tobase64File_11336";
   var expectFileName = WORKDIR + "/base64File_11336";
   var length = 3000000;

   if( File.exist( actualFileName ) )
   {
      File.remove( actualFileName );
   }
   if( File.exist( expectFileName ) )
   {
      File.remove( expectFileName );
   }

   var readFile = new File( readFileName );
   var actualFile = new File( actualFileName );
   var expectFile = new File( expectFileName );
   var cmd = new Cmd();
   toBase64CodeTest( readFile, actualFile, expectFile, length, cmd );

}
