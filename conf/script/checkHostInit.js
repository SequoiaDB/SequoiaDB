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
@description: Prepare for check target host. We are going to push some tool
              processes and js files to target host, and then start a temporary
              sdbcm there
@modify list:
   2014-7-26 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "HostInfo": [ { "IP": "192.168.20.42", "HostName": "susetzb", "User": "root", "Passwd": "sequoiadb", "SshPort": "22" }, { "IP": "192.168.20.165", "HostName": "rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SshPort": "22" },{ "IP": "192.168.20.166", "HostName": "rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SshPort": "22" } ] } ;
   SYS_JSON:
   ENV_JSON:
@return
   RET_JSON: the format is: { "HostInfo": [ { "errno": 0, "detail": "", "AgentService": "10000", "IP": "192.168.20.42" }, { "errno": 0, "detail": "", "AgentService": "10000", "IP": "192.168.20.165" } ] }
*/

var FILE_NAME_CHECK_HOST_INIT = "checkHostInit.js" ;
var RET_JSON        = new Object() ;
RET_JSON[HostInfo]  = [] ;
var rc              = SDB_OK ;
var errMsg          = "" ;

/* *****************************************************************************
@discretion: init
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _init()
{              
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_INIT, "Begin to pre-check host" ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_INIT, "Finish pre-checking host" ) ;
}

/* *****************************************************************************
@discretion: push tool programs and js scripts to target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
@return void
***************************************************************************** */
function _pushPacket( ssh )
{
   var src = "" ;
   var dest = "" ;
   var local_prog_path = "" ;
   var local_spt_path  = "" ;
   
   // tool programs used to start remote sdbcm in remote
   var programs = [ "sdblist", "sdbcmd", "sdbcm", "sdbcmart", "sdbcmtop", "sdb" ] ;

   // js files used to check remote host's info
   var js_files = [ "common.js", "define.js", "log.js",
                    "func.js", "checkHostItem.js", "checkHost.js" ] ;
   try
   {
      // 1. get program's path
      try
      {
         local_prog_path = adaptPath( System.getEWD() ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to get current program's working director in localhost" ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
      // 2. get js script's path
      try
      {
         local_spt_path  = getSptPath( local_prog_path ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to get js script file's path in localhost" ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
                 errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
      PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
              sprintf( "local_prog_path is: ?, local_spt_path is: ?",
                       local_prog_path, local_spt_path ) ) ;
      // 3. push programs and js script files to target host  
      if ( SYS_LINUX == SYS_TYPE )
      {
         // push tool programs
         for ( var i = 0; i < programs.length; i++ )
         {
            src = local_prog_path + programs[i] ;
            dest = OMA_PATH_TEMP_BIN_DIR + programs[i] ;
            ssh.push( src, dest ) ;
         }
         // push js files
         for ( var i = 0; i < js_files.length; i++ )
         {
            src = local_spt_path + js_files[i] ;
            dest = OMA_PATH_TEMP_SPT_DIR + js_files[i] ;
            ssh.push( src, dest ) ;
         }
      }
      else
      {
         // TODO:
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to push programs and js files to host[" + ssh.getPeerIP() + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
              errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: change mode in temporary directory
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
@return void
***************************************************************************** */
function _changeModeInTmpDir( ssh )
{
   var cmd = "" ;
   try
   {
      if ( SYS_LINUX == SYS_TYPE )
      {
        // change mode
        cmd = "chmod -R 755 " + OMA_PATH_TEMP_BIN_DIR ;
        ssh.exec( cmd ) ;
      }
      else
      {
         // TODO: tanzhaobo
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = "Failed to change the newly created temporary directory's mode in host[" + ssh.getPeerIP() + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
              errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: start sdbcm program in remote or local host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   port[string]: port for remote sdbcm
@return void
***************************************************************************** */
function _startTmpCM( ssh, port, secs )
{
   var cmd = "" ;
   var cmd1 = "" ;
   var cmd2 = "" ;
   if ( SYS_LINUX == SYS_TYPE )
   {
      try
      {
         cmd1 = "cd " + OMA_PATH_TEMP_BIN_DIR ;
         cmd2 = "./" + OMA_PROG_SDBCMART ;
         cmd2 += " " + OMA_OPTION_SDBCMART_I ;
         cmd2 += " " + OMA_OPTION_SDBCMART_STANDALONE ;
         cmd2 += " " + OMA_OPTION_SDBCMART_PORT ;
         cmd2 += " " + port ;
         cmd2 += " " + OMA_OPTION_SDBCMART_ALIVETIME ;
         cmd2 += " " + secs ;
         cmd = cmd1 + " ; " + cmd2 ;
         ssh.exec( cmd ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = "Failed to start temporary sdbcm in host[" + ssh.getPeerIP() + "]" ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
                 sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
         exception_handle( rc, errMsg ) ;
      }
   }
   else
   {
      //TODO:
   }
}

/* *****************************************************************************
@discretion: install temporary sdbcm in target host
@author: Tanzhaobo
@parameter
   ssh[object]: the ssh object
   ip[string]: the target ip address
@return
   retObj[string]: the result of install
***************************************************************************** */
function _installTmpCM( ssh, ip )
{
   var retObj = new preCheckResult() ;
   retObj[IP] = ip ;

   // 1. build directory in target host
   PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
           "create temporary director in host: " + ssh.getPeerIP() ) ;
   createTmpDir( ssh ) ;

   // 2. push tool programs and js files to target host
   PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
           "push packet to host: " + ssh.getPeerIP() ) ;
   _pushPacket( ssh ) ;

   // 3. change temporary director's mode
   PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
           "change the mode of temporary director in host: " + ssh.getPeerIP() ) ;
   _changeModeInTmpDir( ssh ) ;

   // 4. get a usable port in target host for installing temporary sdbcm
   PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
           "get a usable port from remote for sdbcm running in host: " + ssh.getPeerIP() ) ;
   var port = getAUsablePortFromRemote( ssh ) ;
   if ( OMA_PORT_INVALID == port )
   {
      errMsg = "Failed to get a usable port in host[" + ssh.getPeerIP() + "]" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT, errMsg ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }

   // 5. start temporary sdbcm program in target host
   PD_LOG( arguments, PDDEBUG, FILE_NAME_CHECK_HOST_INIT,
           "start temporary sdbcm in host: " + ssh.getPeerIP() ) ;
   _startTmpCM( ssh, port, OMA_TMP_SDBCM_ALIVE_TIME ) ;

   PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_INIT,
           sprintf( "start temporary sdbcm successfully in host ?, the port is: ?",
                     ssh.getPeerIP(), port ) ) ;
   // 6. return the temporary sdbcm to omsvc
   retObj[AgentPort] = port + "" ;
   return retObj ;
}

function main()
{ 
   var infoArr = null ;
   var arrLen = null ;
   
   _init() ;
   
   try
   {
      infoArr = BUS_JSON[HostInfo] ;
      arrLen = infoArr.length ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      rc = GETLASTERROR() ;
      // record error message in log
      PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_INIT,
              sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      // tell to user error happen
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   if ( arrLen == 0 )
   {
      errMsg = "Not specified any host to pre-check" ;
      PD_LOG( arguments, PDEVENT, FILE_NAME_CHECK_HOST_INIT, errMsg ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   
   for( var i = 0; i < arrLen; i++ )
   {
      var ssh      = null ;
      var obj      = null ;
      var user     = null ;
      var passwd   = null ;
      var ip       = null ;
      var sshport  = null ;
      var ret      = new preCheckResult() ;
      try
      {
         obj       = infoArr[i] ;
         ip        = obj[IP] ;
         user      = obj[User] ;
         passwd    = obj[Passwd] ;
         sshport   = parseInt(obj[SshPort]) ;
         
         ssh = new Ssh( ip, user, passwd, sshport ) ;
         // install
         ret = _installTmpCM( ssh, ip ) ;
      }
      catch ( e )
      {
         SYSEXPHANDLE( e ) ;
         ret[IP] = ip ;
         ret[Errno] = GETLASTERROR() ;
         ret[Detail] = GETLASTERRMSG() ;
         PD_LOG( arguments, PDERROR, FILE_NAME_CHECK_HOST_INIT,
                 sprintf("Failed to pre-check host[?], rc: ?, detail: ?", ip, ret[Errno], ret[Detail] ) ) ;
      }
      // set return result
      RET_JSON[HostInfo].push( ret ) ;
   }

   _final() ;
   return RET_JSON ;
}

// execute
main() ;

