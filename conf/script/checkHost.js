/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

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
@description: get host info
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "IP": "192.168.20.165", "HostName": "rhel64-test8", "User": "root", "Passwd": "sequoiadb" } ;
   SYS_JSON:
   ENV_JSON:
@return
   RET_JSON: the format is: {"IP":"192.168.20.165","HostName":"rhel64-test8","OS":{"Distributor":"RedHatEnterpriseServer","Release":"6.4","Bit":64},"OM":{"Version":"1.8","Path":"/opt/sequoiadb/bin/","Port":"11790","Release":15348},"CPU":[{"ID":"","Model":"","Core":2,"Freq":"2.00GHz"}],"Memory":{"Model":"","Size":2887,"Free":174},"Disk":[{"Name":"/dev/mapper/vg_rhel64test8-lv_root","Mount":"/","Size":43659,"Free":35065,"IsLocal":false},{"Name":"/dev/sda1","Mount":"/boot","Size":460,"Free":423,"IsLocal":true},{"Name":"//192.168.20.10/files","Mount":"/mnt","Size":47836,"Free":29332,"IsLocal":false}],"Net":[{"Name":"lo","Model":"","Bandwidth":"","IP":"127.0.0.1"},{"Name":"eth0","Model":"","Bandwidth":"","IP":"192.168.20.165"}],"Port":[{"Port":"","CanUse":false}],"Service":[{"Name":"","IsRunning":false,"Version":""}],"Safety":{"Name":"","Context":"","IsRunning":false}} 
*/

var FILE_NAME_CHECK_HOST = "checkHost.js" ;
var disablePathArr = [ "/bin", "/boot", "/root", "/sbin", "/dev", "/etc", "/lib", "/tmp", "/media", "/sys" ] ;
var disableFileSystem = [ "none", "tmpfs" ] ;
var errMsg           = "" ;
var rc               = SDB_OK ;
var RET_JSON         = new Object() ;

RET_JSON[IP]         = "" ;
RET_JSON[HostName]   = "" ; 
RET_JSON[OS]         = "" ;
RET_JSON[OMA]        = "" ;
RET_JSON[CPU]        = "" ;
RET_JSON[Memory]     = "" ;
RET_JSON[Disk]       = "" ;
RET_JSON[Net]        = "" ;
RET_JSON[Port]       = "" ;
RET_JSON[Service]    = "" ;
RET_JSON[Safety]     = "" ;

function OMAOption()
{
   this.role   = "cm" ;
   this.mode   = "run" ;
   this.expand = true ;
}

function OMAFilter()
{
   this.type   = "sdbcm" ;
}

/* *****************************************************************************
@discretion: chech the disk is mounted on the system directoris or not
@author: Tanzhaobo
@parameter
   disk: object of disk
@return
   true for the disk can be use while false for not
***************************************************************************** */
function _checkDiskPath( disk )
{
   var path = disk[Mount] ;
   if (undefined == path )
   {
      return false ;
   }
   for ( var i = 0; i < disablePathArr.length; i++ )
   {
      if ( 0 == path.indexOf( disablePathArr[i] ) )
      {
         return false ;
      }
   }
   return true ;
}

function _checkDiskFileSystem( disk )
{
   var fileSystem = disk[Filesystem] ;
   if ( undefined == fileSystem )
   {
      return false ;
   }
   for ( var i = 0; i < disableFileSystem.length; i++ )
   {
      if ( fileSystem === disableFileSystem[i] )
      {
         return false ;
      }
   }
   return true ;
}

/* *****************************************************************************
@discretion: extract OM Agent's version, release number, path and port,
   when it has been installed
@author: Tanzhaobo
@parameter
   installInfoObj[object]:
   omaInfoObj[object]:
@return
   retObj[object]: an OMAInfo object
***************************************************************************** */
function _extractOMAInfo( installInfoObj, omaInfoObj )
{
   var retObj      = new OMAInfo() ;
   var installpath = "" ;
   
   if ( SYS_LINUX == SYS_TYPE )
   {  
      // 1. get OM Agent SdbUser
      try
      {
         retObj[SdbUser] = installInfoObj[SDBADMIN_USER] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get OM Agent's SdbUser" ;
         rc = GETLASTERROR() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 2. get OM Agent installed path
      try
      {
         installpath = installInfoObj[INSTALL_DIR] ;
         retObj[Path] = adaptPath(installpath ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get OM Agent's install path" ;
         rc = GETLASTERROR() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. get OM Agent's service
      try
      {
         if ( "undefined" == typeof(omaInfoObj) || null == omaInfoObj )
            exception_handle( SDB_INVALIDARG, "OM Agent's info is " + omaInfoObj ) ;
         retObj[Service] = omaInfoObj[SvcName3] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get OM Agent's service" ;
         rc = GETLASTERROR() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 4. get version
      var cmd = null ;
      var sdbcmprog = null ;
      var str = null ;
      var len = 0 ;
      var ben = 0 ;
      var end = 0 ;
      try
      {
         cmd = new Cmd() ;
         sdbcmprog = adaptPath( installpath ) + OMA_PROG_BIN_SDBCM ;
         str = cmd.run( sdbcmprog + " --version ", "", OMA_GTE_VERSION_TIME ) ;
         beg = str.indexOf( OMA_MISC_OM_VERSION ) ;
         end = str.indexOf( '\n' ) ;
         len = OMA_MISC_OM_VERSION.length ;
         retObj[Version] = str.substring( beg + len, end ) ;
      }
      catch ( e )
      {
         // version 1.8 sp1 and other versions older then 1.8 need this error msg
         if ( ( (1 == e) && (null == str) ) || ( SDB_TIMEOUT) == e )
            errMsg = "The OM Agent is not compatible" ;
         else
            errMsg = "Failed to get OM Agent's version" ;
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }

      // 5. get release
      var subStr = "" ;
      try
      {
         beg = str.indexOf( OMA_MISC_OM_RELEASE ) ;
         len = OMA_MISC_OM_RELEASE.length ;
         subStr = str.substring( beg + len, str.length ) ;
         retObj[Release] = parseInt( subStr ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get OM Agent's release number" ;
         rc = GETLASTERROR() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   else
   {
      // TODO:
   }
   return retObj ;
}

// om agent's status and version
function _getOMAInfo()
{
   var installInfoObj = null ;
   var installPath    = null ;
   var omaArr         = null ;
   var omaObj         = null ;
   var runNum         = 0 ;
   var localNum       = 0 ;

   // 1. get the amount of existed OM Agent and the OM Agent's info obj
   var option = new OMAOption() ;
   var filter = new OMAFilter() ;

   // check whether running OM Agent exists or not
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST,
           sprintf( "Check whether running OM Agent exists passes: " +
                    "option[?], filter[?]", JSON.stringify(option),
                    JSON.stringify(filter) ) ) ;
   omaArr = Sdbtool.listNodes( option, filter ) ;
   runNum = omaArr.size() ;
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST,
           sprintf( "The amount of running OM Agent in host[?] is [?]", System.getHostName(), runNum ) ) ;
   if ( 0 != runNum )
   {
      try
      {
         omaObj = eval( '(' + omaArr.pos() + ')' ) ;
         // get installed SequoiaDB info
         installInfoObj = getInstallInfoObj() ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to check whether OM Agent exists in host[?] or not", System.getHostName() ) ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   else
   {
      // get installed SequoiaDB info
      try
      {
         installInfoObj = getInstallInfoObj() ;
         installPath =  adaptPath( installInfoObj[INSTALL_DIR] ) + OMA_PATH_BIN ;
      }
      catch( e )
      {
         if ( SDB_FNE == e )
         {
            PD_LOG( arguments, PDWARNING, FILE_NAME_CHECK_HOST,
                    sprintf( "Take OM Agent does not exist in host[?]",
                             System.getHostName() ) ) ;
            RET_JSON[OMA] = new OMAInfo() ;
            return ;
         }
         else
         {
            SYSEXPHANDLE( e ) ;
            rc = GETLASTERROR() ;
            errMsg = sprintf( "Failed to get SequoiaDB install info in host[?]",
                              System.getHostName() ) ;
            PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
                    sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
            exception_handle( rc, errMsg ) ;
         }
      }
      // when no running OM Agent exists, get the amount of local OM Agent
      option["mode"] = "local" ;
      PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST,
              sprintf( "Check whether local OM Agent exists passes: " +
                       "option[?], filter[?], installPath[?]", JSON.stringify(option),
                       JSON.stringify(filter), installPath ) ) ;
      try
      {
         omaArr = Sdbtool.listNodes( option, filter, installPath ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to check whether local OM Agent exists in host[?] or not, " +
                           "take it not exists", System.getHostName() ) ;
         PD_LOG( arguments, PDWARNING, FILE_NAME_CHECK_HOST,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         RET_JSON[OMA] = new OMAInfo() ;
         return ;
      }
      localNum = omaArr.size() ;
      PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST,
              sprintf( "The amount of local OM Agent in host[?] is [?]", System.getHostName(), localNum ) ) ;
      if ( 0 != localNum )
      {
         omaObj = eval( '(' + omaArr.pos() + ')' ) ;
      }
   }

   // 3.get OM Agent info
   if ( 0 != runNum || 0 != localNum )
   {
      RET_JSON[OMA] = _extractOMAInfo( installInfoObj, omaObj ) ;
   }
   else
   {
      RET_JSON[OMA] = new OMAInfo() ;
   }
}

// os info
function _getOSInfo()
{
   var osInfo = new OSInfo() ;
   
   try
   {
      var obj             = eval( '(' + System.getReleaseInfo() + ')' ) ;
      osInfo[Distributor] = obj[Distributor] ;
      osInfo[Release]     = obj[Release] ;
      osInfo[Description] = obj[Description] ;
      osInfo[Bit]         = obj[Bit] ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get os's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      RET_JSON[OS] = new OSInfo() ;
      return ;
   }
   RET_JSON[OS] = osInfo ;
}

// memory
function _getMemInfo()
{
   var memInfo = new MemoryInfo() ;
   
   try
   {
      var obj = eval( '(' + System.getMemInfo() + ')' ) ;
      // TODO: model is not offer
      memInfo[Model]   = "" ;
      memInfo[Size]    = obj[Size] ;
      memInfo[Free]    = obj[Free] ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get memory's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      RET_JSON[Memory] = new MemoryInfo() ;
      return ;
   }
   RET_JSON[Memory] = memInfo ;
}

// disk
function _getDiskInfo()
{
   var objs = null ;
   var arr = null ;
   var diskInfos = [] ;
   try
   {
      objs      = eval( '(' + System.getDiskInfo() + ')' ) ;
      arr       = objs[Disks] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj           = arr[i] ;
         if ( _checkDiskPath( obj ) && _checkDiskFileSystem( obj ) )
         {
            var diskInfo      = new DiskInfo() ;
            diskInfo[Name]    = obj[Filesystem] ;
            diskInfo[Mount]   = obj[Mount] ;
            diskInfo[Size]    = obj[Size] ;
            diskInfo[Free]    = obj[Size] - obj[Used] ;
            diskInfo[IsLocal] = obj[IsLocal] ;
            diskInfos.push( diskInfo ) ;
         }
         else
         {
            PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST, 
                    "Give up using disk mounted on [" + obj[Mount] + "]"  ) ;
         }
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get disk's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
      RET_JSON[Disk] = [] ;
      return ;
   }
   
   RET_JSON[Disk] = diskInfos ;
}

// cpu
function _getCPUInfo()
{
   var objs = null ;
   var arr = null ;
   var cpuInfos = [] ;
   
   try
   {
      objs     = eval( '(' + System.getCpuInfo() + ')' ) ;
      arr      = objs[Cpus] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj        = arr[i] ;
         var cpuInfo    = new CPUInfo() ;
         // TODO: not offer ID and Model
         cpuInfo[ID]    = "" ;
         cpuInfo[Model] = obj[Info] ;
         cpuInfo[Core]  = obj[Core] ;
         cpuInfo[Freq]  = obj[Freq] ;
         cpuInfos.push( cpuInfo ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get cpu's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      RET_JSON[CPU] = [] ;
      return ;
   }
   
   RET_JSON[CPU] = cpuInfos ;
}

// net card
function _getNetCardInfo()
{
   var objs         = null ;
   var arr          = null ;
   var netcardInfos = [] ;
   try
   {
      objs         = eval( '(' + System.getNetcardInfo() + ')' ) ;
      arr          = objs[Netcards] ;
      for ( var i = 0; i < arr.length; i++ )
      {
         var obj                = arr[i] ;
         var netcardInfo        = new NetInfo() ;
         netcardInfo[Name]      = obj[Name] ;
         // TODO: not offer Model and bandwidth 
         netcardInfo[Model]     = "" ;
         netcardInfo[Bandwidth] = "" ;
         netcardInfo[IP]        = obj[Ip] ;
         netcardInfos.push( netcardInfo ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get network card's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      RET_JSON[Net] = [] ;
      return ;
   }
   RET_JSON[Net] = netcardInfos ;
}

// port status 
function _getPortInfo()
{
   // TODO: no any plan yet
   var portInfos = [] ;
   portInfos.push( new PortInfo() ) ;
   RET_JSON[Port] = portInfos ;
}

// service
function _getServiceInfo()
{
   // TODO: no any plan yet
   var svcInfos = [] ;
   svcInfos.push( new ServiceInfo() ) ;
   RET_JSON[Service] = svcInfos ;
}

// safety
function _getSafetyInfo()
{
   var safetyInfo = new SafetyInfo() ;
   var obj        = null ;
   try
   {
      // TODO: System.getIpTablesInfo does not offer any useful info
      obj =  eval( '(' + System.getIpTablesInfo() + ')' ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to get safety's information" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST,
              sprintf( errMsg + "rc: ?, detail: ?",
              GETLASTERROR(), GETLASTERRMSG() ) ) ;
      RET_JSON[Safety] = new SafetyInfo() ;
      return ;
   }

   RET_JSON[Safety] = safetyInfo ;
}

function main()
{
   RET_JSON[IP] = BUS_JSON[IP] ;
   RET_JSON[HostName] = BUS_JSON[HostName] ; 

   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST, "Begin to check host[" + RET_JSON[IP] + "]"  ) ;   
 
   // get local host info
   _getOMAInfo() ;
   _getOSInfo() ;
   _getCPUInfo() ;
   _getMemInfo() ;
   _getDiskInfo() ;
   _getNetCardInfo() ;
   _getPortInfo() ;
   _getServiceInfo() ;
   _getSafetyInfo() ;
   
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST, "Finish checking host[" + RET_JSON[IP] + "]" ) ;
   return RET_JSON ;
}

// execute
main() ;

