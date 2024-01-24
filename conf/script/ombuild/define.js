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
@description: the definition for rebuilding OM
@modify list:
   2016-5-27 Zhaobo Tan  Init
@parameter
   void 
@return
   void 
*/

var AdditionType                          = "AddtionType" ;
var Location                              = "Location" ;

var FIELD_HOST_NAME                       = "host_name" ;
var FIELD_DEFAULT_ROOT_USER               = "default_root_user" ;
var FIELD_DEFAULT_ROOT_PASSWD             = "default_root_passwd" ;
var FIELD_DEFAULT_SSH_PORT                = "default_ssh_port" ;
var FIELD_MODULE_NAME                     = "module_name" ;
var FIELD_MODULE_TYPE                     = "module_type" ;
var FIELD_CLUSTER_NAME                    = "cluster_name" ;
var FIELD_CLUSTER_DESCRIPTION             = "cluster_description" ;
var FIELD_SDB_ADMIN_USER_NAME             = "sdb_admin_user_name" ;
var FIELD_SDB_ADMIN_PASSWORD              = "sdb_admin_password" ;
var FIELD_SDB_ADMIN_GROUP_NAME            = "sdb_admin_group_name" ;
var FIELD_SDB_DEFAULT_INSTALL_PATH        = "sdb_default_install_path" ;
var FIELD_DB_AUTH_USER                    = "db_auth_user" ;
var FIELD_DB_AUTH_PASSWD                  = "db_auth_passwd" ;
var FIELD_DB_CONFIG_OPTION                = "db_config_option" ;
var FIELD_DB_CONFIG_SVC_SCHEDULER         = "svcscheduler" ;
var FIELD_DB_INSTALL_PACKET               = "db_install_packet" ;
var FIELD_OM_ADMIN_USER_NAME              = "om_admin_user_name" ;
var FIELD_OM_ADMIN_PASSWORD               = "om_admin_password" ;

var FIELD_ROOT_USER                       = "_root_user" ;
var FIELD_ROOT_PASSWD                     = "_root_passwd" ;
var FIELD_SSH_PORT                        = "_ssh_port" ;
var FIELD_INSTALL_PATH                    = "_install_path" ;

var FIELD_COORD_INFO_HOSTNAME             = "HostName" ;
var FIELD_COORD_INFO_DBPATH               = "dbpath" ;
var FIELD_COORD_INFO_SERVICE              = "Service" ;
var FIELD_COORD_INFO_TYPE                 = "Type" ;
var FIELD_COORD_INFO_NAME                 = "Name" ;
var FIELD_COORD_INFO_NODEID               = "NodeID" ;
var FIELD_COORD_INFO_STATUS               = "Status" ;

var FIELD_CONF_CONFPATH                   = "confpath" ;
var FIELD_CONF_DBPATH                     = "dbpath" ;
var FIELD_CONF_HOSTNAME                   = "hostname" ;
var FIELD_CONF_IP                         = "ip" ;
var FIELD_CONF_SVCNAME                    = "svcname" ;
var FIELD_CONF_REPLNAME                   = "replname" ;
var FIELD_CONF_CATALOGNAME                = "catalogname" ;
var FIELD_CONF_SHARDNAME                  = "shardname" ;
var FIELD_CONF_HTTPNAME                   = "httpname" ;
var FIELD_CONF_ROLE                       = "coord" ;
var FIELD_CONF_CATALOGADDR                = "catalogaddr" ;

var FIELD_CONF_GROUPNAME                  = "groupname" ;
var FIELD_CONF_DATAGROUPNAME              = "datagroupname" ;
var FIELD_CONF_ROLE                       = "role" ;
var FIELD_CONF_DIAGLEVEL                  = "diaglevel" ;
var FIELD_CONF_HJBUF                      = "hjbuf" ;

var FIELD_CONF_ARCHIVECOMPRESSON          = "archivecompresson" ;
var FIELD_CONF_ARCHIVEEXPIRED             = "archiveexpired" ;
var FIELD_CONF_ARCHIVEON                  = "archiveon" ;
var FIELD_CONF_ARCHIVEPATH                = "archivepath" ;
var FIELD_CONF_ARCHIVEQUOTA               = "archivequota" ;
var FIELD_CONF_ARCHIVETIMEOUT             = "archivetimeout" ;
var FIELD_CONF_BKUPPATH                   = "bkuppath" ;

var FIELD_CONF_INDEXPATH                  = "indexpath" ; 
var FIELD_CONF_LOBMETAPATH                = "lobmetapath" ;
var FIELD_CONF_LOBPATH                    = "lobpath" ;

var FIELD_CONF_LOGBUFFSIZE                = "logbuffsize" ;
var FIELD_CONF_LOGFILENUM                 = "logfilenum" ;
var FIELD_CONF_LOGFILESZ                  = "logfilesz" ;
var FIELD_CONF_MAXPREFPOOL                = "maxprefpool" ;
var FIELD_CONF_MAXREPLSYNC                = "maxreplsync" ;
var FIELD_CONF_MAXSYNCJOB                 = "maxsyncjob" ;
var FIELD_CONF_NUMPRELOAD                 = "numpreload" ;
var FIELD_CONF_PREFEREDINSTANCE           = "preferedinstance" ;
var FIELD_CONF_SORTBUF                    = "sortbuf" ;

var FIELD_CONF_SYNCDEEP                   = "syncdeep" ;
var FIELD_CONF_SYNCINTERVAL               = "syncinterval" ;
var FIELD_CONF_SYNCRECORDNUM              = "syncrecordnum" ;

var FIELD_CONF_SYNCSTRATEGY               = "syncstrategy" ;
var FIELD_CONF_TRANSACTION                = "transactionon" ;
var FIELD_CONF_WEIGHT                     = "weight" ;
var FIELD_CONF_OM_ADDR                    = "omaddr" ;

var ACTION_BUILD_OM                       = "buildom" ;
var ACTION_REMOVE_OM                      = "removeom" ;
var ACTION_ADD_BUSINESS                   = "addbusiness" ;
var ACTION_UPDATE_COORD                   = "updatecoord" ;
var ACTION_FLUSH_CONFIG                   = "flushconfig" ;

function isIP( strIP ) {
   if ( strIP == undefined ) return false ;
   var re = /^(\d+)\.(\d+)\.(\d+)\.(\d+)$/g ;
   if( re.test(strIP) ) {
      if ( RegExp.$1 < 256 && RegExp.$2 < 256 && 
           RegExp.$3 < 256 && RegExp.$4 < 256 ) {
           return true;
      }
   }
   return false;
} ;

var CoordInfo = function() {
   this.HostName = null ;
	this.dbpath   = null ;
	this.Service  = [ { "Type" : 0, "Name" : "" }, 
		               { "Type" : 1, "Name" : "" }, 
		               { "Type" : 2, "Name" : "" } ] ;
	this.NodeID   = null ;
//	this.Status   = 1 ;

	CoordInfo.prototype.toString = function() {
      return JSON.stringify( this ) ;
	} ;
} ;

var CoordInfoWrapper = function() {
   this.hostName    = null ;
	this.ip          = null ;
   this.svnName     = null ;
	this.cataAddrArr = [] ;
   this.infoObj     = null ;

	CoordInfoWrapper.prototype.toString = function() {
      return JSON.stringigy( this ) ;
	} ;
} ;

var ClusterInfo = function() {
   // cluster common info
   this.ClusterName         = null ;
	this.Desc                = null ;
   this.SdbUser             = null ;
   this.SdbPasswd           = null ;
   this.SdbUserGroup        = null ;
	this.InstallPath         = null ;
	
	ClusterInfo.prototype.toString = function() {
      return JSON.stringify( this ) ;
	} ;
} ;

var ModuleInfo = function() {
   this[AdditionType]       = 0 ;
   this[BusinessType]       = "sequoiadb" ;
   this[DeployMod]          = "distribution" ;
   this[BusinessName]       = "" ;
   this[ClusterName]        = "" ;
   this[Location]           = [] ;
   this[Time]               = "" ;
   
	ModuleInfo.prototype.toString = function() {
      return JSON.stringify( this ) ;
	} ;
} ;

var HostInfo = function() {
   // host info
   this.address             = null ;
   this.hostName            = null ;   
   this.ip                  = null ;
   this.rootUserName        = null ;
   this.rootPassword        = null ;
   this.sshPort             = null ;
   this.installPath         = null ;
   // ssh obj
   this.rootSshObj          = null ;
   this.adminSshObj         = null ;
   // sdb account info
   this.sdbUserName         = null ;
   this.sdbPassword         = null ;
   this.sdbUserGroupName    = null ;
   // remote installed info
   this.installedInfo       = null ;

	HostInfo.prototype.toString = function() {
      return JSON.stringify( this ) ;
	} ;
} ;

var CheckHostInfo = function() {
   this[IP]           = "" ;
   this[HostName]     = "" ; 
   this[OMA]          = "" ;
   this[OS]           = "" ;
   this[CPU]          = "" ;
   this[Memory]       = "" ;
   this[Disk]         = "" ;
   this[Net]          = "" ;
   this[Port]         = "" ;
   this[Service]      = "" ;
   this[Safety]       = "" ;
   this[ClusterName]  = "" ;
   this[User]         = "" ;
   this[Passwd]       = "" ;
   this[InstallPath]  = "" ;
   this[AgentService] = "" ;
   this[SshPort]      = "" ;
} ;

var NodeConfInfo = function() {
   this[FIELD_CONF_DATAGROUPNAME]     = "" ;
   this[FIELD_CONF_DBPATH]            = "" ;
   this[FIELD_CONF_SVCNAME]           = "" ;
   this[FIELD_CONF_ROLE]              = "" ;
   this[FIELD_CONF_DIAGLEVEL]         = "" ;
   this[FIELD_CONF_HJBUF]             = "" ;
   this[FIELD_CONF_LOGBUFFSIZE]       = "" ;
   this[FIELD_CONF_LOGFILENUM]        = "" ;
   this[FIELD_CONF_LOGFILESZ]         = "" ;
   this[FIELD_CONF_MAXPREFPOOL]       = "" ;
   this[FIELD_CONF_MAXREPLSYNC]       = "" ;
   this[FIELD_CONF_NUMPRELOAD]        = "" ;
   this[FIELD_CONF_PREFEREDINSTANCE]  = "" ;
   this[FIELD_CONF_SORTBUF]           = "" ;
   this[FIELD_CONF_SYNCSTRATEGY]      = "" ;
   this[FIELD_CONF_TRANSACTION]       = "" ;
   this[FIELD_CONF_WEIGHT]            = "" ;
} ;

var NodeConfInfoAdapter = function() {
   this[Config]                       = null ;
   this[FIELD_CONF_CONFPATH]          = "" ;
} ;

var NodeConfInfoWrapper = function() {
   this[BusinessName]                 = "" ;
   this[HostName]                     = "" ;
   this[Adapter]                      = [] ;
} ;

var NodeInfo = function() {
   this[HostName]                     = "" ;
   this[Service]                      = "" ;
   this[Path]                         = [] ;
} ;

var FlushedConfig = function() {
   this.hostName                      = "" ;
   this.adminSshObj                   = null ;
   this.installPath                   = "" ;
   this.confFile                      = "" ;
   this.originalStr                   = "" ;
} ;

var GroupInfo = function() {
   this.groupName                     = "" ;
   this.nodeArr                       = [] ;
} ;

var AuthInfo = function() {
   this[AuthUser]                     = "" ;
   this[AuthPasswd]                   = "" ;
} ;