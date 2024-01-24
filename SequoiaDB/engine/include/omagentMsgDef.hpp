/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = omagentMsgDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          06/30/2014  TZB Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_MSG_DEF_HPP__
#define OMAGENT_MSG_DEF_HPP__

#include "pmdOptions.hpp"
#include "msgDef.hpp"
#include "omDef.hpp"
#include "omagentDef.hpp"

// business type
#define OMA_BUS_TYPE_SEQUOIADB               OM_BUSINESS_SEQUOIADB
#define OMA_BUS_TYPE_SEQUOIASQL_POSTGRESQL   OM_BUSINESS_SEQUOIASQL_POSTGRESQL
#define OMA_BUS_TYPE_SEQUOIASQL_MYSQL        OM_BUSINESS_SEQUOIASQL_MYSQL
#define OMA_BUS_TYPE_SEQUOIASQL_MARIADB      OM_BUSINESS_SEQUOIASQL_MARIADB

#define OMA_BUS_TYPE_ZOOKEEPER         OM_BUSINESS_ZOOKEEPER
#define OMA_BUS_TYPE_SEQUOIASQL_OLAP   OM_BUSINESS_SEQUOIASQL_OLAP
#define OMA_BUS_TYPE_SEQUOIASQL        OM_BUSINESS_SEQUOIASQL
#define OMA_BUS_TYPE_SPARK             OM_BUSINESS_SPARK
#define OMA_BUS_TYPE_HDFS              OM_BUSINESS_HDFS

// field
#define OMA_FIELD_HOSTINFO                         "HostInfo" /* OM_REST_FIELD_HOST_INFO */
#define OMA_FIELD_SSH_PORT                         "SshPort" /* OM_BSON_FIELD_HOST_SSHPORT */
#define OMA_FIELD_AGENT_PORT                       "AgentPort" /* OM_BSON_FIELD_AGENT_PORT */
#define OMA_FIELD_SDBUSER                          "SdbUser" /* OM_CLUSTER_FIELD_SDBUSER */
#define OMA_FIELD_SDBPASSWD                        "SdbPasswd" /* OM_CLUSTER_FIELD_SDBPASSWD */
#define OMA_FIELD_SDBUSERGROUP                     "SdbUserGroup" /* OM_CLUSTER_FIELD_SDBUSERGROUP */
#define OMA_FIELD_INSTALLPATH                      "InstallPath"
#define OMA_FIELD_INSTALLPATH3                     "installpath"
#define OMA_FIELD_TRANSACTION_ID                   "TransactionID" /* OM_BSON_FIELD_TRANSACTION_ID */
#define OMA_FIELD_PACKET_PATH                      "PacketPath"
#define OMA_FIELD_CONFIG                           "Config" /* OM_CONFIGURE_FIELD_CONFIG */
#define OMA_FIELD_PACKAGES                         "Packages"

#define OMA_FIELD_ZOOID3                           "zooid"
#define OMA_FIELD_INSTALLPATH3                     "installpath"
#define OMA_FIELD_DATAPATH3                        "datapath"
#define OMA_FIELD_DATAPORT3                        "dataport"
#define OMA_FIELD_ELECTPORT3                       "electport"
#define OMA_FIELD_CLIENTPORT3                      "clientport"
#define OMA_FIELD_SYNCLIMIT3                       "synclimit"
#define OMA_FIELD_INITLIMIT3                       "initlimit"
#define OMA_FIELD_TICKTIME3                        "ticktime"

#define OMA_FIELD_STAGE_INSTALL                    "install" /* OM_TASK_STATUS_INSTALL */
#define OMA_FIELD_STAGE_UNINSTALL                  "uninstall" /* OM_TASK_STATUS_UNINSTALL */
#define OMA_FIELD_STAGE_ROLLBACK                   "rollback" /* OM_TASK_STATUS_ROLLBACK */

#define OMA_FIELD_HOSTS                            "Hosts"
#define OMA_FIELD_HOSTNAME                         "HostName"
#define OMA_FIELD_USER                             "User"
#define OMA_FIELD_PASSWORD                         "Password"
#define OMA_FIELD_PASSWD                           "Passwd"
#define OMA_FIELD_IP                               "IP"
#define OMA_FIELD_IP2                              "Ip"
#define OMA_FIELD_INFO                             "Info"
#define OMA_FIELD_OMA                              "OMA"


#define OMA_FIELD_PING                             "Ping"
#define OMA_FIELD_SSH                              "Ssh"
#define OMA_FIELD_LOCAL_PACKET_PATH                "LocalPacketPath"
#define OMA_FIELD_REMOTE_PACKET_PATH               "RemotePacketPath"
#define OMA_FIELD_AGENT_IS_RUNNING                 "IsRunning"
#define OMA_FIELD_INSTALL_PATH                     "InstallPath"

#define OMA_FIELD_ERRNO                            "errno"
#define OMA_FIELD_DETAIL                           "detail"
#define OMA_FIELD_HASRUNNING                       "HasRunning"
#define OMA_FIELD_HASPUSH                          "HasPush"
#define OMA_FIELD_PORTHASUSED                      "PortHasUsed"
#define OMA_FIELD_TASKID                           "TaskID"
#define OMA_FIELD_DEPLOYMOD                        "DeployMod"
#define OMA_FIELD_ISFINISH                         "IsFinish"
#define OMA_FIELD_PROGRESS                         "Progress"
#define OMA_FIELD_TOTALCOUNT                       "TotalCount"
#define OMA_FIELD_INSTALLEDCOUNT                   "InstalledCount"
#define OMA_FIELD_UNINSTALLEDCOUNT                 "UninstalledCount"
#define OMA_FIELD_DESC                             "Desc"
#define OMA_FIELD_STANDALONE                       "Standalone"
#define OMA_FIELD_COORD                            "Coord"
#define OMA_FIELD_CATALOG                          "Catalog"
#define OMA_FIELD_DATA                             "Data"
#define OMA_FIELD_ERRMSG                           "ErrMsg"
#define OMA_FIELD_TOTALNUM                         "TotalNum"
#define OMA_FIELD_FINISHNUM                        "FinishNum"
#define OMA_FIELD_SVCNAME                          "svcname"
#define OMA_FIELD_SVCNAME2                         "SvcName"
#define OMA_FIELD_GROUPNAME                        "GroupName"
#define OMA_FIELD_GROUPNAMES                       "GroupNames"
#define OMA_FIELD_PROG_PATH                        "ProgPath"
#define OMA_FIELD_HASINSTALL                       "HasInstall" 
#define OMA_FIELD_HASUNINSTALL                     "HasUninstall" 
#define OMA_FIELD_OMAHOSTNAME                      "OmaHostName"
#define OMA_FIELD_OMASVCNAME                       "OmaSvcName"
#define OMA_FIELD_TMPCOORDHOSTNAME                 "TmpCoordHostName"
#define OMA_FIELD_TMPCOORDSVCNAME                  "TmpCoordSvcName"
#define OMA_FIELD_INSTALLGROUPNAME                 "InstallGroupName"
#define OMA_FIELD_INSTALLGROUPNAMES                "InstallGroupNames"
#define OMA_FIELD_INSTALLHOSTNAME                  "InstallHostName"
#define OMA_FIELD_INSTALLHOSTS                     "InstallHosts"
#define OMA_FIELD_INSTALLSVCNAME                   "InstallSvcName"
#define OMA_FIELD_INSTALLPACKET                    "InstallPacket"
#define OMA_FIELD_INSTALLPATH2                     "InstallPath"
#define OMA_FIELD_INSTALLCONFIG                    "InstallConfig"
#define OMA_FIELD_UNINSTALLHOSTNAME                "UninstallHostName"
#define OMA_FIELD_UNINSTALLSVCNAME                 "UninstallSvcName"
#define OMA_FIELD_UNINSTALLGROUPNAME               "UninstallGroupName"
#define OMA_FIELD_UNINSTALLGROUPNAMES              "UninstallGroupNames"
#define OMA_FIELD_AGENTHOST                        "AgentHost"
#define OMA_FIELD_AGENTSERVICE                     "AgentService"
#define OMA_FIELD_AUTHUSER                         "AuthUser"
#define OMA_FIELD_AUTHPASSWD                       "AuthPasswd"
#define OMA_FIELD_CATAADDR                         "CataAddr"
#define OMA_FIELD_CATASVCNAME                      "CataSvcName"
#define OMA_FIELD_SSHPORT                          "SshPort"
#define OMA_FIELD_HASERROR                         "HasError"
#define OMA_FIELD_HASFINISH                        "HasFinish"
#define OMA_FIELD_TASKTYPE                         "Type"
#define OMA_FIELD_TASKDETAIL                       "TaskDetail"
#define OMA_FIELD_RESULTINFO                       "ResultInfo"
#define OMA_FIELD_ROLE                             "role"
#define OMA_FIELD_STATUSDESC                       "StatusDesc"
#define OMA_FIELD_FLOW                             "Flow"
#define OMA_FIELD_DATAGROUPNAME                    "datagroupname"
#define OMA_FIELD_CLUSTERNAME                      "ClusterName"
#define OMA_FIELD_CLUSTERNAME2                     "clustername"
#define OMA_FIELD_CLUSTERNAME3                     "clustername"
#define OMA_FIELD_BUSINESSNAME                     "BusinessName"
#define OMA_FIELD_BUSINESSNAME2                    "businessname"
#define OMA_FIELD_BUSINESSNAME3                    "businessname"
#define OMA_FIELD_BUSINESSTYPE                     "BusinessType"
#define OMA_FIELD_USERTAG                          "UserTag"
#define OMA_FIELD_USERTAG2                         "usertag"
#define OMA_FIELD_USERTAG3                         "usertag"
#define OMA_FIELD_FORCE                            "Force"

// host info
#define OMA_FIELD_HOST                             "Host"
#define OMA_FIELD_OS                               "OS"
#define OMA_FIELD_OM                               "OM"
#define OMA_FIELD_TIME                             "Time"
#define OMA_FIELD_MEMORY                           "Memory"
#define OMA_FIELD_DISK                             "Disk"
#define OMA_FIELD_DISKS                            "Disks"
#define OMA_FIELD_CPU                              "CPU"
#define OMA_FIELD_CPUS                             "Cpus"
#define OMA_FIELD_NET                              "Net"
#define OMA_FIELD_NETCARDS                         "Netcards"
#define OMA_FIELD_PORT                             "Port"
#define OMA_FIELD_PORT2                            "port"
#define OMA_FIELD_SERVICE                          "Service"
#define OMA_FIELD_SAFETY                           "Safety"
#define OMA_FIELD_SERVERINFO                       "ServerInfo"

// om
#define OMA_FIELD_VERSION                          "Version"
#define OMA_FIELD_PATH                             "Path"

// memory
#define OMA_FIELD_SIZE                             "Size"
#define OMA_FIELD_MODEL                            "Model"
#define OMA_FIELD_FREE                             "Free"
#define OMA_FIELD_UNIT                             "Unit"

// Disk
#define OMA_FIELD_NAME                             "Name"
#define OMA_FIELD_FILESYSTEM                       "Filesystem"
#define OMA_FIELD_MOUNT                            "Mount"
#define OMA_FIELD_ISLOCAL                          "IsLocal"
#define OMA_FIELD_USED                             "Used"

// cpu
#define OMA_FIELD_ID                               "ID"
#define OMA_FIELD_CORE                             "Core"
#define OMA_FIELD_MODEL                            "Model"
#define OMA_FIELD_FREQ                             "Freq"

// net
#define OMA_FIELD_BANDWIDTH                        "Bandwidth"

// port
#define OMA_FIELD_STATUS                           "Status"

// service

// safety
#define OMA_FIELD_CONTEXT                          "Context"

// ssql exec
#define OMA_FIELD_DBUSER                           "DbUser"
#define OMA_FIELD_DBPASSWD                         "DbPasswd"
#define OMA_FIELD_DBNAME                           "DbName"
#define OMA_FIELD_SQL                              "Sql"
#define OMA_FIELD_RESULTFORMAT                     "ResultFormat"

#define OMA_FIELD_PIPE_FILE                        "PipeFile"

#define OMA_FIELD_ROWNUM                           "RowNum"
#define OMA_FIELD_ROWVALUE                         "Value"

// business


// install db business result


// config file
#define OMA_OPTION_DATAGROUPNAME                   "datagroupname"
#define OMA_OPTION_HELP                            PMD_OPTION_HELP
#define OMA_OPTION_VERSION                         PMD_OPTION_VERSION
#define OMA_OPTION_DBPATH                          PMD_OPTION_DBPATH
#define OMA_OPTION_IDXPATH                         PMD_OPTION_IDXPATH
#define OMA_OPTION_CONFPATH                        PMD_OPTION_CONFPATH
#define OMA_OPTION_LOGPATH                         PMD_OPTION_LOGPATH
#define OMA_OPTION_DIAGLOGPATH                     PMD_OPTION_DIAGLOGPATH
#define OMA_OPTION_DIAGLOG_NUM                     PMD_OPTION_DIAGLOG_NUM
#define OMA_OPTION_BKUPPATH                        PMD_OPTION_BKUPPATH
#define OMA_OPTION_MAXPOOL                         PMD_OPTION_MAXPOOL
#define OMA_OPTION_SVCNAME                         PMD_OPTION_SVCNAME
#define OMA_OPTION_REPLNAME                        PMD_OPTION_REPLNAME
#define OMA_OPTION_SHARDNAME                       PMD_OPTION_SHARDNAME
#define OMA_OPTION_CATANAME                        PMD_OPTION_CATANAME
#define OMA_OPTION_RESTNAME                        PMD_OPTION_RESTNAME
#define OMA_OPTION_DIAGLEVEL                       PMD_OPTION_DIAGLEVEL
#define OMA_OPTION_ROLE                            PMD_OPTION_ROLE
#define OMA_OPTION_CATALOG_ADDR                    PMD_OPTION_CATALOG_ADDR
#define OMA_OPTION_LOGFILESZ                       PMD_OPTION_LOGFILESZ
#define OMA_OPTION_LOGFILENUM                      PMD_OPTION_LOGFILENUM
#define OMA_OPTION_TRANSACTIONON                   PMD_OPTION_TRANSACTIONON
#define OMA_OPTION_NUMPRELOAD                      PMD_OPTION_NUMPRELOAD
#define OMA_OPTION_MAX_PREF_POOL                   PMD_OPTION_MAX_PREF_POOL
#define OMA_OPTION_MAX_SUB_QUERY                   PMD_OPTION_MAX_SUB_QUERY
#define OMA_OPTION_MAX_REPL_SYNC                   PMD_OPTION_MAX_REPL_SYNC
#define OMA_OPTION_LOGBUFFSIZE                     PMD_OPTION_LOGBUFFSIZE
#define OMA_OPTION_DMS_TMPBLKPATH                  PMD_OPTION_DMS_TMPBLKPATH
#define OMA_OPTION_SORTBUF_SIZE                    PMD_OPTION_SORTBUF_SIZE
#define OMA_OPTION_HJ_BUFSZ                        PMD_OPTION_HJ_BUFSZ
#define OMA_OPTION__SYNC_STRATEGY                  PMD_OPTION_SYNC_STRATEGY
#define OMA_OPTION_REPL_BUCKET_SIZE                PMD_OPTION_REPL_BUCKET_SIZE
#define OMA_OPTION_MEMDEBUG                        PMD_OPTION_MEMDEBUG
#define OMA_OPTION_MEMDEBUGSIZE                    PMD_OPTION_MEMDEBUGSIZE
#define OMA_OPTION_CATALIST                        PMD_OPTION_CATALIST
#define OMA_OPTION_DPSLOCAL                        PMD_OPTION_DPSLOCAL
#define OMA_OPTION_TRACEON                         PMD_OPTION_TRACEON
#define OMA_OPTION_TRACEBUFSZ                      PMD_OPTION_TRACEBUFSZ
#define OMA_OPTION_SHARINGBRK                      PMD_OPTION_SHARINGBRK
#define OMA_OPTION_DMS_CHK_INTERVAL                PMD_OPTION_DMS_CHK_INTERVAL
#define OMA_OPTION_INDEX_SCAN_STEP                 PMD_OPTION_INDEX_SCAN_STEP
#define OMA_OPTION_START_SHIFT_TIME                PMD_OPTION_START_SHIFT_TIME
#define OMA_OPTION_PREFINST                        PMD_OPTION_PREFINST
#define OMA_OPTION_NUMPAGECLEANERS                 PMD_OPTION_NUMPAGECLEANERS
#define OMA_OPTION_PAGECLEANINTERVAL               PMD_OPTION_PAGECLEANINTERVAL


#endif
