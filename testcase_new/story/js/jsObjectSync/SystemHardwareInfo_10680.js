/******************************************************************************
*@Description : test js object System function: getCpuInfo snapshotCpuInfo
*               getMemInfo snapshotMemInfo getDiskInfo snapshotDiskInfo
*               getNetcardInfo snapshotNetcardInfo
*               TestLink : 10680 System对象获取cpu信息
*                          10681 System对象获取cpu快照
*                          10682 System对象获取内存信息
*                          10683 System对象获取内存快照
*                          10684 System对象获取磁盘信息
*                          10685 System对象获取磁盘快照
*                          10686 System对象获取网卡信息
*                          10687 System对象获取网卡快照
*                          10980:System.snapshotDiskInfo()增加磁盘IO统计信息 
*@author      : Liang XueWang
******************************************************************************/

// 测试获取Cpu信息  /proc/cpuinfo 需要注意PPC的区别
SystemTest.prototype.testGetCpuInfo = function()
{
   this.init();

   var cpuInfo = this.system.getCpuInfo().toObj();
   var isppc = isPPC( this.hostname, this.svcname );

   // 测试物理cpu的个数
   checkCpuNum( this.cmd, cpuInfo, isppc );

   // 测试cpu名称
   checkCpuName( this.cmd, cpuInfo, isppc );

   // 测试cpu时间
   checkCpuTime( this.cmd, cpuInfo );

   this.release();
}

// 测试获取Cpu快照  getCpuInfo校验
SystemTest.prototype.testSnapshotCpuInfo = function()
{
   this.init();

   var cpuInfo1 = this.system.getCpuInfo().toObj();
   var cpuInfo2 = this.system.snapshotCpuInfo().toObj();
   for( var k in cpuInfo2 )
   {
      if( !isApproEqual( cpuInfo1[k], cpuInfo2[k] ) )
      {
         throw new Error( "testSnapshotCpuInfo fail,check key: " + k + " " + this + cpuInfo1[k] + cpuInfo2[k] );
      }
   }

   this.release();
}

// 测试获取内存信息
SystemTest.prototype.testGetMemInfo = function()
{
   this.init();

   var memInfo1 = this.system.getMemInfo().toObj();
   var command = "free -m | grep Mem | awk '{print $2,$3,$4}'";
   var tmpInfo = this.cmd.run( command ).split( "\n" )[0];
   var memInfo2 = tmpInfo.split( " " );
   var size = memInfo2[0];   // 内存总大小
   var used = memInfo2[1];   // 已使用内存大小
   var free = memInfo2[2];   // 空闲内存大小
   var unit = "M";
   if( !isApproEqual( size, memInfo1.Size ) ||
      unit !== memInfo1.Unit )
   {
      throw new Error( "testGetMemInfo fail,check mem info " + this + memInfo2 + JSON.stringify( memInfo1 ) );
   }
   if( !(memInfo1.Used >0 &&
        memInfo1.Free >0 &&
       memInfo1.Used + memInfo1.Free <= size ))
   {
      throw new Error( "testGetMemInfo fail,check mem info " + this + memInfo2 + JSON.stringify( memInfo1 ) );
   }

   this.release();
}

// 测试获取内存快照  getMemInfo校验
SystemTest.prototype.testSnapshotMemInfo = function()
{
   this.init();

   var memInfo1 = this.system.getMemInfo().toObj();
   var memInfo2 = this.system.snapshotMemInfo().toObj();
   for( var k in memInfo1 )
   {
      if( memInfo2[k] !== memInfo1[k] &&
         !isApproEqual( memInfo2[k], memInfo1[k] ) )
      {
         throw new Error( "testSnapshotMemInfo fail,check key: " + k + " " + this + memInfo1[k] + memInfo2[k] );
      }
   }

   this.release();
}

// 测试获取磁盘信息
SystemTest.prototype.testGetDiskInfo = function()
{
   this.init();

   var diskInfo = this.system.getDiskInfo().toObj();
   var diskFileContent = this.cmd.run( "cat /etc/mtab" ).split( "\n" );

   // 测试磁盘挂载点、文件系统等信息
   checkDiskInfo( diskInfo, diskFileContent );

   // 测试磁盘总容量，已使用容量
   var command = "df -mP | grep -v 'Filesystem\\|文件系统' | awk '{print $1,$2,$3,$4}'";
   var result = this.cmd.run( command ).split( "\n" );
   checkDiskSize( diskInfo, result );

   // 测试磁盘ReadSec WriteSec
   var result = getDiskIO( this.cmd );
   checkDiskIO( diskInfo, result, this.cmd );

   this.release();
}

// 测试获取磁盘快照  getDiskInfo校验
SystemTest.prototype.testSnapshotDiskInfo = function()
{
   this.init();

   var disks1 = this.system.getDiskInfo().toObj().disks;
   var disks2 = this.system.snapshotDiskInfo().toObj().disks;
   if( disks1 != disks2 )   // 对象比较，不使用全等
   {
      throw new Error( "testSnapshotDiskInfo fail,test disks " + this + JSON.stringify( disks1 ) + JSON.stringify( disks2 ) );
   }

   this.release();
}

// 测试获取网卡信息
SystemTest.prototype.testGetNetcardInfo = function()
{
   this.init();

   var netcards1 = this.system.getNetcardInfo().toObj().Netcards;
   var netcards2 = getNetcards( this.cmd ).Netcards;
   checkNetcards( netcards1, netcards2 );

   this.release();
}

// 测试获取网卡快照
SystemTest.prototype.testSnapshotNetcardInfo = function()
{
   this.init();

   var info1 = this.system.snapshotNetcardInfo().toObj();
   var command = "cat /proc/net/dev | grep -E -v 'Receive|bytes' | " +
      "sed 's/:/ /g' | awk '{print $1,$2,$3,$4,$5,$10,$11,$12,$13}'";
   var info2 = this.cmd.run( command ).split( "\n" );
   var timestamp = this.cmd.run( "date +%s" ).split( "\n" )[0];

   // 测试时间戳
   if( !isApproEqual( info1.CalendarTime, timestamp ) )
   {
      throw new Error( "testSnapshotNetcardInfo fail,test timestamp " + this + timestamp + info1.CalendarTime );
   }
   // 测试网卡收发消息
   for( var i = 0; i < info2.length - 1; i++ )
   {
      var tmp = info2[i].split( " " );
      var Name = tmp[0];          // 网卡名
      var RXBytes = tmp[1] * 1;     // 接收字节数
      var RXPackets = tmp[2] * 1;   // 接收数据包数
      var RXErrors = tmp[3] * 1;    // 接收数据包失败数
      var RXDrops = tmp[4] * 1;     // 接收数据包丢弃数
      var TXBytes = tmp[5] * 1;     // 发送字节数
      var TXPackets = tmp[6] * 1;   // 发送数据包数
      var TXErrors = tmp[7] * 1;    // 发送数据包失败数
      var TXDrops = tmp[8] * 1;     // 发送数据包丢弃数

      var netcard = info1.Netcards[i];
      if( Name !== netcard.Name ||
         !isApproEqual( RXBytes, netcard.RXBytes ) ||
         !isApproEqual( RXPackets, netcard.RXPackets ) ||
         !isApproEqual( RXErrors, netcard.RXErrors ) ||
         !isApproEqual( RXDrops, netcard.RXDrops ) ||
         !isApproEqual( TXBytes, netcard.TXBytes ) ||
         !isApproEqual( TXPackets, netcard.TXPackets ) ||
         !isApproEqual( TXErrors, netcard.TXErrors ) ||
         !isApproEqual( TXDrops, netcard.TXDrops ) )
      {
         throw new Error( "testSnapshotNetcardInfo fail,test netcard " + this + tmp + JSON.stringify( netcard ) );
      }
   }

   this.release();
}

/******************************************************************************
*@Description : check get cpu info  cpu num 
*@author      : Liang XueWang
******************************************************************************/
function checkCpuNum ( cmd, info, isppc )
{
   var cpuNum;    // 物理cpu个数
   if( isppc )
      cpuNum = cmd.run( "cat /proc/cpuinfo | grep machine | uniq |" +
         " wc -l" ).split( "\n" )[0] * 1;
   else
   {
      cpuNum = cmd.run( "cat /proc/cpuinfo | grep 'physical id' | uniq |" +
         " wc -l" ).split( "\n" )[0] * 1;
      if( cpuNum === 0 ) cpuNum = 1;
   }
   assert.equal( cpuNum, info.Cpus.length );
}

/******************************************************************************
*@Description : check get cpu info  cpu name 
*@author      : Liang XueWang
******************************************************************************/
function checkCpuName ( cmd, info, isppc )
{
   var cpuNames;
   if( isppc )
      cpuNames = cmd.run( "cat /proc/cpuinfo | grep cpu | uniq |" +
         " cut -d ':' -f 2 | sed 's/^ *//g'" ).split( "\n" );
   else
      cpuNames = cmd.run( "cat /proc/cpuinfo | grep 'model name' | uniq |" +
         " cut -d ':' -f 2 | sed 's/^ *//g'" ).split( "\n" );
   for( var i = 0; i < info.Cpus.length; i++ )
   {
      var cpuName = info.Cpus[i].Info;
      if( cpuNames.indexOf( cpuName ) === -1 )
      {
         throw new Error( "checkCpuName fail,test cpu name" + cpuName + cpuNames );
      }
   }
}

/******************************************************************************
*@Description : check get cpu info  sys name 
*@author      : Liang XueWang
******************************************************************************/
function checkCpuTime ( cmd, info )
{
   var times = cmd.run( "cat /proc/stat | head -n 1 |" +
      " awk '{for(i=2;i<9;i++) print $i}'" ).split( "\n" );
   // 单位：jiffies   1 jiffies = 0.01 second  (x86节拍为1秒100次)
   var userTime = times[0] * 1;     // 系统启动到当前时刻，用户态的Cpu时间 
   var niceTime = times[1] * 1;     // 系统启动到当前时刻，nice为负的Cpu时间
   var systemTime = times[2] * 1;   // 系统启动到当前时刻，核心时间
   var idleTime = times[3] * 1;     // 系统启动到当前时刻，除硬盘IO以外的其他等待时间
   var iowaitTime = times[4] * 1;   // 系统启动到当前时刻，硬盘IO的等待时间
   var irqTime = times[5] * 1;      // 系统启动到当前时刻，硬中断时间
   var softirqTime = times[6] * 1;  // 系统启动到当前时刻，软中断时间

   // 转为秒数后比较，微秒有误差（去除小数部分）
   if( !isApproEqual( ( userTime + niceTime ) / 100, info.User / 1000 ) )
   {
      throw new Error( "checkCpuTime fail,system's user time:" + ( userTime + niceTime ) / 100 + ",getCpuInfo's user time:" + info.User / 1000 );
   }
   if( !isApproEqual( systemTime / 100, info.Sys / 1000 ) )
   {
      throw new Error( "checkCpuTime fail,system's sys time:" + systemTime / 100 + ",getCpuInfo's sys time:" + info.Sys / 1000 );
   }
   if( !isApproEqual( idleTime / 100, info.Idle / 1000 ) )
   {
      throw new Error( "checkCpuTime fail,system's idle time" + idleTime / 100 + ",getCpuInfo's idle time:" + info.Idle / 1000 );
   }
   if( !isApproEqual( ( irqTime + softirqTime ) / 100, info.Other / 1000 ) )
   {
      throw new Error( "checkCpuTime fail,system's other time:" + ( irqTime + softirqTime ) / 100 + ",getCpuInfo's other time:" + info.Other / 1000 );
   }
}

/******************************************************************************
*@Description : check get disk info  Filesystem FsType Mount IsLocal
*@author      : Liang XueWang
******************************************************************************/
function checkDiskInfo ( info, content )
{
   var disks = info.Disks;
   for( var i = 0; i < disks.length; i++ )
   {
      var found = false;
      for( var j = 0; j < content.length; j++ )
      {
         if( content[j].indexOf( disks[i].Filesystem ) !== -1 &&   // 分区名
            content[j].indexOf( disks[i].FsType ) !== -1 &&       // 文件系统类型
            content[j].indexOf( disks[i].Mount ) !== -1 )         // 挂载点
         {
            found = true;
            break;
         }
      }
      if( found === false )
      {
         throw new Error( "checkDiskInfo fail,check disk info" + disks[i] + content );
      }
      if( disks[i].Filesystem.indexOf( "/dev" ) !== -1 &&
         disks[i].IsLocal !== true )
      {
         throw new Error( "checkDiskInfo fail,check IsLocal" + disks[i].Filesystem + disks[i].IsLocal );
      }
      if( disks[i].Filesystem.indexOf( "/dev" ) === -1 &&
         disks[i].IsLocal !== false )
      {
         throw new Error( "checkDiskInfo fail,check IsLocal" + disks[i].Filesystem + disks[i].IsLocal );
      }
   }
}

/******************************************************************************
*@Description : check get disk info  Size  Used  Unit
*@author      : Liang XueWang
******************************************************************************/
function checkDiskSize ( info, res )
{
   var disks = info.Disks;
   for( var i = 0; i < res.length - 1; i++ )
   {
      var tmp = res[i].split( " " );
      var fs = tmp[0];
      var size = tmp[1] * 1;
      var used = tmp[2] * 1;
      var avail = tmp[3] * 1;

      var found = false;
      for( var j = 0; j < disks.length; j++ )
      {
         if( fs === disks[j].Filesystem &&                    // 分区名
            isApproEqual( size, disks[j].Size ) &&          // 磁盘总大小
            isApproEqual( used, disks[j].Used ) &&    // 已使用磁盘大小
            "MB" === disks[j].Unit )
         {
            found = true;
            break;
         }
      }
      if( found === false )
      {
         throw new Error( "checkDiskSize fail,check disk size" + res[i] + JSON.stringify( disks ) );
      }
   }
}

/******************************************************************************
*@Description : get disk io stat  ReadSec WriteSec
*@author      : Liang XueWang
******************************************************************************/
function getDiskIO ( cmd )
{
   var command = "head -n 1 /proc/diskstats | awk '{print NF}'";
   var columns = cmd.run( command ).split( "\n" )[0];
   //系统内核版本不同，/proc/diskstats的列数不同
   if( columns >= "14" )
   {
      command = "cat /proc/diskstats | awk '{print $3,$6,$10}'";
   }
   else if( columns === "7" )
   {
      command = "cat /proc/diskstats | awk '{print $3,$5,$7}'";
   }
   else
   {
      throw new Error( "getDiskIO fail, the columns of /proc/diskstats is " + columns + " in current kernel version." );
   }
   var result = [];
   var tmpInfo = cmd.run( command ).split( "\n" );
   for( var i = 0; i < tmpInfo.length - 1; i++ )
   {
      var diskstats = tmpInfo[i].split( " " );
      result[i] = {};
      result[i].diskName = diskstats[0];    // 磁盘名
      result[i].readSec = diskstats[1];     // 读扇区次数
      result[i].writeSec = diskstats[2];    // 写扇区次数
   }
   return result;
}

/******************************************************************************
*@Description : check get disk info  ReadSec WriteSec
*@author      : Liang XueWang
******************************************************************************/
function checkDiskIO ( info, result, cmd )
{
   var disks = info.Disks;
   for( var i = 0; i < disks.length; i++ )
   {
      if( disks[i].IsLocal === false )
      {
         if( disks[i].ReadSec !== 0 || disks[i].WriteSec !== 0 )
         {
            throw new Error( "checkDiskIO fail,check ReadSec WriteSec local false" + "0,0" + JSON.stringify( disks[i] ) );
         }
      }
      else
      {
         var found = false;
         for( var j = 0; j < result.length; j++ )
         {
            var fs = readlink( cmd, disks[i].Filesystem );
            if( fs === "/dev/" + result[j].diskName ||
               fs === result[j].diskName )
            {
               found = true;
               if( !isApproEqual( disks[i].ReadSec, result[j].readSec ) ||
                  !isApproEqual( disks[i].WriteSec, result[j].writeSec ) )
               {
                  throw new Error( "checkDiskIO fail,check ReadSec WriteSec" + JSON.stringify( result[j] ) + JSON.stringify( disks[i] ) );
               }
            }
         }
         if( found === false )
         {
            throw new Error( "checkDiskIO fail,check ReadSec WriteSec local true" + JSON.stringify( disks[i] ) + JSON.stringify( result ) );
         }
      }
   }
}

/******************************************************************************
*@Description : redlink disk file system
*@author      : Liang XueWang
******************************************************************************/
function readlink ( cmd, fs )
{
   var command = "readlink -f " + fs;
   try
   {
      var info = cmd.run( command ).split( "\n" )[0];
   }
   catch( e )
   {
      if( e.message == 1 ) return fs;
      else
      {
         println( "throw e: " + e );
         throw e;
      }
   }
   var ind = info.lastIndexOf( "/" );
   return info.slice( ind + 1 );
}

/******************************************************************************
*@Description : get netcard name and ip,return object
*@author      : Liang XueWang
******************************************************************************/
function getNetcards ( cmd )
{
   var obj = {};
   obj.Netcards = [];

   var names = cmd.run( "cat /proc/net/dev | grep : | cut -d : -f 1" ).split( "\n" );
   var ips = [];
   var k = 0;
   for( var i = 0; i < names.length - 1; i++ )
   {
      names[i] = names[i].replace( /[\t ]/g, '' );
      var command = "ip addr show " + names[i] + " | grep inet | grep -v inet6"
         + " | awk '{print $2}'";
      var tmpInfo = cmd.run( command ).split( "\n" );
      for( var j = 0; j < tmpInfo.length - 1; j++ )
      {
         var ind = tmpInfo[j].indexOf( "/" );
         var ip = tmpInfo[j].slice( 0, ind );
         obj.Netcards[k] = {};
         obj.Netcards[k].Name = names[i];
         obj.Netcards[k].Ip = ip;
         k++;
      }
   }
   return obj;
}

/******************************************************************************
*@Description : get netcard name and ip,return object
*@author      : Liang XueWang
******************************************************************************/
function checkNetcards ( netcards1, netcards2 )
{
   assert.equal( netcards1.length, netcards2.length );
   for( var i = 0; i < netcards1.length; i++ )
   {
      var found = false;
      for( var j = 0; j < netcards2.length; j++ )
      {
         if( netcards1[i].toString() === netcards2[j].toString() )
         {
            found = true;
            break;
         }
      }
      if( found === false )
      {
         throw new Error( "checkNetcards fail,check netcard info" + JSON.stringify( netcards1[i] ) + JSON.stringify( netcards2 ) );
      }
   }
}

main( test );

function test ()
{
   var localhost = toolGetLocalhost();
   var remotehost = toolGetRemotehost();
   var localSystem = new SystemTest( localhost, CMSVCNAME );
   var remoteSystem = new SystemTest( remotehost, CMSVCNAME );
   var systems = [localSystem, remoteSystem];

   for( var i = 0; i < systems.length; i++ )
   {
      systems[i].testGetCpuInfo();

      systems[i].testSnapshotCpuInfo();

      systems[i].testGetMemInfo();

      systems[i].testSnapshotMemInfo();

      systems[i].testGetDiskInfo();

      systems[i].testSnapshotDiskInfo();

      systems[i].testGetNetcardInfo();

      systems[i].testSnapshotNetcardInfo();
   }
}

