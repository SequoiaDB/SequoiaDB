/******************************************************************************
*@Description : test js object File function: getUmask setUmask
*               TestLink : 10820 设置File对象的掩码
*                          10821 获取文件默认掩码
*@auhor       : Liang XueWang
******************************************************************************/

// 测试获取文件默认权限的掩码
FileTest.prototype.testGetUmask = function()
{
   this.init();

   var umask1 = this.file.getUmask( '8' );    // 获取掩码
   var tmpInfo = this.cmd.run( "umask" ).split( "\n" );
   var umask2 = tmpInfo[tmpInfo.length - 2];
   assert.equal( umask1, umask2 );

   this.release();
}


// 测试设置文件默认权限的掩码
FileTest.prototype.testSetUmask = function()
{
   this.init();

   var oldUmask = this.file.getUmask( '8' );

   this.file.setUmask( 0222 );   // 设置掩码
   var umask = this.file.getUmask( '8' );
   assert.equal( umask, "0222" );

   var tmpFilename = "/tmp/testUmask.txt";   // 新建文件，检查掩码生效
   var tmpFile;
   if( this.isLocal )
      tmpFile = new File( tmpFilename );
   else
      tmpFile = this.remote.getFile( tmpFilename );
   var command = "ls -l " + tmpFilename + " | awk '{print $1}'";
   var tmpInfo = this.cmd.run( command ).split( "\n" );
   var mode = tmpInfo[tmpInfo.length - 2].slice( 0, 10 );
   this.cmd.run( "rm -rf " + tmpFilename );
   assert.equal( mode, "-r-x------" );

   this.file.setUmask( parseInt( oldUmask, 8 ) );
   this.release();
}

// 测试使用不同进制获取Umask
FileTest.prototype.testGetUmaskWithBase = function()
{
   this.init();

   var oldUmask = this.file.getUmask( '8' );

   this.file.setUmask( 0222 );
   var umask = this.file.getUmask();
   if( umask !== 146 || typeof ( umask ) !== "number" )
   {
      throw new Error( "getUmaskWithBase get umask with no parameter " + this + 146 + umask );
   }

   var base = ['8', '10', '16', 8, 10, 16];
   var result = ['0222', '146', '0x92', '0222', '146', '0x92'];
   for( var i = 0; i < base.length; i++ )
   {
      umask = this.file.getUmask( base[i] );
      if( umask !== result[i] || typeof ( umask ) !== "string" )
      {
         throw new Error( "getUmaskWithBase get umask with base " + base[i] + " " + this + result[i] + umask );
      }
   }

   this.file.setUmask( parseInt( oldUmask, 8 ) );
   this.release();
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileUmask10820.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试获取文件权限默认掩码
      fts[i].testGetUmask();

      // 测试设置文件权限默认掩码
      fts[i].testSetUmask();

      // 测试使用不同进制获取掩码
      fts[i].testGetUmaskWithBase();
   }
}

