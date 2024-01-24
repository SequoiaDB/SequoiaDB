/************************************
*@Description: seqDB-20175 : 截断File对象的内容
*@author:      luweikang
*@createDate:  2019.11.04
**************************************/


main( test );

function test ()
{
   var loaclFilePath = WORKDIR + "/localFile20175/";
   var remoteFilePath = WORKDIR + "/remoteFile20175/";
   var fileName = "file20175";
   var fileSize = 1024;
   var content = "";
   for( var i = 0; i < fileSize; i++ )
   {
      var content = content + "a";
   }

   var localFile1 = createFile( loaclFilePath, fileName + "_1", content, true );
   localFile1.truncate( fileSize );
   localFile1.close();
   checkTruncateResult( loaclFilePath, fileName + "_1", content, true );

   var localFile2 = createFile( loaclFilePath, fileName + "_2", content, true );
   localFile2.truncate( fileSize / 2 );
   localFile2.close();
   checkTruncateResult( loaclFilePath, fileName + "_2", content.substr( 0, fileSize / 2 ), true );

   var localFile3 = createFile( loaclFilePath, fileName + "_3", content, true );
   localFile3.truncate( fileSize * 2 );
   localFile3.close();
   checkTruncateResult( loaclFilePath, fileName + "_3", content, true );

   var localFile4 = createFile( loaclFilePath, fileName + "_4", content, true );
   localFile4.truncate( 0 );
   localFile4.close();
   checkTruncateResult( loaclFilePath, fileName + "_4", null, true );

   var localFile5 = createFile( loaclFilePath, fileName + "_5", content, true );
   localFile5.truncate();
   localFile5.close();
   checkTruncateResult( loaclFilePath, fileName + "_5", null, true );

   var remoteFile1 = createFile( remoteFilePath, fileName + "_1", content, false );
   remoteFile1.truncate( fileSize );
   remoteFile1.close();
   checkTruncateResult( remoteFilePath, fileName + "_1", content, false );

   var remoteFile2 = createFile( remoteFilePath, fileName + "_2", content, false );
   remoteFile2.truncate( fileSize / 2 );
   remoteFile2.close();
   checkTruncateResult( remoteFilePath, fileName + "_2", content.substr( 0, fileSize / 2 ), false );

   var remoteFile3 = createFile( remoteFilePath, fileName + "_3", content, false );
   remoteFile3.truncate( fileSize * 2 );
   remoteFile3.close();
   checkTruncateResult( remoteFilePath, fileName + "_3", content, false );

   var remoteFile4 = createFile( remoteFilePath, fileName + "_4", content, false );
   remoteFile4.truncate( 0 );
   remoteFile4.close();
   checkTruncateResult( remoteFilePath, fileName + "_4", null, false );

   var remoteFile5 = createFile( remoteFilePath, fileName + "_5", content, false );
   remoteFile5.truncate();
   remoteFile5.close();
   checkTruncateResult( remoteFilePath, fileName + "_5", null, false );

   removeFile( loaclFilePath, true );
   removeFile( remoteFilePath, false );
}

function createFile ( filePath, fileName, content, isLocal )
{

   if( isLocal )
   {
      File.mkdir( filePath );
      var file = new File( filePath + fileName );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      remote.getFile().mkdir( filePath );
      var file = remote.getFile( filePath + fileName );
   }

   file.write( content );
   return file;
}

function checkTruncateResult ( filePath, fileName, expContent, isLocal )
{
   if( isLocal )
   {
      var file = new File( filePath + fileName );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      var file = remote.getFile( filePath + fileName );
   }

   if( expContent != null )
   {
      var actStr = file.read();
      file.close();
      assert.equal( actStr, expContent );
   }
   else
   {
      assert.tryThrow( SDB_EOF, function()
      {
         file.read();
      } );
   }

}

function removeFile ( filePath, isLocal )
{
   if( isLocal )
   {
      File.remove( filePath );
   }
   else
   {
      var remote = new Remote( COORDHOSTNAME, CMSVCNAME );
      remote.getFile().remove( filePath );
   }
}