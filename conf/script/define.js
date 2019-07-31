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
@description: global value define
@modify list:
   2014-7-26 Zhaobo Tan  Init
*/

// global
var SYS_LINUX = "LINUX" ;
var SYS_WIN   = "WINDOWS" ;
var SYS_TYPE  = System.type() ;

if ( "LINUX" != SYS_TYPE && "WINDOWS" != SYS_TYPE )
{
   throw new Error("Failed to get system type") ;
}

// fields
var Adapter                                = "Adapter" ;
var AgentPort                              = "AgentService" ;
var AgentService                           = "AgentService" ;
var AuthUser                               = "AuthUser" ;
var AuthPasswd                             = "AuthPasswd" ;
var Bandwidth                              = "Bandwidth" ;
var Bit                                    = "Bit" ;
var CataAddr                               = "CataAddr" ;
var CataSvcName                            = "CataSvcName" ;
var CanPing                                = "Ping" ;
var CanSsh                                 = "Ssh" ;
var Config                                 = "Config" ;
var Core                                   = "Core" ;
var Context                                = "Context" ;
var CPU                                    = "CPU" ;
var Cpus                                   = "Cpus" ;
var CanUse                                 = "CanUse" ;
var Disk                                   = "Disk" ;
var Disks                                  = "Disks" ;
var DeployMod                              = "DeployMod" ;
var Distributor                            = "Distributor" ;
var Description                            = "Description" ;
var Free                                   = "Free" ;
var Freq                                   = "Freq" ;
var Filesystem                             = "Filesystem" ;
var Group                                  = "Group" ;
var GroupName                              = "GroupName" ;
var HostName                               = "HostName" ;
var HasInstall                             = "HasInstall" ;
var HasInstalled                           = "HasInstalled" ;
var HasUninstall                           = "HasUninstall" ;
var HostInfo                               = "HostInfo" ;
var Hosts                                  = "Hosts" ;
var MD5                                    = "MD5" ;
var Memory                                 = "Memory" ;
var Model                                  = "Model" ;
var Mount                                  = "Mount" ;
var Name                                   = "Name" ;
var Net                                    = "Net" ;
var Netcards                               = "Netcards" ;
var ID                                     = "ID" ;
var IP                                     = "IP" ;
var Ip                                     = "Ip" ;
var Info                                   = "Info" ;
var IsLocal                                = "IsLocal" ;
var IsNeedUninstall                        = "IsNeedUninstall" ;
var IsRunning                              = "IsRunning" ;
var IsOMStop                               = "IsOMStop" ;
var InstallConfig                          = "InstallConfig" ;
var InstallGroupName                       = "InstallGroupName" ;
var InstallGroupNames                      = "InstallGroupNames" ;
var InstallHostName                        = "InstallHostName" ;
var InstallPacket                          = "InstallPacket" ;
var InstallPath                            = "InstallPath" ;
var InstallSvcName                         = "InstallSvcName" ;
var Olap                                   = "olap";
var OS                                     = "OS" ;
var OM                                     = "OM" ;
var OMA                                    = "OMA" ;
var OmaHostName                            = "OmaHostName" ;
var OmaSvcName                             = "OmaSvcName" ;
var Path                                   = "Path" ;
var Passwd                                 = "Passwd" ;
var ProgPath                               = "ProgPath" ;
var Port                                   = "Port" ;
var PrimaryNode                            = "PrimaryNode" ;
var PacketPath                             = "PacketPath" ;
var Rc                                     = "Rc" ;
var Reachable                              = "Reachable" ;
var Result                                 = "Result" ;
var Release                                = "Release" ;
var Role                                   = "role" ;
var Safety                                 = "Safety" ;
var SdbUserGroup                           = "SdbUserGroup" ;
var SdbPasswd                              = "SdbPasswd" ;
var SdbUser                                = "SdbUser" ;
var Sequoiasql                             = "sequoiasql";
var Service                                = "Service" ;
var ServiceName                            = "ServiceName" ;
var DbName                                 = "DbName" ;
var DbUser                                 = "DbUser" ;
var DbPasswd                               = "DbPasswd" ;
var Desc                                   = "Desc" ;
var Sql                                    = "Sql" ;
var ResultFormat                           = "ResultFormat" ;
var FormatPretty                           = "pretty" ;
var PipeFile                               = "PipeFile" ;
var ServerInfo                             = "ServerInfo" ;
var Size                                   = "Size" ;
var SshPort                                = "SshPort" ;
var Status                                 = "Status" ;
var TaskID                                 = "TaskID" ;
var TmpCoordHostName                       = "TmpCoordHostName" ;
var TmpCoordSvcName                        = "TmpCoordSvcName" ;
var Time                                   = "Time" ;
var TimeStamp                              = "TimeStamp" ;
var Type                                   = "Type" ;
var VCoordHostName                         = "TmpCoordHostName" ;
var VCoordSvcName                          = "TmpCoordSvcName" ;
var Usable                                 = "Usable" ;
var Used                                   = "Used" ;
var User                                   = "User" ;
var UninstallGroupName                     = "UninstallGroupName" ;
var UninstallGroupNames                    = "UninstallGroupNames" ;
var UninstallHostName                      = "UninstallHostName" ;
var UninstallSvcName                       = "UninstallSvcName" ;
var Version                                = "Version" ;
var SvcName                                = "SvcName" ;
var Sys                                    = "Sys" ;
var Idle                                   = "Idle" ;
var Other                                  = "Other" ;
var CalendarTime                           = "CalendarTime" ;
var NetCards                               = "NetCards" ;
var ClusterName                            = "ClusterName" ;
var BusinessType                           = "BusinessType";
var BusinessName                           = "BusinessName" ;
var UserTag                                = "UserTag" ;

var ISPROGRAMEXIST                         = "ISPROGRAMEXIST" ;
var INSTALL_DIR                            = "INSTALL_DIR" ;
var CLUSTERNAME                            = "CLUSTERNAME" ;
var BUSINESSNAME                           = "BUSINESSNAME" ;
var USERTAG                                = "USERTAG" ;
var SDBADMIN_USER                          = "SDBADMIN_USER" ;
var OMA_SERVICE                            = "OMA_SERVICE" ;

// TODO: change this value to xxxx3
var Errno                                  = "errno" ;
var Detail                                 = "detail" ;
var Task                                   = "task" ;

var InstallPath2                           = "installPath" ;
var DataPath2                              = "dataPath" ;
var DataDir2                               = "dataDir" ;
var DataPort2                              = "dataPort" ;
var ElectPort2                             = "electPort" ;
var ClientPort2                            = "clientPort" ;
var SyncLimit2                             = "syncLimit" ;
var InitLimit2                             = "initLimit" ;
var TickTime2                              = "tickTime" ;
var HostName2                              = "hostName" ;
var SvcName2                               = "svcName" ;

// TODO: change this value to xxxxx3
var CatalogAddr2                           = "catalogaddr" ;
var ClusterName2                           = "clustername" ;
var BusinessName2                          = "businessname" ;
var UserTag2                               = "usertag" ;

var SvcName3                               = "svcname" ;
var InstallPath3                           = "installpath" ;
var DataPath3                              = "datapath" ;
var DataDir3                               = "datadir" ;
var DataPort3                              = "dataport" ;
var ElectPort3                             = "electport" ;
var ClientPort3                            = "clientport" ;
var SyncLimit3                             = "synclimit" ;
var InitLimit3                             = "initlimit" ;
var TickTime3                              = "ticktime" ;
var ZooID3                                 = "zooid" ;
var ClusterName3                           = "clustername" ;
var BusinessName3                          = "businessname" ;
var UserTag3                               = "usertag" ;

var DefaultPort2                           = "defaultPort" ;

// SequoiaSQL OLAP
var Cluster                                = "cluster";
var Master                                 = "master";
var Standby                                = "standby";
var Segment                                = "segment";
var MasterHost                             = "master_host";
var MasterPort                             = "master_port";
var MasterDir                              = "master_dir";
var StandbyHost                            = "standby_host";
var SegmentPort                            = "segment_port";
var SegmentDir                             = "segment_dir";
var SegmentHosts                           = "segment_hosts";
var HdfsUrl                                = "hdfs_url";
var MasterTempDir                          = "master_temp_dir";
var SegmentTempDir                         = "segment_temp_dir";
var InstallDir                             = "install_dir";
var LogDir                                 = "log_dir";
var MaxConnections                         = "max_connections";
var SharedBuffers                          = "shared_buffers";
var IsSingle                               = "is_single";
var ReadSec                                = "ReadSec" ;
var WriteSec                               = "WriteSec" ;

// deploy mode
var OMA_DEPLOY_CLUSTER                    = "distribution" ;
var OMA_DEPLOY_STANDALONE                 = "standalone" ;

var SYSTEM_OS_INFO                         = "System.getReleaseInfo()" ;
var SYSTEM_CPU_INFO                        = "System.getCpuInfo()" ;
var SYSTEM_MEM_INFO                        = "System.getMemInfo()" ;
var SYSTEM_DISK_INFO                       = "System.getDiskInfo()" ;
var SYSTEM_NET_INFO                        = "System.getNetcardInfo()" ;

// file in linux
var OMA_PATH_TEMP_OMA_DIR                  = "/tmp/omatmp/" ;
var OMA_PATH_TEMP_OMA_DIR2                 = "/tmp/omatmp" ;
var OMA_PATH_TEMP_BIN_DIR                  = "/tmp/omatmp/bin/" ;
var OMA_PATH_TEMP_PACKET_DIR               = "/tmp/omatmp/packet/" ;
var OMA_PATH_TEMP_CONF_DIR                 = "/tmp/omatmp/conf/" ;
var OMA_PATH_TEMP_DATA_DIR                 = "/tmp/omatmp/data/" ;
var OMA_PATH_TMP_WEB_DIR                   = "/tmp/omatmp/web/" ;
var OMA_PATH_TEMP_TEMP_DIR                 = "/tmp/omatmp/tmp/" ;
var OMA_PATH_TEMP_LOG_DIR                  = "/tmp/omatmp/conf/log/" ;
var OMA_PATH_TEMP_LOCAL_DIR                = "/tmp/omatmp/conf/local/" ;
var OMA_PATH_TEMP_SPT_DIR                  = "/tmp/omatmp/conf/script/" ;
var OMA_PATH_BIN                           = "bin/";

var OMA_PATH_OMA_WORK_DIR                  = "/tmp/omagent" ;
var OMA_PATH_OMA_WORK_TASK_DIR             = "/tmp/omagent/task" ;
//  /tmp/omagent/task/$taskID/pid.txt
var OMA_FILE_PSQL_PID_FILE                 = "pid.txt" ;
//  /tmp/omagent/task/$taskID/result.fifo
var OMA_FILE_PSQL_FIFO_FILE                = "result.fifo" ;

var OMA_FILE_TEMP_ADD_HOST_CHECK           = OMA_PATH_TEMP_TEMP_DIR + "addHostCheckEnvResult" ;
var OMA_FILE_SDBCM_CONF                    = "sdbcm.conf" ;
var OMA_FILE_SDBCM_CONF2                   = "conf/sdbcm.conf" ;
var OMA_FILE_LOG                           = "log.js" ;
var OMA_FILE_COMMON                        = "common.js" ;
var OMA_FILE_DEFINE                        = "define.js" ;
var OMA_FILE_FUNC                          = "func.js" ;
var OMA_FILE_CHECK_HOST_ITEM               = "checkHostItem.js" ;
var OMA_FILE_CHECK_HOST                    = "checkHost.js" ;
var OMA_FILE_ADD_HOST_CHECK_ENV            = "addHostCheckEnv.js" ;
var OMA_FILE_INSTALL_INFO                  = "/etc/default/sequoiadb" ;


// program in linux
var OMA_PROG_BIN_SDBCM                     = "bin/sdbcm" ;
var OMA_PROG_BIN_SDB                       = "bin/sdb" ;
var OMA_PROG_SDBCMD                        = "sdbcmd" ;
var OMA_PROG_SDBCMART                      = "sdbcmart" ;
var OMA_PROG_SDBCMTOP                      = "sdbcmtop" ;
var OMA_PROG_UNINSTALL                     = "uninstall" ;

// misc
var OMA_MISC_CONFIG_PORT                   = "_Port" ;
var OMA_MISC_OM_VERSION                    = "version: " ;
var OMA_MISC_OM_RELEASE                    = "Release: " ;

// status
var STATUS_INIT                            = 0 ;
var STATUS_RUNNING                         = 1 ;
var STATUS_ROLLBACK                        = 2 ;
var STATUS_CANCEL                          = 3 ;
var STATUS_FINISH                          = 4 ;
var STATUS_FAIL                            = 10 ;

var DESC_STATUS_INIT                       = "INIT" ;
var DESC_STATUS_RUNNING                    = "RUNNING" ;
var DESC_STATUS_ROLLBACK                   = "ROLLBACK" ;
var DESC_STATUS_CANCEL                     = "CANCEL" ;
var DESC_STATUS_FINISH                     = "FINISH" ;
var DESC_STATUS_FAIL                       = "FAIL" ;

// new field, normative naming
var FIELD_TASKID                           = TaskID ;
var FIELD_CONFIG                           = Config ;
var FIELD_PLAN                             = "Plan" ;
var FIELD_RESULTINFO                       = "ResultInfo" ;
var FIELD_HOSTNAME                         = HostName ;
var FIELD_SVCNAME                          = SvcName3 ;
var FIELD_SERVICE                          = Service ;
var FIELD_DBPATH                           = "dbpath" ;
var FIELD_ROLE                             = Role ;
var FIELD_COORD                            = "coord" ;
var FIELD_COORD2                           = "Coord" ;
var FIELD_CATALOG                          = "catalog" ;
var FIELD_DATA                             = "data" ;
var FIELD_STANDALONE                       = "standalone" ;
var FIELD_INFO                             = Info ;
var FIELD_DATAGROUPNAME                    = "datagroupname" ;
var FIELD_STATUS                           = Status ;
var FIELD_STATUS_DESC                      = "StatusDesc" ;
var FIELD_FLOW                             = "Flow" ;
var FIELD_ERRNO                            = Errno ;
var FIELD_DETAIL                           = Detail ;
var FIELD_GROUPNAME                        = GroupName ;
var FIELD_DEPLOYMOD                        = DeployMod ;
var FIELD_PROGRESS                         = "Progress" ;
var FIELD_USER                             = User ;
var FIELD_PASSWD                           = Passwd ;
var FIELD_GROUP                            = Group ;
var FIELD_SECONDS                          = "Seconds" ;
var FIELD_PRIMARY_NODE                     = PrimaryNode ;
var FIELD_NODE_ID                          = "NodeID" ;
var FIELD_NAME                             = Name ;
var FIELD_NAME2                            = "name" ;
var FIELD_CMD                              = "cmd" ;
var FIELD_SAC_TASKID                       = "sactaskid" ;
var FIELD_ADDRESS                          = "Address" ;
var FIELD_ADDRESS2                         = "address" ;
var FIELD_BUSINESS_NAME                    = BusinessName ;
var FIELD_BUSINESS_NAME2                   = BusinessName2 ;
var FIELD_BUSINESS_TYPE                    = BusinessType ;
var FIELD_CLUSTER_NAME                     = ClusterName ;
var FIELD_CLUSTER_NAME2                    = ClusterName2 ;
var FIELD_CONFIG                           = Config ;
var FIELD_HOST_INFO                        = HostInfo ;
var FIELD_SEQUOIADB                        = "sequoiadb" ;
var FIELD_SEQUOIASQL_OLTP                  = "sequoiasql-oltp" ;
var FIELD_HOSTLIST                         = "HostList" ;
var FIELD_IP                               = IP ;
var FIELD_HOSTS                            = Hosts ;
var FIELD_IP2                              = Ip ;
var FIELD_AGENT_SERVICE                    = AgentService ;
var FIELD_PACKAGES                         = "Packages" ;
var FIELD_PACKAGE_NAME                     = "PackageName" ;
var FIELD_SDBUSERGROUP                     = SdbUserGroup ;
var FIELD_SDBPASSWD                        = SdbPasswd ;
var FIELD_SDBUSER                          = SdbUser ;
var FIELD_INSTALL_PACKET                   = InstallPacket ;
var FIELD_SSH_PORT                         = SshPort ;
var FIELD_INSTALL_PATH                     = InstallPath ;
var FIELD_VERSION                          = Version ;
var FIELD_ENFORCED                         = "Enforced" ;
var FIELD_PORT                             = Port ;
var FIELD_PORT2                            = "port" ;
var FIELD_FROM                             = "From" ;
var FIELD_TO                               = "To" ;
var FIELD_OPTIONS                          = "Options" ;
var FIELD_HTTP_NAME                        = "httpname" ;
var FIELD_DB_NAME                          = DbName ;

// Async task step
var STEP_GENERATE_PLAN                     = "Generate plan" ;
var STEP_DOIT                              = "Doit" ;
var STEP_CHECK_RESULT                      = "Check result" ;
var STEP_ROLLBACK                          = "Rollback" ;


// port
var OMA_PORT_DEFAULT_SDBCM_PORT            = "" ;
try
{
   var omaCfgObj = eval( '(' + Oma.getOmaConfigs() + ')' ) ;
   OMA_PORT_DEFAULT_SDBCM_PORT = omaCfgObj[DefaultPort2] ;
   if ( "undefined" == typeof(OMA_PORT_DEFAULT_SDBCM_PORT) || "" == OMA_PORT_DEFAULT_SDBCM_PORT )
      throw -6 ;
}
catch( e )
{
   OMA_PORT_DEFAULT_SDBCM_PORT            = "11790" ;
}
var OMA_PORT_MAX                           = 65535 ;
var OMA_PORT_INVALID                       = -1 ;
var OMA_PORT_TEMP_AGENT_PORT               = 13742 ;
var OMA_RESERVED_PORT                      = [ 11790, [11800, 11804], [11810, 11814], [11820, 11824], 30000, 50000, 60000 ] ;
// option
var OMA_OPTION_SDBCMART_I                  = "--I" ;
var OMA_OPTION_SDBCMART_PORT               = "--port" ;
var OMA_OPTION_SDBCMART_STANDALONE         = "--standalone" ;
var OMA_OPTION_SDBCMART_ALIVETIME          = "--alivetime" ;

// other
var OMA_NEW_LINE                           = "" ;
if ( SYS_LINUX == SYS_TYPE )
{
   OMA_NEW_LINE = "\n" ;
}
else
{
   OMA_NEW_LINE = "\r\n" ;
}
var OMA_SYS_CATALOG_RG                     = "SYSCatalogGroup" ;
var OMA_SYS_COORD_RG                       = "SYSCoord" ;
var OMA_LINUX                              = "LINUX" ;
var OMA_WINDOWS                            = "WINDOWS" ;
var OMA_TMP_SDBCM_ALIVE_TIME               = 300 ; // sec
var OMA_SLEEP_TIME                         = 500 ; // ms
var OMA_TRY_TIMES                          = 6 ;
var OMA_WAIT_CATA_RG_TRY_TIMES             = 600 ; // sec
var OMA_GTE_VERSION_TIME                   = 10000 // ms
var OMA_WAIT_CATALOG_TRY_TIMES             = 30 ; 
var OMA_WAIT_ZN_TRY_TIMES                  = 30 ;

var OMA_WAIT_PRIMARY_NODE_TIMES            = 30 ;
var OMA_REELECT_TIMEOUT                    = 60 ; //seconds