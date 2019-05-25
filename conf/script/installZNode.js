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
@description: install zookeeper
@modify list:
   2015-6-5 Zhaobo Tan  Init
@parameter
   BUS_JSON: the format is: { "DeployMod":"distribution", "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz", "HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": 3, "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initlimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus", "ServerInfo":["server.1=susetzb:2888:3888", "server.2=rhel64-test8:2888:3888", "server.3=rhel64-test9:2888:3888"] } ;
   SYS_JSON: the format is: { "TaskID": 1 } ;
@return
   RET_JSON: the format is: { "errno": 0, "detail": "", "HostName": "susetzb", "zooid": "0" }
*/

// println
//var BUS_JSON = { "DeployMod":"distribution", "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz", "HostName":"susetzb", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "1", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initlimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus", "ServerInfo":["server.1=susetzb:2888:3888", "server.2=rhel64-test8:2888:3888", "server.3=rhel64-test9:2888:3888"] } ;

//var BUS_JSON = { "DeployMod":"distribution", "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz", "HostName":"rhel64-test8", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "2", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initlimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus", "ServerInfo":["server.1=susetzb:2888:3888", "server.2=rhel64-test8:2888:3888", "server.3=rhel64-test9:2888:3888"] } ;

//var BUS_JSON = { "DeployMod":"distribution", "PacketPath":"/opt/sequoiadb/packet/zookeeper-3.4.6.tar.gz", "HostName":"rhel64-test9", "User": "root", "Passwd": "sequoiadb", "SdbUser": "sdbadmin", "SdbPasswd": "sdbadmin", "SdbUserGroup": "sdbadmin_group", "SshPort": "22", "zooid": "3", "installpath":"/opt/zookeeper", "datapath":"/opt/zookeeper/data", "dataport":"2888", "electport":"3888", "clientport":"2181", "synclimit":"5", "initlimit":"10", "ticktime":"2000", "clustername":"cl", "businessname":"bus", "ServerInfo":["server.1=susetzb:2888:3888", "server.2=rhel64-test8:2888:3888", "server.3=rhel64-test9:2888:3888"] } ;

//var SYS_JSON = { "TaskID": 1 } ;

function installZKResult()
{
   this.errno                     = SDB_OK ;
   this.detail                    = "" ;
   this.HostName                  = "" ;
   this.zooid                     = "0" ;
}

var FILE_NAME_INSTALL_ZNODE = "installZNode.js" ;
var RET_JSON        = new installZKResult() ;
var rc              = SDB_OK ;
var errMsg          = "" ;

var task_id         = "" ;
var host_name       = "" ;
var zoo_id          = "" ;
var user_tag        = "add by om agent" ;
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
   try
   {
      host_name = BUS_JSON[HostName] ;
      zoo_id    = BUS_JSON[ZooID3] ;
   }
   catch ( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = "Js receive invalid argument" ;
      PD_LOG( arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
              sprintf( errMsg + ", rc: ?, detail: ?", SDB_INVALIDARG, GETLASTERRMSG() ) ) ;
      exception_handle( SDB_INVALIDARG, errMsg ) ;
   }
   setTaskLogFileName( task_id ) ;
   
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_ZNODE,
            sprintf( "Begin to install zookeeper node[?.?] in task[?]",
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
   PD_LOG2( task_id, arguments, PDEVENT, FILE_NAME_INSTALL_ZNODE,
            sprintf( "Finish installing zookeeper node[?.?] in task[?]",
                     host_name, zoo_id, task_id ) ) ;
}

/* *****************************************************************************
@discretion: check whether the path exist or not, if so, expect it to be empty
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   path[string]: the path to check
   type[string]: "install" or "data" to select
@return void
***************************************************************************** */
function _checkPath( ssh, path, type )
{
   var tmp_path = "" ;
   var str = "" ;
   var ret = SDB_OK ;
   
   try
   {
      if ( SYS_LINUX == SYS_TYPE )
      {
         // check the path exist or not
         str = " ls -l " + path ;
         try{ ssh.exec( str ) ; }catch( e ){}
         ret = ssh.getLastRet() ;
         if ( SDB_OK == ret ) // in case the path exists
         {
            // when path exists, check whether it's empty or not
            tmp_path = adaptPath( path ) + "*" ;
            str = " ls -l " + tmp_path ;
            try{ ssh.exec( str ) ; }catch( e ){}
            ret = ssh.getLastRet() ;
            if ( SDB_OK == ret )
            {
               errMsg = sprintf( "The ? path exists in host[?], but it's not empty", type, ssh.getPeerIP() ) ;
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
               exception_handle( SDB_INVALIDARG, errMsg ) ;
            }
            else if ( 2 != ret )
            {
               errMsg = specify( "The ? path exists in host[?], but failed to check whether it's empty or not, rc: ?, detail: ?",
                                 type, ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
               PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
               exception_handle( SDB_SYS, errMsg ) ;
            }
         }
         else if ( 2 != ret )
         {
            errMsg = sprintf( "Failed to check whether ? path exists or not, rc: ?, detail: ?",
                              type, ret, ssh.getLastOut() ) ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
            exception_handle( SDB_SYS, errMsg ) ;
         }
      }
      else
      {
         // TODO: windows
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to check ? path in host[?]", type, ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: push zookeeper install packet to target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   packet[string]: the packet to push
   target_path[string]: the path where packet push to
@return void
***************************************************************************** */
function _pushPacket( ssh, packet, target_path )
{
   try
   {
      createTmpDir( ssh ) ;
      ssh.push( packet, target_path ) ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to push zookeeper packet to host[?]", ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: push zookeeper install packet to target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   insallPath[string]: install path
   zooID[string]: zoo node id
   isCluster[bool]: install cluster environment or not
   bus_info[object]: business information
@return void
***************************************************************************** */
/*
// standalone
tickTIme=2000
dataDir=/opt/zookeeper
clientPort=2181

// cluster
tickTime=2000
dataDir=/opt/zookeeper
clientPort=2181

initLimit=10
syncLimit=5

server.1=susetzb:2888:3888
server.2=rhel64-test8:2888:3888
server.3=rhel64-test9:2888:3888
*/
function _genConfFile( ssh, insallPath, zooID, isCluster, bus_info )
{
   var str = "" ;
   var conf_file = adaptPath( insallPath ) + "conf/zoo.cfg" ;
   var tmp_conf_file = "" ;
   if ( SYS_LINUX == SYS_TYPE )
   {
      tmp_conf_file = "/tmp/zoo.cfg" + "_" + zooID ;
   }
   else
   {
      // TODO: windows
   }
   var tickTime     = null ;
   var dataPath     = null ;
   var clientPort   = null ;
   
   var dataPort     = null ;
   var electPort    = null ;
   var syncLimit    = null ;
   var initLimit    = null ;
   var clusterName  = null ;
   var businessName = null ;
   var userTag      = null ;
   var serverInfo   = null ;
   
   // 1. get config arguments
   try
   {
      tickTime     = bus_info[TickTime3] ;
      dataPath     = bus_info[DataPath3] ;
      clientPort   = bus_info[ClientPort3] ;
      if ( true == isCluster )
      {
         dataPort     = bus_info[DataPort3] ;
         electPort    = bus_info[ElectPort3] ;
         syncLimit    = bus_info[SyncLimit3] ;
         initLimit    = bus_info[InitLimit3] ;
         clusterName  = bus_info[ClusterName3] ;
         businessName = bus_info[BusinessName3] ;
         userTag      = user_tag ;
         serverInfo   = bus_info[ServerInfo] ;
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = SDB_INVALIDARG ;
      errMsg = "Failed to get configuration arguments" ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }

   // gen config file in local
   try
   {
      var file = null ;
      if ( File.exist( tmp_conf_file ) )
      {
         File.remove( tmp_conf_file ) ;
      }
      file = new File( tmp_conf_file ) ;
      // clustername
      file.write( ClusterName3 + "=" + clusterName + OMA_NEW_LINE ) ;
      // businessname
      file.write( BusinessName3 + "=" + businessName + OMA_NEW_LINE ) ;
      // userTag
      file.write( UserTag3 + "=" + userTag + OMA_NEW_LINE ) ;
      
      if ( true == isCluster )
      {
         // initLimit
         file.write( InitLimit2 + "=" + initLimit + OMA_NEW_LINE ) ;
         // syncLimit
         file.write( SyncLimit2 + "=" + syncLimit + OMA_NEW_LINE ) ;
         // server info
         for ( var i = 0; i < serverInfo.length; i++ )
         {
            file.write( serverInfo[i] + OMA_NEW_LINE ) ;
         }
      }
      
      file.write( OMA_NEW_LINE ) ;
      // tickTime
      file.write( TickTime2 + "=" + tickTime + OMA_NEW_LINE ) ;
      // dataDir
      file.write( DataDir2 + "=" + dataPath + OMA_NEW_LINE ) ;
      // clientPort
      file.write( ClientPort2 + "=" + clientPort + OMA_NEW_LINE ) ;
      
      file.close() ;
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to create configuration file in localhost[?]", ssh.getLocalIP() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      // try to remove the tmp file
      try{ File.remove( tmp_conf_file ) ; }catch( e ){}
      exception_handle( rc, errMsg ) ;
   }
   
   // pass config file to remote
   try
   {
      if ( SYS_LINUX == SYS_TYPE )
      {
         str = " rm -rf " + conf_file ;
         try { ssh.exec( str ) ; }catch(e){}
         ssh.push( tmp_conf_file, conf_file ) ;
      }
      else
      {
         // TODO: windows
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to push configuration file to host[?]", ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      // try to remove the tmp file
      try{ File.remove( tmp_conf_file ) ; }catch( e ){}
      exception_handle( rc, errMsg ) ;
   }
   // remove tmp file file in local host
   try{ File.remove( tmp_conf_file ) ; }catch( e ){}
}

/* *****************************************************************************
@discretion: gen myid file for znodes cluster
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   dataPath[string]: data path
   zooID[number]: the id of the znode
   isCluster[bool]: install cluster environment or not
@return void
***************************************************************************** */
function _genMyIDFile( ssh, dataPath, zooID, isCluster )
{
   var myid_file = adaptPath( dataPath ) + "myid" ;
   var tmp_myid_file = "" ;
   if ( SYS_LINUX == SYS_TYPE )
   {
      tmp_myid_file = "/tmp/myid" + "_" + zooID;
   }
   else
   {
      // TODO: windows
   }
   
   if ( true == isCluster )
   {
      // 1. gen myid file in localhost
      try
      {
         var file = null ;
         if ( File.exist( tmp_myid_file ) )
         {
            File.remove( tmp_myid_file ) ;
         }
         file = new File( tmp_myid_file ) ;
         file.write( zooID + "" ) ;
         file.close() ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to create myid file in localhost[?]", ssh.getLocalIP() ) ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
          // remove the tmp file
         try{ File.remove( tmp_myid_file ) ; }catch( e ){}
         exception_handle( rc, errMsg ) ;
      }
      // 2. try to create zookeeper data path and clean it up
      try
      {
         str = " mkdir -p " + dataPath ;
         ssh.exec( str ) ;
         str = " rm -rf " + myid_file ;
         try{ ssh.exec( str ) ; }catch(e){}
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to create data path in host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
          // remove the tmp file
         try{ File.remove( tmp_myid_file ) ; }catch( e ){}
         exception_handle( rc, errMsg ) ;
      }
      // 3. push myid file to remote
      try
      {
         ssh.push( tmp_myid_file, myid_file ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         rc = GETLASTERROR() ;
         errMsg = sprintf( "Failed to push myid file to host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
          // remove the tmp file
         try{ File.remove( tmp_myid_file ) ; }catch( e ){}
         exception_handle( rc, errMsg ) ;
      }
      // 4. remove myid file in local host
      try{ File.remove( tmp_myid_file ) ; }catch( e ){}
   }
}

/* *****************************************************************************
@discretion: stop zookeeper in target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPacket[string]: install packet
   installPath[string]: install path
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
      errMsg = sprintf( "Failed to stop zookeeper node in host[?]", ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
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

   if ( SYS_LINUX == SYS_TYPE )
   {
      // remove data path
      str = " rm -rf " + dataPath ;
      try{ ssh.exec( str ) ; }catch( e ){}
      ret = ssh.getLastRet() ;
      if ( 0 != ret )
      {
         errMsg = sprintf( "Failed to remove zookeeper's data directory in host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      }
      // remove install path
      str = " rm -rf " + installPath ;
      try{ ssh.exec( str ) ; }catch( e ){}
      ret = ssh.getLastRet() ;
      if ( 0 != ret )
      {
         errMsg = sprintf( "Failed to remove zookeeper's install directory in host[?]", ssh.getPeerIP() ) ;
         PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_ZNODE,
                  sprintf( errMsg + ", rc: ?, detail: ?", ret, ssh.getLastOut() ) ) ;
      }
   }
   else
   {
      // TODO: windows
   }
}

/* *****************************************************************************
@discretion: install zookeeper in target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPacket[string]: install packet
   installPath[string]: install path
   datapath[string]: data path
@return void
***************************************************************************** */
function _installZNode( ssh, installPacket, installPath, dataPath )
{
   var str = "" ;
   var ret = 0 ;
   var packet_name = "" ;
   var target_path = "" ;
   var tmp_path = "" ;
   var pos = 0 ;
   
   try
   {
      // 1. push packet to remote
       target_path = adaptPath( OMA_PATH_TEMP_PACKET_DIR ) + getPacketName( installPacket ) ;
      _pushPacket( ssh, installPacket, target_path ) ; 
     
      if ( SYS_LINUX == SYS_TYPE )
      {
         // 2. create install path 
         str = " mkdir -p " + installPath ;
         try{ ssh.exec( str ) ; }catch( e ){}
         ret = ssh.getLastRet() ;
         if ( SDB_OK != ret )
         {
            errMsg = sprintf( "Failed to create install path in host[?], rc: ?, detail: ?",
                              ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
            exception_handle( SDB_SYS, errMsg ) ;
         }
         // 3. extract information from packet
         str = " tar -zxf " + target_path + " -C " + installPath ;
         try{ ssh.exec( str ) ; } catch( e ){}
         ret = ssh.getLastRet() ;
         if ( SDB_OK != ret )
         {
            errMsg = sprintf( "Failed to extract zookeeper from packet in host[?], rc: ?, detail: ?",
                     ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
            exception_handle( SDB_SYS, errMsg ) ;
         }
         // 4. move the content to install path
         packet_name = getPacketName( installPacket ) ;
         pos = packet_name.length - ".tar.gz".length ;
         packet_name = packet_name.substr( 0, pos ) ;
         tmp_path = adaptPath( installPath ) + packet_name + "/*"
         str = " mv " + tmp_path + " " + installPath ;
         try{ ssh.exec( str ) ; }catch( e ){}
         ret = ssh.getLastRet() ;
         if ( SDB_OK != ret )
         {
            errMsg = sprintf( "Failed to move zookeeper from [?] to [?] in host[?], rc: ?, detail: ?",
                              tmp_path, installPath, ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
            PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
            exception_handle( SDB_SYS, errMsg ) ;
         }
         // 5. remove tmp_path
         str = " rm -rf " + adaptPath( installPath ) + packet_name ;
         try{ ssh.exec( str ) ; }catch( e ){}
         ret = ssh.getLastRet() ;
         if ( SDB_OK != ret )
         {
            errMsg = sprintf( "Failed to remove zookeeper dir [?] in host[?], rc: ?, detail: ?",
                              tmp_path, ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
            PD_LOG2( task_id, arguments, PDWARNING, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
         }
      }
      else
      {
         // TODO: windows
      }
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      rc = GETLASTERROR() ;
      errMsg = sprintf( "Failed to install zookeeper package in host[?]", ssh.getPeerIP() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( errMsg + ", rc: ?, detail: ?", rc, GETLASTERRMSG() ) ) ;
      exception_handle( rc, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: start zookeeper in target host
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: install path
   dataPath[string]: data path
@return void
***************************************************************************** */
function _startZNode( ssh, installPath, dataPath )
{
   var str = "" ;
   var ret = 0 ;
   
   str = adaptPath( installPath ) + zkServer + " start " ;
   try{ ssh.exec( str ) ; }catch(e){}
   ret = ssh.getLastRet() ;
   if ( 0 != ret )
   {
      errMsg = sprintf( "Failed to start zookeeper in host[?], rc: ?, detail: ?",
                        ssh.getPeerIP(), ret, ssh.getLastOut() ) ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE, errMsg ) ;
      exception_handle( SDB_SYS, errMsg ) ;
   }
}

/* *****************************************************************************
@discretion: clean up all the installed things
@author: Tanzhaobo
@parameter
   ssh[object]: ssh object
   installPath[string]: install path
   dataPath[string]: data path
@return void
***************************************************************************** */
function _cleanup( ssh, installPath, dataPath )
{
   try { _stopZnode( ssh, installPath ) ; } catch(e){}
   try { _removeZNode( ssh, installPath, dataPath ) ; } catch(e){}
   try { removeTmpDir2( ssh ) ; } catch(e){}
}

function main()
{
   var hostName     = null ;
   var user         = null ;
   var passwd       = null ;
   var sshPort      = null ;
   var sdbUser      = null ;
   var sdbUserGroup = null ;
   var packetPath   = null ;
   var zooID        = null ;
   var installPath  = null ;
   var dataPath     = null ;
   var deployMod    = null ;
   var isCluster    = null ;
   var ssh          = null ;
   var ssh2         = null ;
   
   _init() ;
   
   try
   {
      // 1. get arguments
      try
      {
         RET_JSON[HostName] = BUS_JSON[HostName] ;
         RET_JSON[ZooID3]   = BUS_JSON[ZooID3] ;
         
         hostName     = BUS_JSON[HostName] ;
         user         = BUS_JSON[User] ;
         passwd       = BUS_JSON[Passwd] ;
         sshPort      = parseInt(BUS_JSON[SshPort]) ;  
         sdbUser      = BUS_JSON[SdbUser] ;
         sdbPasswd    = BUS_JSON[SdbPasswd] ;
         sdbUserGroup = BUS_JSON[SdbUserGroup] ;
         packetPath   = BUS_JSON[PacketPath] ;
         zooID        = BUS_JSON[ZooID3] ;
         installPath  = BUS_JSON[InstallPath3] ;
         dataPath     = BUS_JSON[DataPath3] ;
         deployMod    = BUS_JSON[DeployMod] ;
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
         rc = SDB_INVALIDARG ;
         // record error message in log
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         // tell to user error happen
         exception_handle( rc, errMsg ) ;
      }
      // 2. ssh to target host
      try
      {
         ssh = new Ssh( hostName, user, passwd, sshPort ) ;
         ssh2 = new Ssh( hostName, sdbUser, sdbPasswd, sshPort ) ;
      }
      catch( e )
      {
         SYSEXPHANDLE( e ) ;
         errMsg = sprintf( "Failed to ssh to host[?]", hostName ) ; 
         rc = GETLASTERROR() ;
         PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
                  errMsg + ", rc: " + rc + ", detail: " + GETLASTERRMSG() ) ;
         exception_handle( rc, errMsg ) ;
      }
/*
      // 3. check install and data directories
      try
      {
         _checkPath( ssh, installPath, "install" ) ;
         _checkPath( ssh, dataPath, "data" ) ;
      }
      catch( e )
      {
         rc = SDB_FE ;
         errMsg = GETLASTERRMSG() ;
         exception_handle( rc, errMsg ) ;
      }
*/
      // 4. install zookeeper
      _installZNode( ssh, packetPath, installPath ) ;
      // 5. add config file
      _genConfFile( ssh, installPath, zooID, isCluster, BUS_JSON ) ;
      // 6. add myid file
      _genMyIDFile( ssh, dataPath, zooID, isCluster ) ;
      // 7. change paths' owner
      changeDirOwner( ssh, installPath, sdbUser, sdbUserGroup ) ;
      changeDirOwner( ssh, dataPath, sdbUser, sdbUserGroup ) ;
      // 8. start zookeeper node
      _startZNode( ssh2, installPath, dataPath ) ;
      // 9. release resource
      try { removeTmpDir2( ssh ) ; } catch(e){}
      try{ ssh.close() ; }catch(e){}
      try{ ssh2.close() ; }catch(e){}
   }
   catch( e )
   {
      SYSEXPHANDLE( e ) ;
      errMsg = GETLASTERRMSG() ; 
      rc = GETLASTERROR() ;
      PD_LOG2( task_id, arguments, PDERROR, FILE_NAME_INSTALL_ZNODE,
               sprintf( "Failed to install zookeeper node[?.?], rc:?, detail:?",
               host_name, zoo_id, rc, errMsg ) ) ;
      try{ ssh.close() ; }catch(e){}
      try{ ssh2.close() ; }catch(e){}    
      RET_JSON[Errno] = rc ;
      RET_JSON[Detail] = errMsg ;
   }
   
   _final() ;
//println("RET_JSON is: " + JSON.stringify(RET_JSON));
   return RET_JSON ;
}

// execute
   main() ;

