/*******************************************************************************

   Copyright (C) 2012-2018 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

*******************************************************************************/
/*
@description: query host status
@modify list:
   2014-9-30 Youbin Lin  Init

@command:
   ./sdb -f "../conf/script/define.js; ../conf/script/queryHostStatusItem.js ;../conf/script/queryHostStatus.js " -e "var BUS_JSON={HostName:'rhelt10', Disk:[{Name:'/dev/sda1'}], Net:[{Name:'eth1'}]} "

@parameter
   BUS_JSON: {HostName:'rhelt10', Disk:[{Name:'/dev/sda1'], Net:[{Name:'eth1'}]}
   SYS_JSON:
   ENV_JSON:
@return
   RET_JSON the result is as :{"HostName":"rhelt10","CPU":{"Sys":49944310,"Idle":25696260350,"Other":70182760,"User":204703470},"Memory":{"Size":3833,"Free":552,"Used":3281},"Disk":[{"Name":"/dev/sda1","Mount":"/","Size":34532,"Free":21441}],"Net":{"CalendarTime":1412059081,"Net":[{"Name":"eth1","RXBytes":4773973643,"RXPackets":12958460,"RXErrors":0,"RXDrops":0,"TXBytes":11956800291,"TXPackets":3906739,"TXErrors":0,"TXDrops":0}]}}
*/

var FILE_NAME_QUEYR_HOST_STATUS = "queryHostStatus.js" ;
var errMsg           = "" ;
var rc               = SDB_OK ;
var RET_JSON         = new Object() ;
RET_JSON[HostName]   = "" ;
RET_JSON[CPU]        = {} ;
RET_JSON[Memory]     = {} ;
RET_JSON[Disk]       = [] ;
RET_JSON[Net]        = {} ;

function snapshotCPUInfo()
{
   var obj     = null ;
   var cpuInfo = new CPUSnapInfo() ;
   try
   {
      obj =  eval( '(' + System.snapshotCpuInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to snapshot cpu info" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   cpuInfo[User]       = obj[User] ;
   cpuInfo[Sys]        = obj[Sys] ;
   cpuInfo[Idle]       = obj[Idle] ;
   cpuInfo[Other]      = obj[Other] ;
   RET_JSON[CPU]       = cpuInfo ;
}

function snapshotMemoryInfo()
{
   var obj  = null ;
   var Info = new MemorySnapInfo() ;
   try
   {
      obj  = eval( '(' + System.snapshotMemInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to snapshot memory info" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   Info[Size]          = obj[Size] ;
   Info[Used]          = obj[Used] ;
   Info[Free]          = obj[Free] ;
   RET_JSON[Memory]    = Info ;
}

function snapshotNetInfo()
{
   var snapshotObj               = null ;
   var listObj                   = null ;
   var netResult               = [] ;
   try
   {
      snapshotObj = eval( '(' + System.snapshotNetcardInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to snapshot net card info" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   var arr = snapshotObj[Netcards] ;
   // Get netcard info
   try
   {
      listObj = eval( '(' + System.getNetcardInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get net card info" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   var listInfoArr = listObj[Netcards] ;
   for ( var i = 0; i < arr.length; i++ )
   {
      var oneNet    = arr[i] ;
      var expectArr = BUS_JSON[Net] ;
      for ( var j = 0; j < expectArr.length; j++ )
      {
         var oneExpect = expectArr[j] ;
         if ( oneNet[Name] == oneExpect[Name] )
         {
            oneNet[IP] = null ;
            // add ip address into oneNet object
            for( var k = 0; k < listInfoArr.length; k++ )
            {
               var infoActual = listInfoArr[ k ] ;
               if( oneNet[Name] == infoActual[Name] )
               {
                  oneNet[IP] = infoActual[Ip] ;
                  break ;
               }
            }
            netResult.push( oneNet ) ;
         }
      }
   }

   var results           = {} ;
   results[CalendarTime] = obj[CalendarTime] ;
   results[Net]          = netResult ;
   RET_JSON[Net]         = results ;
}

function snapshotDiskInfo()
{
   var obj             = null ;
   var results         = [] ;
   try
   {
      obj = eval( '(' + System.snapshotDiskInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to snapshot disk info" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   var arr = obj[Disks] ;
   for ( var i = 0; i < arr.length; i++ )
   {
      var oneDisk   = arr[i] ;
      var expectArr = BUS_JSON[Disk] ;
      for ( var j = 0; j < expectArr.length; j++ )
      {
         var oneExpect = expectArr[j] ;
         if ( oneDisk[Filesystem] == oneExpect[Name] )
         {
            // find the expect disk
            var formated      = new DiskSnapInfo() ;
            formated[Name]    = oneDisk[Filesystem] ;
            formated[Mount]   = oneDisk[Mount] ;
            formated[Size]    = oneDisk[Size] ;
            formated[Free]    = oneDisk[Size] - oneDisk[Used] ;
            formated[ReadSec] = oneDisk[ReadSec] ;
            formated[WriteSec] = oneDisk[WriteSec] ;
            results.push( formated ) ;
         }
      }
   }

   RET_JSON[Disk]      = results ;
}

function main()
{
   try
   {
      RET_JSON[HostName] = BUS_JSON[HostName] ;
      snapshotCPUInfo() ;
      snapshotMemoryInfo() ;
      snapshotNetInfo() ;
      snapshotDiskInfo() ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to query host status" ;
      rc = GETLASTERROR() ;
      PD_LOG( arguments, PDERROR, FILE_NAME_QUEYR_HOST_STATUS,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }

   return RET_JSON ;
}


main() ;

