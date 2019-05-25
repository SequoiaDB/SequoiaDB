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
@description: install catalog
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "User": "root", "Passwd": "sequoiadb", "SshPort": "22","InstallHostName": "rhel64-test9", "InstallSvcName": "11900", "InstallPath": "/opt/sequoiadb/database/catalog/11900", "InstallConfig": { "diaglevel": 3, "role": "catalog", "logfilesz": 64, "logfilenum": 20, "transactionon": "false", "preferedinstance": "A", "numpagecleaners": 1, "pagecleaninterval": 10000, "hjbuf": 128, "logbuffsize": 1024, "maxprefpool": 200, "maxreplsync": 10, "numpreload": 0, "sortbuf": 512, "syncstrategy": "none", "userTag":"", "clusterName":"c1", "businessName":"b1" } } ;
   SYS_JSON: the format is: { "TaskID": 2, "TmpCoordSvcName": "10000" } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "" }
*/

var FILE_NAME_INSTALL_CATALOG = "installCatalog.js" ;
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
      PD_LOG( arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_CATALOG,
            sprintf( "Begin to install catalog[?:?] in task[?]",
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
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_CATALOG,
            sprintf( "Finish installing catalog[?:?] in task[?]",
                     host_name, host_svc, task_id ) ) ;
}


/* *****************************************************************************
@discretion: wait catalog to be ok
@author: Tanzhaobo
@parameter
   hostName[string]: the newly build catalog host name
   svcName[string]: the newly build catalog svc name
@return void
***************************************************************************** */
function _waitCatalogRGReady( hostName, svcName )
{
   var db  = null ;
   var cur = null ;
   var num = 0 ;
   var i   = 0 ;
   for ( ; i < OMA_WAIT_CATA_RG_TRY_TIMES; i++ )
   {
      try
      {
         db = new Sdb( hostName, svcName ) ;
         cur = db.SYSCAT.SYSNODES.find() ;
         num = cur.size() ;
         if ( num )
         {
            // for cur.size() had run out the cursor, we need to get again
            cur = db.SYSCAT.SYSNODES.find({"GroupName": "SYSCatalogGroup"}) ;
            var record = eval ( '(' + cur.next() + ')' ) ;
            var n = record[PrimaryNode] ;
            if ( "undefined" == typeof(n) )
            {
               sleep( OMA_SLEEP_TIME ) ;
               continue ;   
            }
            else
            {
               break ;
            }
         }
         else
         {
            sleep( OMA_SLEEP_TIME ) ;
            continue ; 
         }
      }
      catch ( e )
      {
         sleep( OMA_SLEEP_TIME ) ;
         continue ;
      }
   }
   if ( OMA_WAIT_CATA_RG_TRY_TIMES <= i )
   {
      errMsg = "Wait catalog to be ready timeout" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG, errMsg ) ;  
      exception_handle( SDB_SYS, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: create catalog
@parameter
   db[object]: Sdb object
   hostName[string]: install host name
   svcName[string]: install svc name
   installPath[string]: install path
   config[json]: config info 
@return void
***************************************************************************** */
function _createCatalogNode( db, hostName, svcName, installPath, config )
{
   var rg = null ;
   var node = null ;
   var i  = 0 ;
   // try to get system catalog group
   try
   {
      // when catalog has no primary, wait for a while
      for ( ; i < OMA_WAIT_CATALOG_TRY_TIMES; i++ )
      {
         try
         {
            rg = db.getRG( OMA_SYS_CATALOG_RG ) ;
         }
         catch( e )
         {
            if ( SDB_CLS_NOT_PRIMARY == e )
            {
               PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_CATALOG,
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
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
                  "Catalog has no primary" ) ;
         throw SDB_CLS_NOT_PRIMARY ;
      }
   }
   // catalog has not been created
   catch ( e )
   {
      if ( SDB_CAT_NO_ADDR_LIST == e )
      {
         try
         {
            PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_CATALOG,
                     sprintf( "Create catalog group passes arguments: hostName[?], svcName[?], installPath[?], config[?]",
                              hostName, svcName, installPath, JSON.stringify(config) ) ) ;
            rg = db.createCataRG( hostName, svcName, installPath, config ) ;
            return ;
         }
         catch ( e )
         {
            SYSEXPHANDLE( e ) ;
            errMsg = "Failed to create catalog group" ;
            rc = GETLASTERROR() ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
                     sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
            exception_handle( rc, errMsg ) ;
         }
      }
      else
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Failed to get catalog group" ;
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
                  sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
         exception_handle( rc, errMsg ) ;
      }
   }
   // catalog has been created
   try
   {
      PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_CATALOG,
               sprintf( "Create catalog node passes arguments: hostName[?], svcName[?], installPath[?], config[?]",
                        hostName, svcName, installPath, JSON.stringify(config) ) ) ;
      node = rg.createNode( hostName, svcName, installPath, config ) ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to create catalog node [" + hostName + ":" + svcName + "]" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
   // start catalog node
   try
   {
      node.start() ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to start catalog node [" + hostName + ":" + svcName + "]" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
               sprintf( errMsg + ", rc:?, detail:?", rc, GETLASTERRMSG() ) ) ;  
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var tmpCoordHostName  = null ;
   var tmpCoordSvcName   = null ;
   var sdbUser           = null ;
   var sdbUserGroup      = null ;
   var user              = null ;
   var passwd            = null ;    
   var sshport           = null ;
   var installHostName   = null ;
   var installSvcName    = null ;
   var installPath       = null ;
   var installConfig     = null ;
   var db                = null ;
   var ssh               = null ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         tmpCoordHostName  = System.getHostName() ;
         tmpCoordSvcName   = SYS_JSON[TmpCoordSvcName] ;
         sdbUser           = BUS_JSON[SdbUser] ;
         sdbUserGroup      = BUS_JSON[SdbUserGroup] ;
         user              = BUS_JSON[User] ;
         passwd            = BUS_JSON[Passwd] ;    
         sshport           = parseInt(BUS_JSON[SshPort]) ;
         installHostName   = BUS_JSON[InstallHostName] ;
         installSvcName    = BUS_JSON[InstallSvcName] ;
         installPath       = BUS_JSON[InstallPath] ;
         installConfig     = BUS_JSON[InstallConfig] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
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
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 3. change install path's owner
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
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 5. create catalog node
      _createCatalogNode( db, installHostName, installSvcName,
                          installPath, installConfig ) ;
      // 6. wait catalog to be available
      _waitCatalogRGReady( installHostName, installSvcName ) ; 
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_CATALOG,
               sprintf( "Failed to install catalog[?:?], rc:?, detail:?",
               host_name, host_svc, rc, errMsg ) ) ;             
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

