/******************************************************************************
*@Description : seqDB-10980:System.snapshotDiskInfo()增加磁盘IO统计信息 
*@Info          Test point: "ReadSec" 累计的读扇区数(UINT64)；"WriteSec" 累计的写扇区数(UINT64) 
*@Author      : 2019-3-19  XiaoNi Huang
******************************************************************************/

main( test );

function test ()
{
   var rc = System.snapshotDiskInfo();
   var disks = rc.toObj()["Disks"];

   // 尽可能获取挂载盘的磁盘信息（如"Filesystem": "/dev/sda2"），而不是"Filesystem": "tmpfs"
   var diskInfo = "";
   var fileSystem = "";
   for( var i = 0; i < disks.length; i++ )
   {
      diskInfo = disks[i];
      fileSystem = diskInfo["Filesystem"];
      if( fileSystem.indexOf( '/dev/' ) === 0 )
      {
         break;
      }
   }

   // check results
   readSec = diskInfo["ReadSec"];
   writeSec = diskInfo["WriteSec"];
   if( fileSystem.indexOf( '/dev/' ) === 0 )
   {
      if( readSec <= 0 || writeSec <= 0 )
      {
         throw new Error( "main fail,> 0 <= 0, \ndiskinfo: " + JSON.stringify( diskInfo ) );
      }
   }
   else
   {
      if( readSec < 0 || writeSec < 0 )
      {
         throw new Error( "main fail, >= 0 < 0 \ndiskinfo: " + JSON.stringify( diskInfo ) );
      }
   }
}