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
@description: check whether the for install znodes is ok
@modify list:
   2015-6-5 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}] } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "", "Result":[{"HostName":"susetzb", "zooid": "1", "errno": 0, "detail": ""},{"HostName":"rhel64-test8", "zooid": "2", "errno": 0, "detail": ""},{"HostName":"rhel64-test8", "zooid": "3", "errno": 0, "detail": ""}] } ;
*/

// println
//var BUS_JSON = { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper/zoo1/database/11870", "datapath":"/opt/zookeeper/zoo1/database/11870/mydata", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper/zoo1/database/11877", "datapath":"/opt/zookeeper/zoo1/database/11877/mydata", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper/zoo1/database/11884", "datapath":"/opt/zookeeper/zoo1/database/11884/mydata", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus"} ] } ;

//var SYS_JSON = { "TaskID": 1 } ;

function checkZNEnvResult()
{
   this.HostName         = "" ;
   this.zooid            = "0" ;
   this.errno            = SDB_OK ;
   this.detail           = "" ;
}

var FILE_NAME_CHECK_ZN_ENV = "checkZNEnv.js" ;
var RET_JSON        = new commonResult() ;
var rc              = SDB_OK ;
var errMsg          = "" ;
RET_JSON[Result]    = [] ;

var task_id         = "" ;
var user_tag        = "add by om agent" ;
var zkServer        = "" ;
var cfgFile         = "" ;
if ( SYS_LINUX == SYS_TYPE )
{
   zkServer = "bin/zkServer.sh" ;
   cfgFile  = "conf/zoo.cfg" ;
}
else
{
   // TODO: windows
}

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   // 1. get task id
   task_id = getTaskID( SYS_JSON ) ;

   // 2. specify the log file name
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZN_ENV,
            sprintf( "Begin to check the environment for installing zookeeper in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZN_ENV,
            sprintf( "Finish checking the environment for installing zookeeper in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: check whether zookeeper progress exists or not
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
@return
   [bool]: true or false
***************************************************************************** */
function _checkProgExist( ssh )
{
   var progExisted = false ;
   var str         = "" ;
   var retStr      = "" ;
   var progNum     = "" ;
   
   if ( SYS_LINUX == SYS_TYPE )
   {
      str = " ps -elf | grep zookeeper | grep -v grep -c " ;
      try
      { 
         try{ ssh.exec( str ) ; }catch(e){}
         progNum = parseInt( ssh.getLastOut() ) ;
         if ( true == isNaN( progNum ) )
         {
            PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV,
                     "The amount of progress about zookeeper is NaN" ) ;
            throw SDB_SYS ;
         }
         else
         {
            if ( progNum > 0 )
            {
               progExisted = true ;
               PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZN_ENV,
                        sprintf( "There ? progress(es) about zookeeper is(are) running in host[?]",
                                 progNum, ssh.getPeerIP() ) ) ;
            }
         }
      }
      catch(e)
      {
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV,
                  "Failed to get the amount of progress about zookeeper" ) ;
      }
   }
   else
   {
      // TODO: windows
   }
   
   return progExisted ;
}

/* *****************************************************************************
@discretion: check whether install path and data path exist and thay are not empty
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: the install path
   dataPath[string]: the data path
@return
   [bool]: true or false
***************************************************************************** */
function _fileExsited( ssh, file )
{
   var fileExisted = false ;
   var str         = "" ;
   var ret         = 0 ;

   str = " ls -l " + file ;
   try{ ssh.exec( str ) ; }catch(e){}
   ret = ssh.getLastRet() ;
   if ( 0 == ret )
   {
      fileExisted = true ;
      errMsg = sprintf( "File[?] exists in host[?]", file, ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
   }

   return fileExisted ;
}

function _checkFileExist( ssh, installPath, dataPath )
{
   var fileExisted = false ;
   var file1 = "" ;
   var file2 = "" ;
   var fileExisted1 = false ;
   var fileExisted2 = false ;

   if ( SYS_LINUX == SYS_TYPE )
   {      
      file1 = adaptPath(installPath) + "*" ;
      file2 = adaptPath(dataPath) + "*" ;
   }
   else
   {
      // TODO: windows
   }
   fileExisted1 = _fileExsited( ssh, file1 ) ;
   fileExisted2 = _fileExsited( ssh, file2 ) ;
   if ( true == fileExisted1 )
   {
      fileExisted = true ;
      errMsg = sprintf( "Path[?] exists in host[?], but it is not empty", installPath, ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
   }
   if ( true == fileExisted2 )
   {
      fileExisted = true ;
      errMsg = sprintf( "Path[?] exists in host[?], but it is not empty", dataPath, ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
   }
   
   return fileExisted ;
}

/* *****************************************************************************
@discretion: check whether the files are installed by agent
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: the install path
   clusterName[string]: the cluster name for the installing znode
   businessName[string]: the business name for the installing znode
   userTag[string]: the user tag for the installing znode
@return
   [bool]: true or false
***************************************************************************** */
function _addByAgent( ssh, installPath, clusterName, businessName, userTag )
{
   var addByAgent     = false ;
   var obj            = null ;
   var remote_file    = "" ;
   var local_tmp_file = "" ;
   var str            = "" ;
   var remote_business_name = "" ;
   var remote_cluster_name  = "" ;
   var remote_usertag       = "" ;
   
   if ( SYS_LINUX == SYS_TYPE )
   {
      remote_file = adaptPath( installPath ) + cfgFile ;
      local_tmp_file = "/tmp/zoo.cfg.tmp" ;
   }
   else
   {
      // TODO: windows
   }
   // get znode conf file from remote
   try
   {
      ssh.pull( remote_file, local_tmp_file ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed get znode's conf file from host[?] to check", ssh.getPeerIP() ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      try{ File.remove( local_tmp_file ) ; }catch(e){}
      exception_handle( rc, errMsg ) ;
   }
   
   // check the info( clustername, businessname, usertag )
   try
   {
      obj = eval( '(' + Oma.getOmaConfigs( local_tmp_file ) + ')' ) ;
      remote_business_name = obj[BusinessName3] ;
      remote_cluster_name  = obj[ClusterName3] ;
      remote_usertag       = obj[UserTag3] ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to analysis znode[?]'s info", ssh.getPeerIP() ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;
      try{ File.remove( local_tmp_file ) ; }catch(e){}
      exception_handle( rc, errMsg ) ;
   }
   PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_CHECK_ZN_ENV,
            sprintf( "Info for the installing znode is: ?, ?, ?",
                     clusterName, businessName, userTag ) ) ;
   PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_CHECK_ZN_ENV,
            sprintf( "Info in znode[?]'s conf file is: ?, ?, ?", ssh.getPeerIP(),
                     remote_cluster_name, remote_business_name, remote_usertag ) ) ;
   if ( clusterName != remote_cluster_name ||
        businessName != remote_business_name ||
        userTag != remote_usertag )
   {
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV,
               sprintf( "The existed znode in host[?] was not installed by om agent",
                        ssh.getPeerIP() ) ) ;
      addByAgent = false ;
   }
   else
   {
      addByAgent = true ;
   }
   
   return addByAgent ;
   
}

/* *****************************************************************************
@discretion: 
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: the install path
   dataPath[string]: the data path
@return
   [bool]: true or false
***************************************************************************** */
function _cleanUpZNode( ssh, installPath, dataPath )
{
   var str1 = "" ;
   var str2 = "" ;
   var ret1 = 0 ;
   var ret2 = 0 ;

   // stop znode
   str1 = adaptPath( installPath ) + zkServer + " stop " ;
   try{ ssh.exec( str1 ) ; }catch( e ){}
   ret1 = ssh.getLastRet() ;
   if ( 0 != ret1 )
   {
      errMsg = sprintf( "Failed to stop znode in host[?]", ssh.getPeerIP() ) ;
      rc = SDB_SYS ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( errMsg + ", rc: ?, detail: ?", ret1, ssh.getLastOut() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // remove it
   str1 = " rm -rf " + installPath ;
   str2 = " rm -rf " + dataPath ;
   try{ ssh.exec( str1 ) ; }catch( e ){}
   ret1 = ssh.getLastRet() ;
   try{ ssh.exec( str2 ) ; }catch( e ){}
   ret2 = ssh.getLastRet() ;
   if ( 0 != ret1 || 0 != ret2 )
   {
      errMsg = sprintf( "Failed to remove existed znode's install or data path in host[?]", ssh.getPeerIP() ) ;
      rc = SDB_SYS ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( errMsg + ", rc: ?, detail: ?", ret1, ssh.getLastOut() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: check the znode's status
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: the install path
@return void
***************************************************************************** */
function _checkStatus( ssh, installPath )
{
   var str = "" ;
   var ret = 0 ;

   str = adaptPath( installPath ) + zkServer + " status " ;
   for( var i = 0; i < OMA_WAIT_ZN_TRY_TIMES; i++ )
   {
      try{ ssh.exec( str ) ; }catch( e ){}
      ret = ssh.getLastRet() ;
      if ( 0 != ret )
      {
         sleep( OMA_SLEEP_TIME ) ;
         continue ;
      }
      break ;
   }
   if ( 0 != ret )
   {
      errMsg = sprintf( "zookeeper node is not ready in host[?]", ssh.getPeerIP() ) ;
      rc = SDB_SYS ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var hostName    = null ;
   var user        = null ;
   var passwd      = null ;
   var sdbUser     = null ;
   var sshPort     = null ;
   var installPath = null ;
   var dataPath    = null ;
   var zooID       = null ;
   var ssh         = null ;
   var deployMod   = null ;
   var serverInfo  = null ;
   var isCluster   = false ;
   var hasFailed   = false ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         deployMod  = BUS_JSON[DeployMod] ;
         serverInfo = BUS_JSON[ServerInfo] ;
         if ( OMA_DEPLOY_CLUSTER == deployMod )
         {
            isCluster = true ;
         }
         else
         {
            isCluster = false ;
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      
      for ( var i = 0; i < serverInfo.length; i++ )
      {
         var fileExisted = false ;
         var progExisted = false ;
         var addByAgent  = false ;
         var elem        = null ;
         var result      = new checkZNEnvResult() ;
         result[Errno]   = SDB_OK ;
         result[Detail]  = "" ;
         try
         {
            elem = serverInfo[ i ] ;
            
            result[HostName] = elem[HostName] ;
            result[ZooID3]   = elem[ZooID3] ;
            hostName         = elem[HostName] ;
            zooID            = elem[ZooID3] ;
            user             = elem[User] ;
            passwd           = elem[Passwd] ;
            sdbUser          = elem[SdbUser] ;
            sdbPasswd        = elem[SdbPasswd] ;
            sshPort          = parseInt(elem[SshPort]) ;
            installPath      = elem[InstallPath3] ;
            dataPath         = elem[DataPath3] ;
            clusterName      = elem[ClusterName3] ;
            businessName     = elem[BusinessName3] ;
            userTag          = user_tag ;
         }
         catch( e )
         {
            SYSEXPHANDLE( e ) ;
            errMsg = "Invalid field" ;
            rc = SDB_INVALIDARG ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
                     errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
            result[Errno] = rc ;
            result[Detail] = errMsg ;
            hasFailed = true ;
            RET_JSON[Result].push( result ) ;
            continue ;
         }
         // ssh to target host
         try
         {
            ssh = new Ssh( hostName, user, passwd, sshPort ) ;
         }
         catch( e )
         {
            SYSEXPHANDLE( e ) ;
            errMsg = sprintf( "Failed to ssh to host[?]", hostName ) ; 
            rc = GETLASTERROR() ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
                     errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
            result[Errno] = rc ;
            result[Detail] = errMsg ;
            hasFailed = true ;
            RET_JSON[Result].push( result ) ;
            try{ ssh.close() ; }catch(e){}
            continue ;
         }
         // check whether zookeeper progress exist in target host
         progExisted = _checkProgExist( ssh ) ;
         if ( true == progExisted )
         {
            errMsg = sprintf( "Progress about zookeeper has run in host[?]", ssh.getPeerIP() ) ;
            PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
         }

         // check whether the install and data path exist or not
         fileExisted = _checkFileExist( ssh, installPath, dataPath ) ;
         if ( true == fileExisted )
         {
            try
            {
               // check whether the existed znode is installed by om agent
               addByAgent = _addByAgent( ssh, installPath, clusterName, businessName, userTag ) ;
               if ( true == addByAgent )
               {
                  errMsg = sprintf( "Znode in host[?] was add by omagent, going to remove it", ssh.getPeerIP() ) ;
                  PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
                  _cleanUpZNode( ssh, installPath, dataPath ) ;
               }
               else
               {
                  rc = SDB_FE ;
                  errMsg = sprintf( "Stop installing for an unknown znode had been installed in host[?]", ssh.getPeerIP() ) ;
                  PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV, errMsg ) ;
                  result[Errno] = rc ;
                  result[Detail] = errMsg ;
                  hasFailed = true ;
                  RET_JSON[Result].push( result ) ;
                  try{ ssh.close() ; }catch(e){}
                  continue ;
               }
            }
            catch( e )
            {
               SYSEXPHANDLE( e ) ;
               errMsg = sprintf( "Failed to check whether the existed znode in host[?] is installed by omagent or not", hostName ) ; 
               rc = GETLASTERROR() ;
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
                        errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
               result[Errno] = rc ;
               result[Detail] = errMsg ;
               hasFailed = true ;
               RET_JSON[Result].push( result ) ;
               try{ ssh.close() ; }catch(e){}
               continue ;
            }
         }
         
         // disconnect
         try{ ssh.close() ; }catch(e){}
         RET_JSON[Result].push( result ) ;
      }
      // check whether the test has failed
      if ( true == hasFailed )
      {
         var arr = RET_JSON[Result] ;
         
         for ( var i = 0; i < arr.length; i++ )
         {
            if ( SDB_OK != arr[i][Errno] )
            {
               RET_JSON[Errno] = arr[i][Errno] ;
               RET_JSON[Detail] = arr[i][Detail] ;
               break ;
            }
         }
         if ( SDB_OK == RET_JSON[Errno] )
         {
            RET_JSON[Errno] = SDB_SYS ;
            RET_JSON[Detail] = sprintf( "The environment is not ok for installing ? zookeeper",
                                        isCluster ? "replicated":"stand-alone" ) ;
         }
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZN_ENV,
               sprintf( "Failed to check environment for installing zookeeper, rc:?, detail:?", rc, errMsg ) ) ;   
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
//println( "RET_JSON is: " + JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;

