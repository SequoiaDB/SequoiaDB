/******************************************************************************
*@Description : test js object File function: getInfo md5 stat
*               TestLink : 10815 获取File对象信息
*                          10826 File对象查看文件信息
*                          10827 计算File对象文件Md5值
*@auhor       : Liang XueWang
******************************************************************************/

// 测试获取文件对象信息
FileTest.prototype.testGetInfo = function()
{
   this.init();

   if( this.file === File )   // 检查文件是否为本地File类类型
   {
      this.release();
      return;
   }

   var info = this.file.getInfo().toObj();
   if( this.isLocal )       // 验证本地文件对象信息
   {
      if( info.type !== "File" || info.isRemote !== false ||
         info.filename !== this.filename )
      {
         throw new Error( "testGetInfo fail,check local file info" + this + JSON.stringify( info ) );
      }
   }
   else                      // 验证远程文件对象信息
   {
      checkRemoteFileInfo( info, this );
   }

   this.release();
}

// 测试文件MD5值
FileTest.prototype.testMd5 = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname )
   var fileName = sdbDir[0] + "/conf/sdbcm.conf";
   var md5 = this.file.md5( fileName );
   var tmp = this.cmd.run( "md5sum " + fileName ).split( "\n" );
   var tmpInfo = tmp[tmp.length - 2];
   var expect = tmpInfo.split( " " )[0];
   assert.equal( md5, expect );

   this.release();
}

// 测试查看文件信息
FileTest.prototype.testStat = function()
{
   this.init();

   var dir = "/tmp/testDir";         // 目录
   this.cmd.run( "rm -rf " + dir );
   this.cmd.run( "mkdir " + dir );
   var normalFile = "/etc/hosts";    // 普通文件
   var charFile = "/dev/null";       // 字符设备文件
   var blockFile = "/dev/loop0";     // 块设备文件
   var linkFile = "/dev/stdin";      // 链接文件
   var socketFile = "/dev/log";      // 套接字文件
   var fifoFile = "/tmp/testFifo";   // 命名管道文件
   this.cmd.run( "rm -rf " + fifoFile );
   this.cmd.run( "mkfifo " + fifoFile );
   // var unkowntypeFile ;
   // var files = [dir, normalFile, charFile, blockFile, linkFile, socketFile, fifoFile];
   // 删除一部分可能由于内容变化导致用例失败的文件，CI-2471
   var files = [normalFile, charFile];

   for( var i = 0; i < files.length; i++ )
   {
      try
      {
         var stat1 = this.file.stat( files[i] ).toObj();
         var command = "stat -c '%n|%s|%A|%U|%G|%x|%y|%z' " + files[i];
         var tmpInfo = this.cmd.run( command ).split( "\n" );
         var tmp = tmpInfo[tmpInfo.length - 2];
      }
      catch( e )
      {
         if( e.message == 1 )
         {
            continue;
         }
         throw e;
      }
      var stat2 = tmp.split( "|" );
      checkStat( stat1, stat2 );
   }
   this.cmd.run( "rm -rf " + fifoFile );
   this.cmd.run( "rm -rf " + dir );

   this.release();
}

/******************************************************************************
*@Description : check file stat result with stat command result
*@author      : Liang XueWang            
******************************************************************************/
function checkStat ( stat1, stat2 )
{
   var statObj = {};
   statObj["name"] = stat2[0];
   statObj["size"] = stat2[1];
   statObj["mode"] = stat2[2].slice( 1 );
   statObj["user"] = stat2[3];
   statObj["group"] = stat2[4];
   statObj["accessTime"] = stat2[5];
   statObj["modifyTime"] = stat2[6];
   statObj["changeTime"] = stat2[7];
   statObj["type"] = getFileTypeWithMode( stat2[2] );

   for( var k in stat1 )
   {
      assert.equal( statObj[k], stat1[k] );
   }
}

/******************************************************************************
*@Description : check remote file info
*@author      : Liang XueWang            
******************************************************************************/
function checkRemoteFileInfo ( info, ft )  // ft: FileTest对象
{
   if( info.type !== "File" || info.hostname !== ft.hostname ||
      info.svcname !== ft.svcname || info.isRemote !== true )
   {
      throw new Error( "checkRemoteFileInfo check file info" + ft + JSON.stringify( info ) );
   }
   if( ft.filename === undefined )
   {
      assert.equal( info.filename, undefined );
   }
   else
   {
      assert.equal( info.filename, ft.filename );
   }
}

/******************************************************************************
*@Description : get file type with file mode
*@author      : Liang XueWang            
******************************************************************************/
function getFileTypeWithMode ( mode )
{
   switch( mode[0] )
   {
      case '-':
         return "regular file";
      case 'd':
         return "directory";
      case 'c':
         return "character special file";
      case 'b':
         return "block special file";
      case 'l':
         return "symbolic link";
      case 's':
         return "socket";
      case 'p':
         return "pipe";
      default:
         return "unknow";
   }
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileInfo10815.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试获取file对象信息
      fts[i].testGetInfo();

      // 测试文件MD5
      fts[i].testMd5();

      // 测试查看文件信息
      fts[i].testStat();
   }
}

