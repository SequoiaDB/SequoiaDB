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
@description: create standalone
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "User": "root", "Passwd": "sequoiadb", "SshPort": "22", "InstallHostName": "susetzb", "InstallSvcName": "50000", "InstallPath": "/opt/sequoiadb/database/standalone/50000", "InstallConfig": { "diaglevel": "5", "role": "standalone", "logfilesz": "64", "logfilenum": "10", "transactionon": "false", "preferedinstance": "2", "numpagecleaners": "10", "pagecleaninterval": "1000", "hjbuf": "128", "logbuffsize": "1024", "maxprefpool": "200", "maxreplsync": "10", "numpreload": "0", "sortbuf": "512", "syncstrategy": "none", "usertag": "", "clustername": "c1", "businessname": "b1" } } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
   ENV_JSON:
@return
   RET_JSON: the format is: { "errno":0, "detail":"" }
*/

var RET_JSON = new installNodeResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id   = "" ;
var host_ip   = "" ;
var host_name = "" ;
var host_svc  = "" ;
var FILE_NAME_INSTALL_STANDALONE = "installStandalone.js" ;


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
   try
   {
      host_name = BUS_JSON[InstallHostName] ;
      host_svc  = BUS_JSON[InstallSvcName] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_STANDALONE,
            sprintf( "Begin to install standalone[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{  
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_STANDALONE,
            sprintf( "Finish installing standalone[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}

function _checkData( hostName, svcName )
{
   var hasData = false ;

   try
   {
      var db = new Sdb( hostName, svcName ) ;
      var cursor = db.list( SDB_LIST_COLLECTIONSPACES ) ;
      while( true )
      {
         var json ;
         var record = cursor.next() ;

         if ( record === undefined )
         {
            break ;
         }

         json = record.toObj() ;

         if ( 'string' == typeof( json[FIELD_NAME] ) &&
              json[FIELD_NAME].indexOf( 'SYS' ) != 0 )
         {
            hasData = true ;
            PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_STANDALONE,
                     sprintf( "standalone[?:?] has collection space[?]",
                              hostName, svcName, json[FIELD_NAME] ) ) ;
            break ;
         }
      }
   }
   catch( e )
   {
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_STANDALONE,
            sprintf( "Failed to connect standalone[?:?], detail: ?",
                     hostName, svcName, GETLASTERRMSG() ) ) ;
   }

   if ( hasData == true )
   {
      rc = SDB_SYS ;
      errMsg = sprintf( "standalone[?:?] already exist", hostName, svcName ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               sprintf( errMsg + ", rc:?", rc ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: remove the target standalone anyway
@parameter
   clusterName[string]: which cluster the standalone belongs to
   businessName[string]: what business the standalone belongs to
   userTag[string]: tag specified by user
   hostName[string]: install host name
   svcName[string]: install svc name
   agentPort[string]: the port of sdbcm in install host
@return
   retObj[object]:
***************************************************************************** */
function _removeStandalone( clusterName, businessName, userTag, hostName,
                            svcName, agentPort )
{
   var oma    = null ;
   var option = null ;
   
   // 1. build option for remove specified standalone
   try
   {
      option                = new checkSAInfo() ;
      option[ClusterName2]  = clusterName ;
      option[BusinessName2] = businessName ;
      option[UserTag2]      = userTag ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to build option for removing standalone[?:?]", hostName, svcName ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               sprintf( errMsg + ", rc:?, detail:? ", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // 2. connect to target OM Agent
   try
   {
      oma = new Oma( hostName, agentPort ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to connect to OM Agent[?:?]", hostName, agentPort ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               sprintf( errMsg + ", rc:?, detail:? ", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // 3. remove target standalone anyway
   try
   {
      try
      {
         oma.removeData( svcName, option ) ;
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_STANDALONE,
                  sprintf( "Success to clean up standalone[?:?:?] ",
                           hostName, svcName, JSON.stringify(option) ) ) ;
      }
      catch( e )
      {
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_STANDALONE,
                  sprintf( "Failed to clean up standalone[?:?:?] or this standalone does not exist",
                           hostName, svcName, JSON.stringify(option) ) ) ;
      }
      oma.close() ;
      oma = null ;
   }
   catch ( e )
   {
      if ( null != oma && "undefined" != typeof(oma) )
      {
         try
         {
            oma.close() ;
         }
         catch ( e2 )
         {
         }
      }
   }
}

/* *****************************************************************************
@discretion: remove the installed standalone
@parameter
   hostName[string]: install host name
   svcName[string]: install svc name
   agentPort[string]: the port of sdbcm in install host
@return void
***************************************************************************** */
function _removeStandalone2( hostName, svcName, agentPort )
{
   try
   {
       var om = new Oma( hostName, agentPort ) ;
       om.removeData( svcName ) ;
   }
   catch( e )
   {
   }
}

/* *****************************************************************************
@discretion: create standalone
@parameter
   hostName[string]: install host name
   svcName[string]: install svc name
   installPath[string]: install path
   config[json]: config info 
   agentPort[string]: the port of sdbcm in install host
@return void
***************************************************************************** */
function _createStandalone( hostName, svcName, installPath, config, agentPort )
{
   var oma = null ;
   try
   {
      oma = new Oma( hostName, agentPort ) ;
      PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_STANDALONE,
               sprintf( "Create standalone passes arguments: svcName[?], installPath[?], config[?]",
                         svcName, installPath, JSON.stringify(config) ) ) ;
      oma.createData( svcName, installPath, config ) ;
      oma.startNode( svcName ) ;
      oma.close() ;
      oma = null ;
   }
   catch ( e )
   {
      if ( null != oma && "undefined" != typeof(oma) )
      {
         try
         {
            oma.close() ;
         }
         catch ( e2 )
         {
         }
      }
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to create standalone[?:?]", hostName, svcName ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               sprintf( errMsg + ", rc:?, detail:? ", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var clusterName     = null ;
   var businessName    = null ;
   var userTag         = null ;
   var sdbUser         = null ;
   var sdbUserGroup    = null ;
   var user            = null ;
   var passwd          = null ;
   var sshport         = null ;
   var installHostName = null ;
   var installSvcName  = null ;
   var installPath     = null ;
   var installConfig   = null ;
   var ssh             = null ;
   var agentPort       = null ;
   var preCheckResult  = null ;

   _init() ;

   // 1. get arguments
   try
   {
      sdbUser         = BUS_JSON[SdbUser] ;
      sdbUserGroup    = BUS_JSON[SdbUserGroup] ;
      user            = BUS_JSON[User] ;
      passwd          = BUS_JSON[Passwd] ;    
      sshport         = parseInt(BUS_JSON[SshPort]) ;
      installHostName = BUS_JSON[InstallHostName] ;
      installSvcName  = BUS_JSON[InstallSvcName] ;
      installPath     = BUS_JSON[InstallPath] ;
      installConfig   = BUS_JSON[InstallConfig] ;
      clusterName     = installConfig[ClusterName2] ;
      businessName    = installConfig[BusinessName2] ;
      userTag         = installConfig[UserTag2] ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      rc = GETLASTERROR() ;
      // record error message in log
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
      // tell to user error happen
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }

   _checkData( installHostName, installSvcName ) ;

   try
   {
      // 2. ssh to target host
      try
      {
         ssh = new Ssh( installHostName, user, passwd, sshport ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to ssh to host[?]", installHostName ) ; 
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. get OM Agent's service of target host from local sdbcm config file
      agentPort = getOMASvcFromCfgFile( installHostName ) ;
      
      // 4. clean up environment
       _removeStandalone( clusterName, businessName, userTag,
                          installHostName, installSvcName, agentPort ) ;
                          
      // 5. change install path owner
      changeDirOwner( ssh, installPath, sdbUser, sdbUserGroup ) ;
      
      // 6. create standalone
      _createStandalone( installHostName, installSvcName,
                         installPath, installConfig, agentPort ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ;
      rc = GETLASTERROR() ;
      // try to remove installed standalone
      if ( SDBCM_NODE_EXISTED != rc )
      {
         _removeStandalone2( installHostName, installSvcName, agentPort ) ;
      }
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_STANDALONE,
               sprintf( "Failed to install standalone[?:?], rc:?, detail:?",
               host_name, host_svc, rc, errMsg ) ) ;
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;
