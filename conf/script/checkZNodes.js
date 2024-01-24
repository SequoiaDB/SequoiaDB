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
@description: check whether all the znodes are ok
@modify list:
   2015-6-5 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}] } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "", "Result":[{"HostName":"susetzb", "zooid": "1", "errno": 0, "detail": ""},{"HostName":"rhel64-test8", "zooid": "2", "errno": 0, "detail": ""},{"HostName":"rhel64-test8", "zooid": "3", "errno": 0, "detail": ""}] } ;
*/

// println
//var BUS_JSON = { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}] } ;

//var SYS_JSON = { "TaskID": 1 } ;

function checkZNodeResult()
{
   this.HostName         = "" ;
   this.zooid            = "0" ;
   this.errno            = SDB_OK ;
   this.detail           = "" ;
}

var FILE_NAME_CHECK_ZNODES = "checkZNodes.js" ;
var RET_JSON        = new commonResult() ;
var rc              = SDB_OK ;
var errMsg          = "" ;
RET_JSON[Result]    = [] ;

var task_id         = "" ;
var zkServer        = "" ;
if ( SYS_LINUX == SYS_TYPE )
{
   zkServer = "bin/zkServer.sh"
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
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZNODES,
            sprintf( "Begin to check zookeeper nodes' status in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_CHECK_ZNODES,
            sprintf( "Finish checking zookeeper nodes' status in task[?]", task_id ) ) ;
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
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZNODES,
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
   var zooID       = null ;
   var ssh         = null ;
   var ssh2        = null ;
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
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZNODES,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      
      for ( var i = 0; i < serverInfo.length; i++ )
      {
         var elem   = null ;
         var result = new checkZNodeResult() ;
         result[Errno] = SDB_OK ;
         result[Detail] = "" ;
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
         }
         catch( e )
         {
            SYSEXPHANDLE( e ) ;
            errMsg = "Invalid field" ;
            rc = SDB_INVALIDARG ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZNODES,
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
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZNODES,
                     errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
            result[Errno] = rc ;
            result[Detail] = errMsg ;
            hasFailed = true ;
            RET_JSON[Result].push( result ) ;
            try{ ssh.close() ; }catch(e){}
            continue ;
         }
         // check status
         try
         {
            _checkStatus( ssh, installPath ) ;
         }
         catch( e )
         {
            result[Errno] = GETLASTERROR() ;
            result[Detail] = GETLASTERRMSG() ;
            hasFailed = true ;
            RET_JSON[Result].push( result ) ;
            try{ ssh.close() ; }catch(e){}
            continue ;
         }
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
               break ;
            }
         }
         if ( SDB_OK == RET_JSON[Errno] )
         {
            RET_JSON[Errno] = SDB_SYS ;
         }
         RET_JSON[Detail] = sprintf( "The ? zookeeper is not ready", isCluster ? "replicated":"stand-alone" ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_CHECK_ZNODES,
               sprintf( "Failed to check zookeeper nodes' status, rc:?, detail:?", rc, errMsg ) ) ;   
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
//println( "RET_JSON is: " + JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;

