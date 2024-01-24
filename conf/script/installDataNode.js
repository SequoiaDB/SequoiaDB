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
@description: create data node
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "User": "root", "Passwd": "sequoiadb", "SshPort": "22", "InstallGroupName": "group1", "InstallHostName": "rhel64-test8", "InstallSvcName": "51000", "InstallPath": "/opt/sequoiadb/database/data/51000", "InstallConfig": { "diaglevel": 3, "role": "data", "logfilesz": 64, "logfilenum": 20, "transactionon": "false", "preferedinstance": "A", "numpagecleaners": 1, "pagecleaninterval": 10000, "hjbuf": 128, "logbuffsize": 1024, "maxprefpool": 200, "maxreplsync": 10, "numpreload": 0, "sortbuf": 512, "syncstrategy": "none", "userTag":"", "clusterName":"c1", "businessName":"b1" } } ;
   SYS_JSON: the format is: { "TaskID": 2, "TmpCoordSvcName": "10000" } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "" }
*/

var FILE_NAME_INSTALL_DATA_NODE = "installDataNode.js" ;
var RET_JSON        = new installNodeResult() ;
var rc              = SDB_OK ;
var errMsg          = "" ;

var task_id         = "" ;
var host_name       = "" ;
var host_svc        = "" ;

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
      PD_LOG( arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_DATA_NODE,
            sprintf( "Begin to install data node[?:?] in task[?]",
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
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_DATA_NODE,
            sprintf( "Finish installing data node[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}

/* *****************************************************************************
@discretion: create data node
@parameter
   db[object]: Sdb object
   hostName[string]: install host name
   svcName[string]: install svc name
   installGroup[string]: install group name
   installPath[string]: install path
   config[json]: config info
@return void
***************************************************************************** */
function _createDataNode( db, hostName, svcName,
                          installGroup, installPath, config )
{
   var rg     = null ;
   var node   = null ;
   var i = 0 ;
   try
   {
      // when catalog has no primary, wait for a while
      for ( ; i < OMA_WAIT_CATALOG_TRY_TIMES; i++ )
      {
         try
         {
            rg = db.getRG( installGroup ) ;
         }
         catch( e )
         {
            if ( SDB_CLS_NOT_PRIMARY == e )
            {
               PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_DATA_NODE,
                        "Catalog has no primary, waiting 1 sec" ) ;
               sleep( 1000 ) ; // l sec
               continue ;
            }
            else
            {
               throw e ;
            }
         }
         break ;
      }
      if ( OMA_WAIT_CATALOG_TRY_TIMES == i )
      {
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                  "Catalog has no primary" ) ;
         throw SDB_CLS_NOT_PRIMARY ;
      }
   }
   catch ( e )
   {
      if ( SDB_CLS_GRP_NOT_EXIST == e )
      {
         try
         {
            // create coord replica group
            rg = db.createRG( installGroup ) ;
         }
         catch ( e )
         {
            SYSEXPHANDLE( e ) ;
            errMsg = sprintf( "Failed to create data group[?]", installGroup ) ;
            rc = GETLASTERROR() ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                     sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
            exception_handle( rc, errMsg ) ;
         }
      }
      else
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to get data group[?]", installGroup );
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                  sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
         exception_handle( rc, errMsg ) ;
      }
   }
   // create data node
   try
   {
      PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_DATA_NODE,
               sprintf( "Create data node passes arguments: hostName[?], svcName[?], installPath[?], config[?]",
                        hostName, svcName, installPath, JSON.stringify(config) ) ) ;
      node = rg.createNode( hostName, svcName, installPath, config ) ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to create data node[?:?] in group[?]",
                        hostName, svcName, installGroup ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
   // start data node
   try
   {
      node.start() ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to start data node[?:?] in group[?]",
                        hostName, svcName, installGroup ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
   // try to start data group
   try
   {
      var obj = eval ( '(' + rg.getDetail().next() + ')' ) ;
      var s = obj[Status] ;
      if ( "number" == typeof( s ) )
      {
         if ( s > 0 )
         {
            return ;
         }
         else
         {
            rg.start() ;
         }
      }
      else
      {
         // start the data group anyway
         rg.start() ;
      }
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to start data group[?]", installGroup ) ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var tmpCoordHostName   = null ;
   var tmpCoordSvcName    = null ;
   var sdbUser            = null ;
   var sdbUserGroup       = null ;
   var user               = null ;
   var passwd             = null ;
   var sshport            = null ;
   var installHostName    = null ;
   var installSvcName     = null ;
   var installGroupName   = null ;
   var installPath        = null ;
   var installConfig      = null ;
   var ssh                = null ;
   var db                 = null ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         tmpCoordHostName   = System.getHostName() ;
         tmpCoordSvcName    = SYS_JSON[TmpCoordSvcName] ;
         sdbUser            = BUS_JSON[SdbUser] ;
         sdbUserGroup       = BUS_JSON[SdbUserGroup] ;
         user               = BUS_JSON[User] ;
         passwd             = BUS_JSON[Passwd] ;
         sshport            = parseInt(BUS_JSON[SshPort]) ;
         installHostName    = BUS_JSON[InstallHostName] ;
         installSvcName     = BUS_JSON[InstallSvcName] ;
         installGroupName   = BUS_JSON[InstallGroupName] ;
         installPath        = BUS_JSON[InstallPath] ;
         installConfig      = BUS_JSON[InstallConfig] ;

         delete installConfig[InstallPath] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
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
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 3. change install path owner
      changeDirOwner( ssh, installPath, sdbUser, sdbUserGroup ) ;
      // 4. connect to temporary coord
      try
      {
         db = new Sdb( tmpCoordHostName, tmpCoordSvcName, "", "" ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to connect to temporary coord[?:?]",
                           tmpCoordHostName, tmpCoordSvcName ) ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 5. create data node
      _createDataNode( db, installHostName, installSvcName,
                       installGroupName, installPath, installConfig ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_DATA_NODE,
               sprintf( "Failed to install data node[?:?], rc:?, detail:?",
               host_name, host_svc, rc, errMsg ) ) ;             
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

