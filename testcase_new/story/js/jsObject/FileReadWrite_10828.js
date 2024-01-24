/******************************************************************************
*@Description : test js object File function: read write close seek
*               TestLink : 10828 读取File对象文件内容
*                          10829 向File对象写内容
*                          10830 设置文件指针位置
*                          10831 设置文件指针位置，超过边界
*                          10832 关闭文件
*@auhor       : Liang XueWang
******************************************************************************/

// 测试读写文件，偏移读，关闭文件
FileTest.prototype.testReadWrite = function()
{
   this.init();

   var content = generateContent( 'a', 1025 );
   this.file.write( content );     // 写文件

   this.file.seek( 0, 'b' );
   var readPart = this.file.read( 4 );   // 偏移读部分字符
   assert.equal( readPart, "aaaa" );

   this.file.seek( 0, 'b' );
   var readMax = this.file.read();       // 偏移读1024个字符
   assert.equal( readMax, generateContent( 'a', 1024 ) );

   var readRest = this.file.read();      // 读取剩余字符
   assert.equal( readRest, 'a' );

   this.file.close();      // 关闭文件
   checkClose( this.file );

   this.release();
}

// 测试偏移超出文件边界，仅文件头有边界
FileTest.prototype.testSeekBoundary = function()
{
   this.init();

   try
   {
      this.file.seek( -1, 'b' );  // 测试偏移超出文件头边界
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_INVALIDARG )
      {
         throw e;
      }
   }

   try
   {
      this.file.seek( 0, 'b' );
      this.file.write( "abcdefg" );   // 测试偏移到文件尾部读取
      this.file.seek( 0, 'e' );
      this.file.read();
      throw new Error( "should error" );
   } catch( e )
   {
      if( e.message != SDB_EOF )
      {
         throw e;
      }
   }

   this.release();
}

/******************************************************************************
*@Description : generate content: repeat ch size times like 'aaaaa...'
*@author      : Liang XueWang            
******************************************************************************/
function generateContent ( ch, size )
{
   var str = ch;
   for( var i = 0; i < size - 1; i++ )
      str += ch;
   return str;
}

/******************************************************************************
*@Description : check file close: write after close
*@author      : Liang XueWang            
******************************************************************************/
function checkClose ( file )
{
   assert.tryThrow( SDB_IO, function()
   {
      file.write( 'abcd' );
   } );
}


main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileReadAndWrite10828.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME, filename );   // 本地file对象
   var ft2 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试读写文件，偏移读
      fts[i].testReadWrite();

      // 测试偏移超过边界
      fts[i].testSeekBoundary();
   }
}

