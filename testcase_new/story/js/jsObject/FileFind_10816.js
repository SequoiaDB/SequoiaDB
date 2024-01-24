/******************************************************************************
*@Description : test js object File function: find
*               TestLink : 10816 查找File对象的路径
*@auhor       : Liang XueWang
******************************************************************************/
// 测试查找文件
FileTest.prototype.testFind = function()
{
   this.init();

   var tmpObj = toolGetCmUserGroup( this.hostname, this.svcname );
   var user = tmpObj.user;
   var group = tmpObj.group;
   var values = ["sdbcm.conf", user, "0755", group];
   var modes = ["n", "u", "p", "g"];   // 按文件名、用户、文件权限、用户组查找
   var args = ["-name", "-user", "-perm", "-group"];
   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname )
   var path = sdbDir[0] + "/conf";

   var options = [];    // 查找选项
   var commands = [];   // 查找命令
   for( var i = 0; i < 4; i++ )
   {
      options[i] = {};
      options[i].value = values[i];
      options[i].mode = modes[i];
      options[i].pathname = path;
      commands[i] = "find " + path + " " + args[i] + " " + values[i];
   }

   for( var i = 0; i < options.length; i++ )
   {
      var result = this.file.find( options[i] ).toArray();  // 查找文件
      checkFindResult( result, this.cmd, commands[i] );
   }

   this.release();
}

// 测试不指定value参数时查找
FileTest.prototype.testFindWithoutValue = function()
{
   this.init();

   var sdbDir = toolGetSequoiadbDir( this.hostname, this.svcname )
   var path = sdbDir[0] + "/conf";

   var option = {};    // 查找选项
   option.pathname = path;
   var commands = "find " + path;  // 查找命令

   var result = this.file.find( option ).toArray();  // 查找文件
   checkFindResult( result, this.cmd, commands );

   this.release();
}

/******************************************************************************
*@Description : check find result
*@author      : Liang XueWang            
******************************************************************************/
function checkFindResult ( result, cmd, commands )
{
   if( result.length === 0 )
   {
      try
      {
         cmd.run( commands );
         throw new Error( "should error" );
      } catch( e )
      {
         if( e.message != SDB_FE )
         {
            throw e;
         }
      }
   }
   else
   {
      var findfiles = cmd.run( commands ).split( "\n" );
      for( var i = result.length - 1, j = findfiles.length - 2; i >= 0; i-- , j-- )
      {
         var fileObj = JSON.parse( result[i] );
         assert.equal( fileObj.pathname, findfiles[j] );
      }
   }
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var filename = "/tmp/testFileFind10816.txt";
   var ft1 = new FileTest( localhost, CMSVCNAME );     // 本地File类类型
   var ft2 = new FileTest( localhost, CMSVCNAME, filename );  // 本地file对象
   var ft3 = new FileTest( remotehost, CMSVCNAME );    // 远程File类类型
   var ft4 = new FileTest( remotehost, CMSVCNAME, filename );  // 远程file对象

   var fts = [ft1, ft2, ft3, ft4];

   for( var i = 0; i < fts.length; i++ )
   {
      // 测试查找文件
      fts[i].testFind();
      // 测试不指定value查找文件
      fts[i].testFindWithoutValue();
   }
}

