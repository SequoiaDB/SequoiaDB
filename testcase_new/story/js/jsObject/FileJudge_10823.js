/******************************************************************************
*@Description : test js object File function: isFile isDir isEmptyDir Exist
*               TestLink : 10823 File对象判断是否文件
*                          10824 File对象判断是否目录
*                          10825 File对象判断是否是空目录
*                          10834 检查文件是否存在　
*@auhor       : Liang XueWang
******************************************************************************/
// 测试判断是否是文件
FileTest.prototype.testIsFile = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var fileName = sdbDir[0] + "/conf/sdbcm.conf";
   var notExist = sdbDir[0] + "/conf/notexist";

   var result = this.file.isFile( sdbDir[0] );   // 判断非文件
   assert.equal( result, false );
   result = this.file.isFile( fileName );       // 判断文件 
   assert.equal( result, true );
   try
   {
      this.file.isFile( notExist );      // 判断不存在的文件 
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   this.release();
}

// 测试判断是否是目录
FileTest.prototype.testIsDir = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var fileName = sdbDir[0] + "/conf/sdbcm.conf";
   var result = this.file.isDir( sdbDir[0] );    // 判断目录
   assert.equal( result, true );
   result = this.file.isDir( fileName );        // 判断非目录
   assert.equal( result, false );

   this.release();
}

// 测试判断是否是空目录
FileTest.prototype.testIsEmptyDir = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var emptyDir = "/tmp/emptydir";
   this.cmd.run( "mkdir -p " + emptyDir );
   var fileName = sdbDir[0] + "/conf/sdbcm.conf";

   var result = this.file.isEmptyDir( sdbDir[0] );  // 判断非空目录
   assert.equal( result, false );
   result = this.file.isEmptyDir( emptyDir );      // 判断空目录
   this.cmd.run( "rm -rf " + emptyDir );
   assert.equal( result, true );
   try
   {
      this.file.isEmptyDir( fileName );            // 判断非目录 
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   this.release();
}

// 测试文件是否存在
FileTest.prototype.testExist = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname );
   var existfile = sdbDir[0] + "/conf/sdbcm.conf";
   var notexist = sdbDir[0] + "/conf/sdb.conf";

   var result = this.file.exist( sdbDir[0] );   // 判断存在的目录
   assert.equal( result, true );
   result = this.file.exist( existfile );  // 判断存在的文件
   assert.equal( result, true );
   result = this.file.exist( notexist );   // 判断不存在的文件
   assert.equal( result, false );

   this.release();
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileJudge10823.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试判断是否是文件
      fts[i].testIsFile();

      // 测试判断是否是目录
      fts[i].testIsDir();

      // 测试判断是否是空目录
      fts[i].testIsEmptyDir();

      // 测试文件是否存在
      fts[i].testExist();
   }
}

