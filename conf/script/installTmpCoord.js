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
@description: install temporary coord
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is:
      { "clustername": "myCluster", "businessname": "myModule", "usertag": "tmpCoord", "CataAddr": [] }
      or
      { "clustername": "myCluster", "businessname": "myModule", "usertag": "tmpCoord", "CataAddr":[ { "HostName":"suse", "SvcName":"11803" }, { "HostName":"rhel64-test8", "SvcName":"11803" }, { "HostName":"rhel64-test9", "SvcName":"11803" } ] } ;
   SYS_JSON: the format is: { "TaskID" : 5 }
@return
   RET_JSON: the format is: { "Port", "10000" }
*/

var FILE_NAME_INSTALL_TEMPORARY_COORD = "installTmpCoord.js" ;
var RET_JSON = new installTmpCoordResult() ;
var rc       = SDB_OK ;
var errMsg   = "" ;

var task_id  = "" ;
var tmp_coord_install_path = "" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{
   // get task id
   task_id = getTaskID( SYS_JSON ) ;
   setTaskLogFileName( task_id ) ;
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
            sprintf( "Begin to install temporary coord in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
            sprintf( "Finish installing temporary coord in task[?]", task_id ) ) ;
}

/* *****************************************************************************
@discretion: get catalog address for installing temporary coord
@parameter
   cfgInfo[json]: catalog cfg info object
@return
   retObj[json]: the return catalog address, e.g.
                 { "clustername": "c1", "businessname": "b1", "usertag": "tmpCoord","catalogaddr" : "rhel64-test8:11803,rhel64-test9:11803" }
***************************************************************************** */
function _getCfgInfo( cfgInfo )
{
   var retObj           = new Object() ;
   var addrArr          = [] ;
   var len              = 0 ;
   var addr             = "" ;
   var str              = "" ;
   
   // get configure information
   retObj[ClusterName2]  = cfgInfo[ClusterName2] ;
   retObj[BusinessName2] = cfgInfo[BusinessName2] ;
   retObj[UserTag2]      = cfgInfo[UserTag2] ;
   retObj[CatalogAddr2] = "" ;
   addrArr              = cfgInfo[CataAddr] ;
   len                  = addrArr.length ;
   
   // check info
   PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
            sprintf( "clustername[?], businessname[?], usertag[?]",
                    retObj[ClusterName2], retObj[BusinessName2], retObj[UserTag2] ) ) ;
                    
   if ( "undefined" == typeof(retObj[ClusterName2]) ||
        "undefined" == typeof(retObj[BusinessName2]) ||
        "undefined" == typeof(retObj[UserTag2]) )
   {
      errMsg = "Invalid configure information for installing temporary coord" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + " : clustername[?], businessname[?], usertag[?]",
                       retObj[ClusterName2], retObj[BusinessName2], retObj[UserTag2] ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   if ( 0 == len )
   {
      return retObj ;
   }
   // get catalog address
   for( var i = 0; i < len; i++ )
   {
      var obj = addrArr[i] ;
      var hostname = obj[HostName] ;
      var svcname = obj[SvcName] ;
      if ( 0 == i )
      {
         addr = hostname + ":" + svcname ;
      }
      else
      {
         addr += "," + hostname + ":" + svcname ;
      }
   }
   retObj[CatalogAddr2] = addr ;
   return retObj ;
}

/* *****************************************************************************
@discretion: get the information of temporary coord with the giving the condition
@parameter
   option[object]: the option for Sdbtool
   matcher[object]: the matcher for Sdbtool
@return
   retNum[number]: the amount of temporary coord left last time
***************************************************************************** */
function _getTmpCoordInfo( option, matcher )
{
   var tmpCoordInfoArr = null ;
   
   try
   {
      tmpCoordInfoArr = Sdbtool.listNodes( option, matcher ) ;
      if ( "undefined" == typeof( tmpCoordInfoArr ) )
         exception_handle( SDB_SYS, "The information of temporary coord is undefined" ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to get the information of temporary coord" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   return tmpCoordInfoArr ;
}

/* *****************************************************************************
@discretion: get the amount of temporary coord with the giving information
@parameter
   option[object]: the option for Sdbtool
   matcher[object]: the matcher for Sdbtool
@return
   retNum[number]: the amount of temporary coord left last time
***************************************************************************** */
function _getTmpCoordNum( tmpCoordInfoArr )
{
   var retNum = 0 ;
   
   try
   {
      retNum = tmpCoordInfoArr.size() ;
      if ( "number" != typeof( retNum ) )
         exception_handle( SDB_SYS, sprintf( "retNum is not a number" ) ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to get the amount of temporary coord left last time" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   return retNum ;
}

/* *****************************************************************************
@discretion: get the service of temporary coord with the giving condition
@parameter
   option[object]: the option for Sdbtool
   matcher[object]: the matcher for Sdbtool
@return
   retSvc[string]: the service of temporary coord left last time
***************************************************************************** */
function _getTmpCoordSvc( tmpCoordInfoArr )
{
   var retSvc          = null ;
   var tmpCoordInfoObj = null ;
   var tmpCoordNum    = 0 ;
   
   try
   {
      tmpCoordNum = tmpCoordInfoArr.size() ;
      if ( 1 == tmpCoordNum )
      {
         tmpCoordInfoObj  = eval( '(' + tmpCoordInfoArr.pos() + ')' ) ;
         retSvc  = tmpCoordInfoObj[SvcName3] ;
      }
      else
      {
         exception_handle( SDB_SYS, sprintf( "Failed to get service of temporary coord, for [?] temporary coord left last time", tmpCoordNum ) ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to get the service of the remaining temporary coord" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   return retSvc ;
}

/* *****************************************************************************
@discretion: remove temporary coord
@parameter
   svcName[string]: the service of temporary coord
@return void
***************************************************************************** */
function _removeTmpCoord( svcName )
{
   var omaHostName = null ;
   var omaSvcName  = null ;
   var oma         = null ;
   // 1. connect to OM Agent in local host
   try
   {
      omaHostName     = System.getHostName() ;
      omaSvcName      = Oma.getAOmaSvcName( "localhost" ) ;
      oma = new Oma( omaHostName, omaSvcName ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to connect to OM Agent in local host to remove temporary coord" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   // 2. remove temporary coord
   try
   {
      oma.removeCoord( svcName ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to remove temporary coord left last time" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: rollback the exist nodes
@parameter
@return
***************************************************************************** */
function _rollback( tmpCoordHostName, tmpCoordSvcName )
{
   try
   {
      var db       = null ;
      var cur      = null ;
      var obj      = null ;
      var arr      = [] ;
      var tryTimes = 5 ;
      var i        = 0 ;
      
      // 1. connect to the remaining temporary coord
      try
      {
         PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( "Connect to the remaining temporary coord[?:?]",
                           tmpCoordHostName, tmpCoordSvcName ) ) ;
         for( i = 0; i < tryTimes; i++ )
         {
            try
            {
               db = new Sdb( tmpCoordHostName, tmpCoordSvcName ) ;
            }
            catch( e )
            {
               if ( SDB_TIMEOUT == e )
               {
                  PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
                           sprintf( "Connect to temporary coord[?:?] time time out",
                           tmpCoordHostName, tmpCoordSvcName ) ) ;
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
         if ( tryTimes == i )
         {
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                     sprintf( "Failed to connect to temporary coord[?:?] left last time",
                     tmpCoordHostName, tmpCoordSvcName ) ) ;
            throw SDB_TIMEOUT ;
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to connect to the remaining temporary coord" ;
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 2. test whether the temporary coord can connect to catalog or not
      try
      {
         for ( i = 0; i < OMA_WAIT_CATALOG_TRY_TIMES; i++ )
         {
            try
            {
               db.list( SDB_LIST_GROUPS ) ;
            }
            catch( e )
            {
               if ( SDB_CLS_NOT_PRIMARY == e )
               {
                  PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_TEMPORARY_COORD,
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
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                     "Catalog has no primary" ) ;
            throw SDB_CLS_NOT_PRIMARY ;
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "The remaining temporary coord can't connect to catalog" ;
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. remove data groups
      try
      {
         // list data groups
         cur = db.list( SDB_LIST_GROUPS, {"Role": 0}, {"GroupName":""} ) ;
         while( true )
         {
            obj = eval( '(' + cur.next() + ')' ) ;
            if ( "undefined" == typeof(obj) )
               break ;
            arr.push( obj[GroupName] ) ;
         }
         if ( 0 != arr.length )
         {
            // remove data groups
            for ( var i = 0; i < arr.length; i++ )
            {
               PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
                        sprintf( "Removing data group[?] left last error", arr[i] ) ) ;
               db.removeRG( arr[i] ) ;
            }
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to remove data group[?] left last error" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 4. remove coord group
      try
      {
         // list coord group
         cur = db.list( SDB_LIST_GROUPS, {"Role": 1}, {"GroupName":""} ) ;
         obj = eval( '(' + cur.next() + ')' ) ;
         if ( "undefined" != typeof(obj) )
         {
            // remove coord group
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
                     sprintf( "Removing coord group left last error" ) ) ;
            db.removeCoordRG() ;
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to remove coord group left last error" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 5. remove catalog group
      try
      {
         // list coord group
         cur = db.list( SDB_LIST_GROUPS, {"Role": 2}, {"GroupName":""} ) ;
         obj = eval( '(' + cur.next() + ')' ) ;
         if ( "undefined" != typeof(obj) )
         {
            // remove catalog group
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
                     "Removing catalog group left last error" ) ;
            db.removeCatalogRG() ;
         }
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to remove catalog group left last error" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to rollback exist nodes" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: check whether error had happen(when temporary coord is running),
             if so, going to rollback and stop the temporary coord left last
             error
@parameter
   tmpCoordHostName[string]: the host name of temporary coord
   cfgInfoObj[object]: temporary coord configure info
@return void
***************************************************************************** */
function _handleerror( tmpCoordHostName, cfgInfoObj )
{
   var option  = new tmpCoordOption() ;
   var matcher = new tmpCoordMather() ;
   var needToRollback = false ;
   var oldTmpCoordInfoArr = null ;
   var oldTmpCoordSvc     = null ;
   var oldTmpCoordNum     = null ;
   
   // 1. get configure info for checking error
   try
   {
      matcher[ClusterName2]  = cfgInfoObj[ClusterName2] ;
      matcher[BusinessName2] = cfgInfoObj[BusinessName2] ;
      matcher[UserTag2]      = cfgInfoObj[UserTag2] ;
      // when no catalog address, we are installing business, and we need to rollback
      if ( ( "undefined" == typeof( cfgInfoObj[CatalogAddr2] ) ) ||
           ( "" == cfgInfoObj[CatalogAddr2] ) )
      {
         needToRollback = true ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to get info for checking whether error had happened" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   
   // 2. get the information of remaining temporary coord
   oldTmpCoordInfoArr = _getTmpCoordInfo( option, matcher ) ;
   
   // 3. get remaining temporary coord amount
   oldTmpCoordNum = _getTmpCoordNum( oldTmpCoordInfoArr ) ;
   if ( 0 == oldTmpCoordNum )
   {
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
               "No error happened last time" ) ;
      return ;
   }
   else
   {
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
               "Error had happened last time" ) ;
   }

   // 4. get service of the remaining temporary coord
   oldTmpCoordSvc = _getTmpCoordSvc( oldTmpCoordInfoArr ) ;

   // 5. rollback
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
            sprintf( "Need to rollback: ?", needToRollback? "TRUE" : "FALSE" ) ) ;
   if ( true == needToRollback )
   {
      try
      {
         _rollback( tmpCoordHostName, oldTmpCoordSvc ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to rollback the remaining nodes left last time, going to remove remaining temporary coord anyway" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      }
   }
   
   // 6. remove the remaining temporary coord anyway
   try
   {
      _removeTmpCoord( oldTmpCoordSvc ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to remove the remaining temporary coord, stop installing business" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   
   // 7. check whether the remaining coord still exist or not
   try
   {
      oldTmpCoordInfoArr = _getTmpCoordInfo( option, matcher ) ;
      oldTmpCoordNum = _getTmpCoordNum( oldTmpCoordInfoArr ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to check whether remaining temporary still exist or not" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
   if ( 0 != oldTmpCoordNum )
      exception_handle( SDB_SYS, "After rollback, remaining still exist, stop install business" ) ;
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
            "Success to remove the remaining temporary coord left last time" ) ;
}

function main()
{
   var oma                    = null ;
   var tmpCoordHostName       = null ;
   var omaHostName            = null ;
   var omaSvcName             = null ;
   var tmpCoordSvcName        = null ;
   var installInfoObj         = null ;
   var dbInstallPath          = null ;
   var cfgObj                 = null ;

   _init() ;
   
   try
   {
      // 1. get install temporary coord arguments
      try
      {
         omaHostName      = System.getHostName() ;
         omaSvcName       = Oma.getAOmaSvcName( "localhost" ) ;
         tmpCoordHostName = omaHostName ;
         tmpCoordSvcName  = getAUsablePortFromLocal() + "" ;
         installInfoObj   = eval( '(' + Oma.getOmaInstallInfo() + ')' ) ;
         dbInstallPath    = adaptPath( installInfoObj[INSTALL_DIR] ) ;
         tmp_coord_install_path = adaptPath( dbInstallPath ) + "database/tmpCoord/" + tmpCoordSvcName ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to get arguments for installing temporary coord" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 2. get temporary coord configure info
      try
      {
         cfgObj = _getCfgInfo( BUS_JSON ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to get configure info for temporary coord" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // 3. try to handle error happen last time
      try
      {
         PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( "Handle error passes arguments: tmpCoordHostName[?], cfgObj[?]",
                          tmpCoordHostName, JSON.stringify(cfgObj) ) ) ;
         _handleerror( tmpCoordHostName, cfgObj ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to check and handle error may be happened last time" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      // begin to install a new temporary coord
      // 4. connect to OM Agent in local host
      try
      {
         PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( "Connect to OM Agent[?:?] in local host", omaHostName, omaSvcName ) ) ;
         oma = new Oma( omaHostName, omaSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to connect to OM Agent in local host" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }

      // 5. create temporary coord
      try
      {
         PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( "Create temporary coord passes arguments: svc[?], path[?], cfgObj[?]",
                           tmpCoordSvcName, tmp_coord_install_path, JSON.stringify(cfgObj) ) ) ;
         oma.createCoord( tmpCoordSvcName, tmp_coord_install_path, cfgObj ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to create temporary coord" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 6. start temporary coord
      try
      {
         oma.startNode( tmpCoordSvcName ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to start temporary coord" ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( "Success to create new temporary coord[?:?]",
                        tmpCoordHostName, tmpCoordSvcName ) ) ;
      // 7. disconnect and return the port of temporary coord
      oma.close() ;
      oma = null ;
      RET_JSON[TmpCoordSvcName] = tmpCoordSvcName ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to install temporary coord in local host" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_TEMPORARY_COORD,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      if ( null != oma && "undefined" != typeof(oma) )
      {
         try
         {
            oma.removeCoord( tmpCoordSvcName ) ;
         }
         catch ( e1 )
         {
         }
         try
         {
            oma.close() ;
            oma = null ;
         }
         catch ( e2 )
         {
         }
      }
      RET_JSON[Errno]  = rc ;
      RET_JSON[Detail] = errMsg ;
   }

   _final() ;
   return RET_JSON ;
}

// execute
   main() ;

