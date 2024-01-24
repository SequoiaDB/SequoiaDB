/******************************************************************************
*@Description : test js object File function: mkdir move copy remove
*               TestLink : 10837 新建目录
*                          10836 移动文件
*                          10835 拷贝文件
*                          10833 删除文件
*@auhor       : Liang XueWang
******************************************************************************/
// 测试创建目录，移动文件，拷贝文件，删除文件
FileTest.prototype.testFileOperation = function()
{
   this.init();

   var tmpDirName = "/tmp/FileTest";
   var tmpFileName = "/tmp/FileTest/tmpFile";
   var tmpFile;

   this.file.mkdir( tmpDirName );   // 创建目录
   checkMkdir( this.cmd, tmpDirName );
   if( this.isLocal )
      tmpFile = new File( tmpFileName, 0644 );
   else
      tmpFile = this.remote.getFile( tmpFileName, 0644 );

   try
   {
      this.file.move( tmpFileName, tmpFileName + ".move" ); // 移动文件
      checkMove( this.cmd, tmpFileName, tmpFileName + ".move" );
      this.file.move( tmpFileName + ".move", tmpFileName );

      this.file.copy( tmpFileName, tmpFileName + ".copy" );  // 拷贝文件
      checkCopy( this.cmd, tmpFileName, tmpFileName + ".copy" );

      this.file.remove( tmpFileName );      // 删除文件
      checkRemove( this.cmd, tmpFileName );
      this.file.remove( tmpFileName + ".copy" );
   }
   finally
   {
      this.file.remove( tmpDirName );
   }

   this.release();
}

// 测试拷贝文件时指定权限
FileTest.prototype.testCopyWithMode = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var srcFile = sdbDir[0] + "/bin/sdb";      // -rwxr-xr-x
   var dstFile = sdbDir[0] + "/bin/sdb.bak";
   var mode = this.file.stat( srcFile ).toObj().mode.slice( 0, 10 );
   if( mode !== "rwxr-xr-x" )
   {
      this.release();
      return;
   }
   var umask = this.file.getUmask( '8' );
   if( umask !== "0022" )
   {
      this.release();
      return;
   }
   this.cmd.run( "rm -rf " + dstFile );

   // 测试目标文件不存在时指定权限
   this.file.copy( srcFile, dstFile, false, 0733 );
   var dstFileMode = this.file.stat( dstFile ).toObj().mode.slice( 0, 10 );
   assert.equal( dstFileMode, "rwx-wx-wx" )

   // 测试目标文件存在时设置权限无效，保留原文件权限
   this.file.copy( srcFile, dstFile, true, 0777 );
   var dstFileMode = this.file.stat( dstFile ).toObj().mode.slice( 0, 10 );
   assert.equal( dstFileMode, "rwx-wx-wx" );


   // 测试目标文件存在且isReplace为false时，拷贝失败

   try
   {
      this.file.copy( srcFile, dstFile, false );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_FE )
      {
         throw e;
      }
   }

   this.cmd.run( "rm -rf " + dstFile );

   this.release();
}

/******************************************************************************
*@Description : check mkdir
*@author      : Liang XueWang            
******************************************************************************/
function checkMkdir ( cmd, dirName )
{
   cmd.run( "ls -al " + dirName );
}

/******************************************************************************
*@Description : check move file
*@author      : Liang XueWang            
******************************************************************************/
function checkMove ( cmd, oldFile, newFile )
{
   try
   {
      cmd.run( "ls -al " + oldFile );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != 2 )
      {
         throw e;
      }
   }
   cmd.run( "ls -al " + newFile );
}

/******************************************************************************
*@Description : check copy file
*@author      : Liang XueWang            
******************************************************************************/
function checkCopy ( cmd, srcFile, dstFile )
{
   var tmp;
   tmp = cmd.run( "ls -al " + srcFile + " | awk '{print $1}'" ).split( "\n" );
   var mode1 = tmp[tmp.length - 2];
   tmp = cmd.run( "ls -al " + dstFile + " | awk '{print $1}'" ).split( "\n" );
   var mode2 = tmp[tmp.length - 2]
   assert.equal( mode1, mode2 );
}

/******************************************************************************
*@Description : check remove file
*@author      : Liang XueWang            
******************************************************************************/
function checkRemove ( cmd, fileName )
{
   try
   {
      cmd.run( "ls -al " + fileName );
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != 2 )
      {
         throw e;
      }
   }
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileOperation10833.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试创建目录，移动文件，复制文件，删除文件
      fts[i].testFileOperation();
      // 测试拷贝目录时指定权限
      fts[i].testCopyWithMode();
   }
}

