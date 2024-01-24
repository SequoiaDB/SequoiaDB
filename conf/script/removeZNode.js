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
   BUS_JSON: the format is: { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"} ] } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
@return
   RET_JSON: the format is: {"HostName":"susetzb", "zooid": "1", "errno": 0, "detail": ""} ;
*/

// println
//var BUS_JSON = { "DeployMod":"distribution", "ServerInfo":[ {"HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"}, {"HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initLimit":"10", "ticktime":"2000"} ] } ;

//var SYS_JSON = { "TaskID": 1 } ;

function removeZNodeResult()
{
   this.errno            = SDB_OK ;
   this.detail           = "" ;
   this.HostName         = "" ;
   this.zooid            = "0" ;
}

var FILE_NAME_REMOVE_ZNODE = "removeZNode.js" ;
var RET_JSON        = new removeZNodeResult() ;
var rc              = SDB_OK ;
var errMsg          = "" ;

var task_id         = "" ;
var host_name       = "" ;
var zoo_id          = "" ;
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

   try
   {
      host_name = BUS_JSON[HostName] ;
      zoo_id    = BUS_JSON[ZooID3] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_REMOVE_ZNODE,
              sprintf( errMsg + ", rc: ?, detail: ?", SDB_INVALIDARG, GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   // 2. specify the log file name
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_ZNODE,
            sprintf( "Begin to remove zookeeper node[?.?] in task[?]",
                     host_name, zoo_id, task_id ) ) ;
}

/* *****************************************************************************
@discretion: final
@author: Tanzhaobo
@parameter void
@return void
***************************************************************************** */
function _final()
{
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_ZNODE,
            sprintf( "Finish removing zookeeper node[?.?] in task[?]",
                     host_name, zoo_id, task_id ) ) ;
}

/* *****************************************************************************
@discretion: check the znode's status
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: the install path
@return void
***************************************************************************** */
function _stopZNode( ssh, installPath )
{
   var str = "" ;
   var ret = 0 ;

   str = adaptPath( installPath ) + zkServer + " stop " ;
   try{ ssh.exec( str ) ; }catch( e ){}
   ret = ssh.getLastRet() ;
   if ( 0 != ret )
   {
      errMsg = sprintf( "Failed to stop znode in host[?]", ssh.getPeerIP() ) ;
      rc = SDB_SYS ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: remove zookeeper in target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: install path
   dataPath[string]: data path
@return void
***************************************************************************** */
function _removeZNode( ssh, installPath, dataPath )
{
   var str = "" ;
   var ret = 0 ;
   var hasFailed = false ;

   if ( SYS_LINUX == SYS_TYPE )
   {
      // remove data path
      str = " rm -rf " + dataPath ;
      try{ ssh.exec( str ) ; }catch( e ){}
      ret = ssh.getLastRet() ;
      if ( 0 != ret )
      {
         hasFailed = true ;
         errMsg = sprintf( "Failed to remove zookeeper's data directory in host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      }
      // remove install path
      str = " rm -rf " + installPath ;
      try{ ssh.exec( str ) ; }catch( e ){}
      ret = ssh.getLastRet() ;
      if ( 0 != ret )
      {
         hasFailed = true ;
         errMsg = sprintf( "Failed to remove zookeeper's install directory in host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      }
   }
   else
   {
      // TODO: windows
   }
   // check whether error had happened
   if ( true == hasFailed )
   {
      errMsg = sprintf( "Error happened when removing znode in host[?]", ssh.getPeerIP() ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: check whether the file exist or not
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   file[string]: the file to check
@return 
   [bool] true or false
***************************************************************************** */
function _fileExsited( ssh, file )
{
   var fileExsited = true ;
   var str         = "" ;
   var ret         = 0 ;

   str = " ls -l " + file ;
   try{ ssh.exec( str ) ; }catch(e){}
   ret = ssh.getLastRet() ;
   if ( 0 != ret )
   {
      fileExsited = false ;
      errMsg = sprintf( "File[?] does not exist in host[?], rc: ?, detail: ?",
                        file, ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
   }
   
   return fileExsited ;
}

/* *****************************************************************************
@discretion: check whether the target znode had been removed or not
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: install path
@return 
   [bool] true or false
***************************************************************************** */
function _hasRemoved( ssh, installPath )
{
   var hasRemoved = false ;
   var file1 = "" ;
   var file2 = "" ;
   var fileExsited1 = 0 ;
   var fileExsited2 = 0 ;

   if ( SYS_LINUX == SYS_TYPE )
   {      
      file1 = adaptPath(installPath) + zkServer ;
      file2 = adaptPath(installPath) + cfgFile ;
   }
   else
   {
      // TODO: windows
   }
   fileExsited1 = _fileExsited( ssh, file1 ) ;
   fileExsited2 = _fileExsited( ssh, file2 ) ;
   if ( false == fileExsited1 && false == fileExsited2 )
   {
      hasRemoved = true ;
      errMsg = sprintf( "File[?] and file [?] do not exist in host[?], take the znode had been removed in this host",
                        file1, file2, ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_REMOVE_ZNODE, errMsg ) ;
   }

   return hasRemoved ;
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
   var hasRemoved  = false ;
     
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         RET_JSON[HostName] = BUS_JSON[HostName] ;
         RET_JSON[ZooID3]   = BUS_JSON[ZooID3] ;
         
         hostName         = BUS_JSON[HostName] ;
         zooID            = BUS_JSON[ZooID3] ;
         user             = BUS_JSON[User] ;
         passwd           = BUS_JSON[Passwd] ;
         sdbUser          = BUS_JSON[SdbUser] ;
         sdbPasswd        = BUS_JSON[SdbPasswd] ;
         sshPort          = parseInt(BUS_JSON[SshPort]) ;
         installPath      = BUS_JSON[InstallPath3] ;
         dataPath         = BUS_JSON[DataPath3] ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = "Js receive invalid argument" ;
         rc = SDB_INVALIDARG ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_ZNODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( SDB_INVALIDARG, errMsg ) ;
      }
      // 2. ssh to target host
      try
      {
         ssh = new Ssh( hostName, user, passwd, sshPort ) ; // sdbadmin can stop znode which runs in root 
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to ssh to host[?]", hostName ) ; 
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_ZNODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         try{ ssh.close() ; }catch(e){}
         exception_handle( rc, errMsg ) ;
      }
      // check whether "bin/zkServer.sh" and "conf/zoo.cfg" exist or not,
      // if so, let current znode remove, otherwise, take it had been removed
      hasRemoved = _hasRemoved( ssh, installPath ) ;
      if ( true == hasRemoved )
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_ZNODE,
                  sprintf( "No need to remove znode[?.?]", host_name, zoo_id ) ) ;
      }
      else
      {
         PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_REMOVE_ZNODE,
                  sprintf( "Need to remove znode[?.?]", host_name, zoo_id ) ) ;
         _stopZNode( ssh, installPath ) ;
         _removeZNode( ssh, installPath, dataPath ) ;
      }
      // close ssh
      try{ ssh.close() ; }catch(e){}
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_REMOVE_ZNODE,
               sprintf( "Failed to remove znode in host[?], rc:?, detail:?", hostName, rc, errMsg ) ) ;   
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
      try{ ssh.close() ; }catch(e){}
   }
   
   _final() ;
//println( "RET_JSON is: " + JSON.stringify(RET_JSON) ) ;
   return RET_JSON ;
}

// execute
   main() ;

