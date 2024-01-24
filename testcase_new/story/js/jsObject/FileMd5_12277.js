/******************************************************************************
*@Description : test js object File function: getInfo md5 stat
*               TestLink : 10815 ��ȡFile������Ϣ
*                          10826 File����鿴�ļ���Ϣ
*                          10827 ����File�����ļ�Md5ֵ
*@auhor       : Liang XueWang
******************************************************************************/
FileTest.prototype.testMd5Mode = function()
{
   this.init();

   var readOnlyFile = "/tmp/readOnlyFile.txt";
   var cannotReadFile = "/tmp/cannotReadFile.txt";
   var user = this.cmd.run( "whoami" ).split( "\n" )[0];

   // create file
   if( this.isLocal )
   {
      var file1 = new File( readOnlyFile );
      file1.write( "123" );
      file1.close();
      File.chmod( readOnlyFile, 0444 );
      var file2 = new File( cannotReadFile );
      file2.write( "abc" );
      file2.close();
      File.chmod( cannotReadFile, 0222 );
   }
   else
   {
      var file = this.remote.getFile();
      var file1 = this.remote.getFile( readOnlyFile );
      file1.write( "123" );
      file1.close();
      file.chmod( readOnlyFile, 0444 );
      var file2 = this.remote.getFile( cannotReadFile );
      file2.write( "abc" );
      file2.close();
      file.chmod( cannotReadFile, 0222 );
   }

   // check read only file md5
   this.file.md5( readOnlyFile );

   // check cannot read file 
   try
   {
      this.file.md5( cannotReadFile );
      if( user !== "root" )
      {
         throw new Error( "should error" );
      }
   }
   catch( e )
   {
      if( user === "root" )
      {
         throw new Error( "testMd5Mode get md5 of file " + cannotReadFile + " with user: " + user + " " + this + 0 + e );
      }
      else
      {
         if( e.message != SDB_PERM )
         {
            throw e;
         }
      }
   }

   this.cmd.run( "rm -rf " + readOnlyFile );
   this.cmd.run( "rm -rf " + cannotReadFile );

   this.release();
}

main( test );

function test ()
{
   // ��ȡ����������Զ������
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var ft1 = new FileTest( localhost, CMSVCNAME );     // ����File������
   var ft2 = new FileTest( remotehost, CMSVCNAME );    // Զ��File������

   var fts = [ft1, ft2];

   for( var i = 0; i < fts.length; i++ )
   {
      // �����ļ�MD5
      fts[i].testMd5Mode();
   }
}

