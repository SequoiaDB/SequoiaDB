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
@description: remove target host from cluster( uninstall the db packet and
              stop sdbcm)
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "HostName": "rhel64-test8", "IP": "192.168.20.165", "ClusterName": "c1", "User": "root", "Passwd": "sequoiadb","InstallPath": "/opt/sequoiadb", "SshPort": "22" } ;
   SYS_JSON: task id, the format is: { "TaskID":1 } ;
   ENV_JSON: {}
   OTHER_JSON: {}
@return
   RET_JSON: the format is: { "errno": 0, "detail": "", "IP": "192.168.20.165" }
*/

var FILE_NAME_REMOVE_HOST = "removeHost.js" ;
var RET_JSON       = new removeHostResult() ;
var rc             = SDB_OK ;
var errMsg         = "" ;

var host_ip        = "" ;
var host_name      = "" ;
var task_id        = 0 ;

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
  
   // 2. specify log file's name
   try
   {
      host_ip   = BUS_JSON[IP] ;
      host_name = BUS_JSON[HostName] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = sprintf( "Failed to create js log file for removing host[?]", host_ip ) ;
      PD_LOG( arguments, PDERROR, FILE_NAME_REMOVE_HOST,
              sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERROR(), GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_HOST,
            sprintf( "Begin to remove host[?]", host_name ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_HOST,
            sprintf( "Finish removing host[?]", host_name ) ) ;
}

/* *****************************************************************************
@discretion: check whether sequoiadb had been installed or not
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   path[string]: the path where the sequoiadb install in
@return
   [bool]:
***************************************************************************** */
function _hasUninstalled( ssh, path )
{
   var str     = "" ;
   var command = "" ;
   var prog    = adaptPath( path ) + OMA_PROG_UNINSTALL ;

   try
   {
      if ( SYS_LINUX == SYS_TYPE )
      {
         command = " ls " + prog ;
         try
         {
            str = ssh.exec( command ) ;
         }
         catch( e )
         {
            // "2" is the errno, return by linux shell
            if ( 2 == ssh.getLastRet() )
               return true ;
            else
               exception_handle( SDB_SYS, ssh.getLastOut() ) ;
         }
      }
      else
      {
         // TODO: windows
      }
      str = removeLineBreak( str ) ;
      PD_LOG2( task_id, arguments, PDDEBUG, FILE_NAME_REMOVE_HOST,
               sprintf( "prog is [?], str is [?]", prog, str ) ) ;
      if ( prog == str )
         return false ;
      else
         exception_handle( SDB_SYS, "The result is not what we excepted" ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to check whether sequoiadb had been uninstalled or not" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
               sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: uninstall sequoiadb and stop sdbcm in remote host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   path[string]: the path where the sequoiadb install in
@return void
***************************************************************************** */
function _uninstallDBInRemote( ssh, path )
{
   var installpath = adaptPath( path ) ;
   var str         = null ;
   if ( SYS_LINUX == SYS_TYPE )
   {
      str = installpath + OMA_PROG_UNINSTALL + " --mode " + " unattended " ;
   }
   else
   {
      // TODO: windows
   }
   try
   {
      ssh.exec( str ) ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Failed to uninstall sequoiadb" ;
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
               sprintf( errMsg + ", rc: ?, detail: ?", GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

function main()
{
   var ip          = null ;
   var user        = null ;
   var passwd      = null ;
   var sshport     = null ;
   var clusterName = null ;
   var ssh         = null ;
   var installPackages = null ;
   
   _init() ;
   
   try
   {
      // 1. remove arguments
      try
      {
         ip           = BUS_JSON[IP] ;
         RET_JSON[IP] = ip ;
         user         = BUS_JSON[User] ;
         passwd       = BUS_JSON[Passwd] ;
         sshport      = parseInt(BUS_JSON[SshPort]) ;
         clusterName  = BUS_JSON[ClusterName2] ;
         installPackages = BUS_JSON[FIELD_PACKAGES] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = GETLASTERROR() ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      
      // 2. ssh to target host
      try
      {
         ssh = new Ssh( ip, user, passwd, sshport ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to ssh to host[?]", ip ) ; 
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      
      if ( typeof( installPackages ) != 'object' )
      {
         rc = SDB_SYS ;
         errMsg = sprintf( "Invalid packages, host[?]", ip ) ; 
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
                  errMsg + ", rc: " + rc ) ;
         exception_handle( rc, errMsg ) ;
      }

      for( var index in installPackages )
      {
         var packageName = installPackages[index][FIELD_NAME] ;
         var installPath = installPackages[index][FIELD_INSTALL_PATH] ;

         if( FIELD_SEQUOIADB == packageName &&
             true == isInLocalHost( ssh ) )
         {
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_HOST,
                     sprintf( "It's in localhost[?], not going to uninstall local SequoiaDB", ip ) ) ;
            continue ;
         }

         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_HOST,
                  sprintf( "host[?] uninstall ?", ip, packageName ) ) ;

         // 3. check whether db has been uninstalled
         if( true == _hasUninstalled( ssh, installPath ) )
         {
            PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_HOST,
                     sprintf( "? had been uninstalled in host[?]",
                              packageName, ip ) ) ;
            continue ;
         }

         // 4. uninstall db packet and stop sdbcm in remote host
         _uninstallDBInRemote( ssh, installPath ) ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_HOST,
               sprintf( "Failed to remove host[?], rc:?, detail:?",
                        host_name, rc, errMsg ) ) ;
      RET_JSON[Errno]  = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
   // return the result
   return RET_JSON ;
}

// execute
   main() ;

