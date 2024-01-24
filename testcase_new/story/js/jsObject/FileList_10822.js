/******************************************************************************
*@Description : test js object File function: list
*               TestLink : 10822 File对象列出特定目录的清单
*@auhor       : Liang XueWang
******************************************************************************/
// 测试枚举目录中的文件
FileTest.prototype.testList = function()
{
   this.init();

   // make dir to list
   var dirName = adaptPath( WORKDIR ) + "10822/";
   var dirMode = 0755;
   var fileNum = 5;
   var fileMode = 0644;
   var remote = new Remote( this.hostname, this.svcname );
   var file = remote.getFile();
   file.mkdir( dirName, dirMode );
   for( var i = 0; i < fileNum; i++ )
   {
      var fileName = dirName + fileNum + ".txt";
      var file1 = remote.getFile( fileName, fileMode );
      file1.write( "abcde" );
      file1.close();
   }

   // list dir
   var option = {};
   option["pathname"] = dirName;
   option["detail"] = true;
   var files1 = this.file.list( option ).toArray();   // 枚举文件
   var command = "ls -al " + dirName + " | sed -n '4,$p' | awk '{print $1,$3,$9}'";
   var files2 = this.cmd.run( command ).split( "\n" );
   for( var i = files1.length - 1, j = files2.length - 2; i >= 0; i-- , j-- )
   {
      var fileObj = JSON.parse( files1[i] );
      var tmp = files2[j].split( " " );
      var perm = tmp[0];   // 文件权限
      var user = tmp[1];   // 文件用户
      var filename = tmp[2];  // 文件名
      if( perm !== fileObj.mode || user !== fileObj.user ||
         filename !== fileObj.name )
      {
         throw new Error( "testList test list files in " + dirName + " " + this + tmp + files1[i] );
      }
   }

   // remove dir
   file.remove( dirName );
   remote.close();

   this.release();
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileList10822.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试枚举目录中的文件
      fts[i].testList();
   }
}

