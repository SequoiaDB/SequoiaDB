/******************************************************************************
*@Description : test js object System function: getSystemConfigs
*               TestLink : 10666 System对象获取系统配置信息
*@author      : Liang XueWang
******************************************************************************/

// 测试获取系统配置信息
SystemTest.prototype.testGetSystemConfigs = function()
{
   this.init();

   //var type = [ "kernel", "vm", "fs", "debug", "dev", "abi", "net", "all" ] ;
   var type = ["all"];
   // 动态变化的字段
   var except = ["fs.dentry-state", "fs.inode-nr", "fs.inode-state",
      "fs.file-nr", "kernel.ns_last_pid", "net.netfilter.nf_conntrack_count"];
   for( var i = 0; i < type.length; i++ )
   {
      var configObj = this.system.getSystemConfigs( type[i] ).toObj();
      var dir;
      if( type[i] === "all" )
         dir = "/proc/sys";
      else
         dir = "/proc/sys/" + type[i];
      var result = toolGetConfigs( this.cmd, dir );
      for( var k in result )
      {
         // 排除随机生成或动态变化的字段
         if( k.indexOf( "random" ) !== -1 ||
            k.indexOf( "max_newidle_lb_cost" ) !== -1 )
            continue;
         else if( getIndexInArray( k, except ) !== -1 )
            continue;
         else if( typeof ( configObj[k] ) !== "undefined" && configObj[k] !== result[k] )
         {
            throw new Error( "testGetSystemConfigs test key: " + k + " " + this + "res: " + result[k] + "config: " + configObj[k] );
         }
      }
   }

   this.release();
}

// 获取系统配置信息，从/proc/sys目录下的文件中获取
function toolGetConfigs ( cmd, dir )
{
   var configObj = {};
   try
   {
      var command = "find " + dir + " -type f -perm -o+r ! -mtime 0";
      var files = cmd.run( command ).split( "\n" );
   }
   catch( e )
   {
      if( e.message == 1 )
         return configObj;
      else
      {
         throw e;
      }
   }
   for( var i = 0; i < files.length - 1; i++ )
   {
      var filename = files[i];
      var tmpInfo = filename.split( "/" );
      var key = "";
      for( var j = 0; j < tmpInfo.length; j++ )
      {
         if( tmpInfo[j] === "" || tmpInfo[j] === "proc" ||
            tmpInfo[j] === "sys" )
            continue;
         key += tmpInfo[j];
         if( j !== tmpInfo.length - 1 )
            key += ".";
      }
      try
      {
         var fileContent = cmd.run( "cat " + filename ).split( "\n" );
      }
      catch( e )
      {
         if( e.message == 1 )
            continue;
         else
            throw e;
      }
      var value = "";
      for( var k = 0; k < fileContent.length - 1; k++ )
      {
         if( fileContent[k] === "" )
            continue;
         // 制表符替换成四个空格
         value += fileContent[k].replace( /\t/g, '    ' ) + ";";
      }
      // 删除最后一个分号
      value = value.substring( 0, value.length - 1 );
      configObj[key] = value;
   }
   return configObj;
}

// 查找元素在数组中的下标
function getIndexInArray ( a, arr )
{
   var index = -1;
   for( var i = 0; i < arr.length; i++ )
   {
      if( arr[i] === a )
      {
         index = i;
         break;
      }
   }
   return index;
}

main( test );

function test ()
{
   // 获取本地主机和远程主机
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();

   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   for( var i = 0; i < systems.length; i++ )
   {
      // 测试获取系统配置信息
      systems[i].testGetSystemConfigs();
   }
}
