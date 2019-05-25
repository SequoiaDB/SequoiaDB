/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = omDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/15/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OM_DEF_HPP__
#define OM_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "omagentDef.hpp"

namespace engine
{


   #define OM_CS_DEPLOY                         "SYSDEPLOY"

   /*
   get business config and get business template
   operation type
   */
   #define  OM_FIELD_OPERATION_DEPLOY           "deploy"
   #define  OM_FIELD_OPERATION_EXTEND           "extend"

   /***************** Rest field **********************/
   #define OM_REST_FIELD_CLUSTER_NAME           "ClusterName"
   #define OM_REST_FIELD_BUSINESS_NAME          "BusinessName"
   #define OM_REST_FIELD_PACKAGENAME            "PackageName"
   #define OM_REST_FIELD_INSTALLPATH            "InstallPath"
   #define OM_REST_FIELD_HOST_INFO              "HostInfo"
   #define OM_REST_FIELD_USER                   "User"
   #define OM_REST_FIELD_PASSWORD               "Passwd"
   #define OM_REST_FIELD_ENFORCED               "Enforced"
   #define OM_REST_FIELD_CONFIGINFO             "ConfigInfo"
   #define OM_REST_FIELD_OPERATION_TYPE         "OperationType"
   #define OM_REST_FIELD_FROM                   "From"
   #define OM_REST_FIELD_TO                     "To"
   #define OM_REST_FIELD_OPTIONS                "Options"
   #define OM_REST_FIELD_FORCE                  "Force"
   #define OM_REST_FIELD_SQL                    "Sql"
   #define OM_REST_FIELD_DB_NAME                "DbName"
   #define OM_REST_FIELD_NAME                   "Name"
   #define OM_REST_FIELD_HOST_NAME              "HostName"
   #define OM_REST_FIELD_SERVICE_NAME           "ServiceName"
   #define OM_REST_FIELD_ROLE                   "Role"
   #define OM_REST_FIELD_TYPE                   "Type"
   #define OM_REST_FIELD_PUBLIC_KEY             "PublicKey"

   #define OM_REST_VALUE_PLUGIN                 "plugin"

   /***************** Bson field *****************/
   #define OM_BSON_HOST_INFO                    "HostInfo"
   #define OM_BSON_HOSTNAME                     "HostName"
   #define OM_BSON_IP                           "IP"
   #define OM_BSON_DBPATH                       "dbpath"
   #define OM_BSON_PORT                         "port"
   #define OM_BSON_SVCNAME                      "svcname"
   #define OM_BSON_ROLE                         "role"
   #define OM_BSON_DATAGROUPNAME                "datagroupname"
   #define OM_BSON_CLUSTER_NAME                 "ClusterName"
   #define OM_BSON_BUSINESS_NAME                "BusinessName"
   #define OM_BSON_BUSINESS_TYPE                "BusinessType"
   #define OM_BSON_DEPLOY_MOD                   "DeployMod"
   #define OM_BSON_OPERATION_TYPE               "OperationType"
   #define OM_BSON_BUSINESS_INFO                "BusinessInfo"
   #define OM_BSON_PROPERTY                     "Property"
   #define OM_BSON_NAME                         "Name"
   #define OM_BSON_VALUE                        "Value"
   #define OM_BSON_INFO                         "Info"
   #define OM_BSON_CONFIG                       "Config"
   #define OM_BSON_ZOOID                        "zooid"
   #define OM_BSON_USER                         "User"
   #define OM_BSON_USER2                        "user"
   #define OM_BSON_PASSWD                       "Passwd"
   #define OM_BSON_PASSWD2                      "passwd"
   #define OM_BSON_PASSWORD                     "Password"
   #define OM_BSON_PASSWORD2                    "password"
   #define OM_BSON_INSTALL_PATH                 "InstallPath"
   #define OM_BSON_SSHPORT                      "SshPort"
   #define OM_BSON_ADDRESS                      "Address"
   #define OM_BSON_FROM                         "From"
   #define OM_BSON_TO                           "To"
   #define OM_BSON_OPTIONS                      "Options"
   #define OM_BSON_SERVICENAME                  "ServiceName"
   #define OM_BSON_DB_NAME                      "DbName"
   #define OM_BSON_SQL                          "Sql"
   #define OM_BSON_LEASETIME                    "LeaseTime"
   #define OM_BSON_LEASETIME2                   "leaseTime"

   /***************** XML field *****************/
   #define OM_XML_FIELD_BUSINESS_TYPE           "BusinessType"
   #define OM_XML_FIELD_DEPLOY_MOD              "DeployMod"
   #define OM_XML_FIELD_PROPERTY                "Property"
   #define OM_XML_FIELD_SEPARATE_CONFIG         "SeparateConfig"
   #define OM_XML_FIELD_NAME                    "Name"

   /***************** CL table field ******************/

   #define OM_PUBLIC_FIELD_CLUSTERNAME          "ClusterName"
   #define OM_PUBLIC_FIELD_AGENT_SERVICE        "AgentService"
   #define OM_PUBLIC_FIELD_SDBUSER              "SdbUser"
   #define OM_PUBLIC_FIELD_SDBPASSWD            "SdbPasswd"
   #define OM_PUBLIC_FIELD_SDBUSERGROUP         "SdbUserGroup"
   #define OM_PUBLIC_FIELD_IP                   "IP"
   #define OM_PUBLIC_FIELD_BUSINESS_TYPE        "BusinessType"
   #define OM_PUBLIC_FIELD_BUSINESS_NAME        "BusinessName"
   #define OM_PUBLIC_FIELD_DEPLOY_MOD           "DeployMod"
   #define OM_PUBLIC_FIELD_SSHPORT              "SshPort"
   #define OM_PUBLIC_FIELD_USER                 "User"
   #define OM_PUBLIC_FIELD_PASSWD               "Passwd"
   #define OM_PUBLIC_FIELD_HOSTNAME             "HostName"

   /******* SYSCLUSTER *******/
   #define OM_CS_DEPLOY_CL_CLUSTER              OM_CS_DEPLOY".SYSCLUSTER"

   #define OM_CLUSTER_FIELD_NAME                OM_PUBLIC_FIELD_CLUSTERNAME
   #define OM_CLUSTER_FIELD_DESC                "Desc"
   #define OM_CLUSTER_FIELD_SDBUSER             "SdbUser"
   #define OM_CLUSTER_FIELD_SDBPASSWD           "SdbPasswd"
   #define OM_CLUSTER_FIELD_SDBUSERGROUP        "SdbUserGroup"
   #define OM_CLUSTER_FIELD_INSTALLPATH         "InstallPath"
   #define OM_CLUSTER_FIELD_GRANTCONF           "GrantConf"
   #define OM_CLUSTER_FIELD_GRANTNAME           "Name"
   #define OM_CLUSTER_FIELD_PRIVILEGE           "Privilege"
   #define OM_CLUSTER_FIELD_HOSTFILE            "HostFile"
   #define OM_CLUSTER_FIELD_ROOTUSER            "RootUser"

   #define OM_CS_DEPLOY_CL_CLUSTERIDX1          "{name:\"SYSDEPLOY_CLUSTER_IDX1\"\
,key:{"OM_CLUSTER_FIELD_NAME":1}, unique: true, enforced: true }"

   /******* SYSBUSINESS *******/
   #define OM_CS_DEPLOY_CL_BUSINESS             OM_CS_DEPLOY".SYSBUSINESS"

   #define OM_BUSINESS_FIELD_NAME               OM_PUBLIC_FIELD_BUSINESS_NAME
   #define OM_BUSINESS_FIELD_TYPE               OM_PUBLIC_FIELD_BUSINESS_TYPE
   #define OM_BUSINESS_FIELD_DEPLOYMOD          OM_PUBLIC_FIELD_DEPLOY_MOD
   #define OM_BUSINESS_FIELD_CLUSTERNAME        OM_PUBLIC_FIELD_CLUSTERNAME
   #define OM_BUSINESS_FIELD_TIME               "Time"
   #define OM_BUSINESS_FIELD_ADDTYPE            "AddtionType"
   #define OM_BUSINESS_FIELD_INFO               "BusinessInfo"
   #define OM_BUSINESS_FIELD_LOCATION           "Location"
   #define OM_BUSINESS_FIELD_ID                 "_id"

   #define OM_BUSINESS_ADDTYPE_DISCOVERY        1
   #define OM_BUSINESS_ADDTYPE_INSTALL          0

   #define OM_CS_DEPLOY_CL_BUSINESSIDX1         "{name:\"SYSDEPLOY_BUSINESS_IDX1\"\
,key:{"OM_BUSINESS_FIELD_NAME":1}, unique: true, enforced: true }"

   /******* SYSCONFIGURE *******/
   #define OM_CS_DEPLOY_CL_CONFIGURE            OM_CS_DEPLOY".SYSCONFIGURE"
   #define OM_CONFIGURE_FIELD_HOSTNAME          OM_PUBLIC_FIELD_HOSTNAME
   #define OM_CONFIGURE_FIELD_BUSINESSNAME      OM_PUBLIC_FIELD_BUSINESS_NAME
   #define OM_CONFIGURE_FIELD_BUSINESSTYPE      OM_PUBLIC_FIELD_BUSINESS_TYPE
   #define OM_CONFIGURE_FIELD_CLUSTERNAME       OM_PUBLIC_FIELD_CLUSTERNAME
   #define OM_CONFIGURE_FIELD_DEPLOYMODE        OM_PUBLIC_FIELD_DEPLOY_MOD
   #define OM_CONFIGURE_FIELD_CONFIG            "Config"
   #define OM_CONFIGURE_FIELD_SVCNAME           "svcname"
   #define OM_CONFIGURE_FIELD_ERRNO             "errno"
   #define OM_CONFIGURE_FIELD_DETAIL            "detail"
   #define OM_CONFIGURE_FIELD_ROLE              "role"
   #define OM_CONFIGURE_FIELD_PORT              "Port"
   #define OM_CONFIGURE_FIELD_PORT2             "port"
   #define OM_CONFIGURE_FIELD_INSTALLPATH       "InstallPath"

   /******* SYSHOST *******/
   #define OM_CS_DEPLOY_CL_HOST                 OM_CS_DEPLOY".SYSHOST"
   #define OM_HOST_FIELD_NAME                   OM_PUBLIC_FIELD_HOSTNAME
   #define OM_HOST_FIELD_PACKAGES               "Packages"
   #define OM_HOST_FIELD_PACKAGENAME            "Name"
   #define OM_HOST_FIELD_INSTALLPATH            "InstallPath"
   #define OM_HOST_FIELD_VERSION                "Version"
   #define OM_HOST_FIELD_CLUSTERNAME            OM_PUBLIC_FIELD_CLUSTERNAME
   #define OM_HOST_FIELD_IP                     OM_PUBLIC_FIELD_IP
   #define OM_HOST_FIELD_USER                   OM_PUBLIC_FIELD_USER
   #define OM_HOST_FIELD_PASSWD                 OM_PUBLIC_FIELD_PASSWD
   #define OM_HOST_FIELD_PASSWORD               OM_PUBLIC_FIELD_PASSWD
   #define OM_HOST_FIELD_TIME                   "Time"
   #define OM_HOST_FIELD_OS                     "OS"
   #define OM_HOST_FIELD_OMA                    "OMA"
   #define OM_HOST_FIELD_OM_HASINSTALL          "HasInstalled"
   #define OM_HOST_FIELD_OM_VERSION             OM_HOST_FIELD_VERSION
   #define OM_HOST_FIELD_OM_PATH                "Path"
   #define OM_HOST_FIELD_OM_PORT                "Port"
   #define OM_HOST_FIELD_OM_RELEASE             "Release"
   #define OM_HOST_FIELD_MEMORY                 "Memory"
   #define OM_HOST_FIELD_DISK                   "Disk"
   #define OM_HOST_FIELD_DISK_NAME              "Name"
   #define OM_HOST_FIELD_DISK_SIZE              "Size"
   #define OM_HOST_FIELD_DISK_MOUNT             "Mount"
   #define OM_HOST_FIELD_DISK_FREE_SIZE         "Free"
   #define OM_HOST_FIELD_DISK_USED              "Used"
   #define OM_HOST_FIELD_CPU                    "CPU"
   #define OM_HOST_FIELD_NET                    "Net"
   #define OM_HOST_FIELD_NET_NAME               "Name"
   #define OM_HOST_FIELD_NET_IP                 OM_PUBLIC_FIELD_IP
   #define OM_HOST_FIELD_PORT                   "Port"
   #define OM_HOST_FIELD_SERVICE                "Service"
   #define OM_HOST_FIELD_SAFETY                 "Safety"
   #define OM_HOST_FIELD_AGENT_PORT             "AgentService"
   #define OM_HOST_FIELD_SSHPORT                OM_PUBLIC_FIELD_SSHPORT

   #define OM_CS_DEPLOY_CL_HOSTIDX1             "{name:\"SYSDEPLOY_HOST_IDX1\",\
key: {"OM_HOST_FIELD_NAME":1}, unique: true, enforced: true }"

   #define OM_CS_DEPLOY_CL_HOSTIDX2             "{name:\"SYSDEPLOY_HOST_IDX2\",\
key: {"OM_HOST_FIELD_IP":1}, unique: true, enforced: true }"

   /******* SYSTASKINFO *******/
   #define OM_CS_DEPLOY_CL_TASKINFO             OM_CS_DEPLOY".SYSTASKINFO"
   #define OM_TASKINFO_FIELD_TASKID             "TaskID"
   #define OM_TASKINFO_FIELD_TYPE               "Type"
   #define OM_TASKINFO_FIELD_TYPE_DESC          "TypeDesc"
   #define OM_TASKINFO_FIELD_NAME               "TaskName"
   #define OM_TASKINFO_FIELD_CREATE_TIME        "CreateTime"
   #define OM_TASKINFO_FIELD_END_TIME           "EndTime"
   #define OM_TASKINFO_FIELD_STATUS             "Status"
   #define OM_TASKINFO_FIELD_STATUS_DESC        "StatusDesc"
   #define OM_TASKINFO_FIELD_AGENTHOST          "AgentHost"
   #define OM_TASKINFO_FIELD_AGENTPORT          OM_PUBLIC_FIELD_AGENT_SERVICE
   #define OM_TASKINFO_FIELD_AGENT_SERVICE      OM_PUBLIC_FIELD_AGENT_SERVICE
   #define OM_TASKINFO_FIELD_INFO               "Info"
   #define OM_TASKINFO_FIELD_ERRNO              OP_ERRNOFIELD
   #define OM_TASKINFO_FIELD_DETAIL             OP_ERR_DETAIL
   #define OM_TASKINFO_FIELD_PROGRESS           "Progress"
   #define OM_TASKINFO_FIELD_RESULTINFO         "ResultInfo"
   #define OM_TASKINFO_FIELD_SDBUSER            OM_PUBLIC_FIELD_SDBUSER
   #define OM_TASKINFO_FIELD_SDBPASSWD          OM_PUBLIC_FIELD_SDBPASSWD
   #define OM_TASKINFO_FIELD_SDBUSERGROUP       OM_PUBLIC_FIELD_SDBUSERGROUP
   #define OM_TASKINFO_FIELD_CLUSTERNAME        OM_PUBLIC_FIELD_CLUSTERNAME
   #define OM_TASKINFO_FIELD_INSTALLPACKET      "InstallPacket"
   #define OM_TASKINFO_FIELD_PACKAGENAME        "PackageName"
   #define OM_TASKINFO_FIELD_HOSTINFO           "HostInfo"
   #define OM_TASKINFO_FIELD_ENFORCED           "Enforced"
   #define OM_TASKINFO_FIELD_HOSTNAME           "HostName"
   #define OM_TASKINFO_FIELD_IP                 "IP"
   #define OM_TASKINFO_FIELD_USER               OM_PUBLIC_FIELD_USER
   #define OM_TASKINFO_FIELD_PASSWD             OM_PUBLIC_FIELD_PASSWD
   #define OM_TASKINFO_FIELD_INSTALLPATH        "InstallPath"
   #define OM_TASKINFO_FIELD_VERSION            "Version"
   #define OM_TASKINFO_FIELD_CONFIG             "Config"
   #define OM_TASKINFO_FIELD_SSHPORT            OM_PUBLIC_FIELD_SSHPORT
   #define OM_TASKINFO_FIELD_BUSINESS_TYPE      OM_PUBLIC_FIELD_BUSINESS_TYPE
   #define OM_TASKINFO_FIELD_BUSINESS_NAME      OM_PUBLIC_FIELD_BUSINESS_NAME
   #define OM_TASKINFO_FIELD_DEPLOY_MOD         OM_PUBLIC_FIELD_DEPLOY_MOD
   #define OM_TASKINFO_FIELD_AUTH_USER          "AuthUser"
   #define OM_TASKINFO_FIELD_AUTH_PASSWD        "AuthPasswd"

   #define OM_CS_DEPLOY_CL_TASKINFOIDX1      "{name:\"SYSDEPLOY_TASKINFO_IDX1\",key: {"\
OM_TASKINFO_FIELD_TASKID":1}, unique: true, enforced: true } "

   /******* SYSBUSINESSAUTH *******/
   #define OM_CS_DEPLOY_CL_BUSINESS_AUTH        OM_CS_DEPLOY".SYSBUSINESSAUTH"
   #define OM_AUTH_FIELD_BUSINESS_NAME          OM_PUBLIC_FIELD_BUSINESS_NAME
   #define OM_AUTH_FIELD_USER                   OM_PUBLIC_FIELD_USER
   #define OM_AUTH_FIELD_PASSWD                 OM_PUBLIC_FIELD_PASSWD
   #define OM_CS_DEPLOY_CL_BUSINESSAUTHIDX1     "{name:\"SYSDEPLOY_BUSINESSAUTH\
_IDX1\",key: {"OM_BUSINESS_FIELD_NAME":1}, unique: true, enforced: true }"

   /******* SYSRELATIONSHIP *******/
   #define OM_CS_DEPLOY_CL_RELATIONSHIP         OM_CS_DEPLOY".SYSRELATIONSHIP"
   #define OM_RELATIONSHIP_FIELD_NAME           "Name"
   #define OM_RELATIONSHIP_FIELD_FROM           "From"
   #define OM_RELATIONSHIP_FIELD_TO             "To"
   #define OM_RELATIONSHIP_FIELD_OPTIONS        "Options"
   #define OM_RELATIONSHIP_FIELD_CREATETIME     "CreateTime"
   #define OM_CS_DEPLOY_CL_RELATIONSHIPIDX1     "{name:\"SYSDEPLOY_RELATIONSHIP\
_IDX1\",key: {"OM_RELATIONSHIP_FIELD_NAME":1}, unique: true, enforced: true }"

   /******* SYSPLUGINS *******/
   #define OM_CS_DEPLOY_CL_PLUGINS              OM_CS_DEPLOY".SYSPLUGINS"
   #define OM_PLUGINS_FIELD_NAME                "Name"
   #define OM_PLUGINS_FIELD_BUSINESSTYPE        OM_PUBLIC_FIELD_BUSINESS_TYPE
   #define OM_PLUGINS_FIELD_SERVICENAME         "ServiceName"
   #define OM_PLUGINS_FIELD_UPDATETIME          "UpdateTime"
   #define OM_CS_DEPLOY_CL_PLUGINSIDX1          "{name:\"SYSDEPLOY_PLUGINS_IDX1\
\",key: {"OM_PLUGINS_FIELD_NAME":1}, unique: true, enforced: true }"
   #define OM_CS_DEPLOY_CL_PLUGINSIDX2          "{name:\"SYSDEPLOY_PLUGINS_IDX2\
\",key: {"OM_PLUGINS_FIELD_BUSINESSTYPE":1}, unique: true, enforced: true }"

   /********** SYSSTRATEGY **********/
   #define OM_CS_STRATEGY                       "SYSSTRATEGY"

   /********** SYSBUSINESSTASKPROPERTY **********/
   #define OM_CS_STRATEGY_CL_BUSINESS_TASK_PRO  OM_CS_STRATEGY".SYSBUSINESSTASKPROPERTY"
   #define OM_STRATEGY_FIELD_TASKID             FIELD_NAME_TASKID
   #define OM_STRATEGY_BUSINESSTASKPROIDX1      "{name:\"OM_STRATEGY_BUSINESSTASKPROIDX1\",\
key: {"OM_STRATEGY_FIELD_TASKID":1} }"


   #define OM_PATH_WEB                       "web"
   #define OM_PATH_CONFIG                    "config"
   #define OM_PATH_VERSION                   "version"

   #define OM_VALUE_BOOLEAN_TRUE1             "true"
   #define OM_VALUE_BOOLEAN_TRUE2             "TRUE"
   #define OM_VALUE_BOOLEAN_FALSE1            "false"
   #define OM_VALUE_BOOLEAN_FALSE2            "FALSE"

   #define OM_TEMPLATE_REPLICA_NUM           "replicanum"
   #define OM_TEMPLATE_DATAGROUP_NUM         "datagroupnum"
   #define OM_TEMPLATE_CATALOG_NUM           "catalognum"
   #define OM_TEMPLATE_COORD_NUM             "coordnum"
   #define OM_TEMPLATE_TRANSACTION           PMD_OPTION_TRANSACTIONON

   #define OM_DBPATH_PREFIX_DATABASE         "database"

   #define OM_CONF_DETAIL_EX_DG_NAME         "datagroupname"
   #define OM_CONF_DETAIL_DBPATH             PMD_OPTION_DBPATH
   #define OM_CONF_DETAIL_SVCNAME            PMD_OPTION_SVCNAME
   #define OM_CONF_DETAIL_CATANAME           PMD_OPTION_CATANAME
   #define OM_CONF_DETAIL_DIAGLEVEL          PMD_OPTION_DIAGLEVEL
   #define OM_CONF_DETAIL_ROLE               PMD_OPTION_ROLE
   #define OM_CONF_DETAIL_LOGFSIZE           PMD_OPTION_LOGFILESZ
   #define OM_CONF_DETAIL_LOGFNUM            PMD_OPTION_LOGFILENUM
   #define OM_CONF_DETAIL_TRANSACTION        PMD_OPTION_TRANSACTIONON
   #define OM_CONF_DETAIL_PREINSTANCE        PMD_OPTION_PREFINST
   #define OM_CONF_DETAIL_PCNUM              PMD_OPTION_NUMPAGECLEANERS
   #define OM_CONF_DETAIL_PCINTERVAL         PMD_OPTION_PAGECLEANINTERVAL
   #define OM_CONF_DETAIL_DATAGROUPNAME      "datagroupname"
   #define OM_CONF_DETAIL_TRANSACTIONON      PMD_OPTION_TRANSACTIONON

   #define OM_TEMPLATE_ZOO_NUM               "zoonodenum"

   #define OM_ZOO_CONF_DETAIL_ZOOID          "zooid"
   #define OM_ZOO_CONF_DETAIL_INSTALLPATH    "installpath"
   #define OM_ZOO_CONF_DETAIL_DATAPATH       "datapath"
   #define OM_ZOO_CONF_DETAIL_DATAPORT       "dataport"
   #define OM_ZOO_CONF_DETAIL_ELECTPORT      "electport"
   #define OM_ZOO_CONF_DETAIL_CLIENTPORT     "clientport"
   #define OM_ZOO_CONF_DETAIL_SYNCLIMIT      "synclimit"
   #define OM_ZOO_CONF_DETAIL_INITLIMIT      "initlimit"
   #define OM_ZOO_CONF_DETAIL_TICKTIME       "ticktime"

   #define OM_SEQUOIASQL_DEPLOY_OLTP            "oltp"

   #define OM_SEQUOIASQL_DEPLOY_OLAP            "olap"
   #define OM_SSQL_OLAP_DEPLOY_STANDBY          "deploy_standby"
   #define OM_SSQL_OLAP_SEGMENT_NUM             "segment_num"
   #define OM_SSQL_OLAP_CONF_MASTER_HOST        "master_host"
   #define OM_SSQL_OLAP_CONF_MASTER_PORT        "master_port"
   #define OM_SSQL_OLAP_CONF_STANDBY_HOST       "standby_host"
   #define OM_SSQL_OLAP_CONF_MASTER_DIR         "master_dir"
   #define OM_SSQL_OLAP_CONF_MASTER_TEMP_DIR    "master_temp_dir"
   #define OM_SSQL_OLAP_CONF_HDFS_URL           "hdfs_url"
   #define OM_SSQL_OLAP_CONF_SEGMENT_HOSTS      "segment_hosts"
   #define OM_SSQL_OLAP_CONF_SEGMENT_PORT       "segment_port"
   #define OM_SSQL_OLAP_CONF_SEGMENT_DIR        "segment_dir"
   #define OM_SSQL_OLAP_CONF_SEGMENT_TEMP_DIR   "segment_temp_dir"
   #define OM_SSQL_OLAP_CONF_ROLE               "role"
   #define OM_SSQL_OLAP_CONF_IS_SINGLE          "is_single"
   #define OM_SSQL_OLAP_CONF_INSTALL_DIR        "install_dir"
   #define OM_SSQL_OLAP_CONF_LOG_DIR            "log_dir"
   #define OM_SSQL_OLAP_CONF_MAX_CONNECTIONS    "max_connections"
   #define OM_SSQL_OLAP_CONF_SHARED_BUFFERS     "shared_buffers"
   #define OM_SSQL_OLAP_CONF_REMOVE_IF_FAILED   "remove_if_failed"
   #define OM_SSQL_OLAP_MASTER                  "master"
   #define OM_SSQL_OLAP_STANDBY                 "standby"
   #define OM_SSQL_OLAP_SEGMENT                 "segment"

   /*
      OM Field Define
   */
   #define OM_BUSINESS_SEQUOIADB             "sequoiadb"
   #define OM_BUSINESS_ZOOKEEPER             "zookeeper"
   #define OM_BUSINESS_SPARK                 "spark"
   #define OM_BUSINESS_HDFS                  "hdfs"
   #define OM_BUSINESS_YARN                  "yarn"
   #define OM_BUSINESS_SEQUOIASQL            "sequoiasql"
   #define OM_BUSINESS_SEQUOIASQL_OLAP       "sequoiasql-olap"
   #define OM_BUSINESS_SEQUOIASQL_OLTP       "sequoiasql-oltp"

   /*
      discover businesss respone
   */
   #define OM_BUSINESS_RES_HOSTS             "hosts"

   /*
      addHost's ResultInfo:
      {
        IP,HostName,Status,StatusDesc,errno,detail,Flow
      }
   */
   #define OM_TASKINFO_FIELD_FLOW            "Flow"

   enum omTaskType
   {
      OM_TASK_TYPE_ADD_HOST        = 0,
      OM_TASK_TYPE_REMOVE_HOST     = 1,
      OM_TASK_TYPE_ADD_BUSINESS    = 2,
      OM_TASK_TYPE_REMOVE_BUSINESS = 3,
      OM_TASK_TYPE_SSQL_EXEC       = 4,
      OM_TASK_TYPE_EXTEND_BUSINESS = 5,
      OM_TASK_TYPE_SHRINK_BUSINESS = 6,
      OM_TASK_TYPE_DEPLOY_PACKAGE  = 7,

      OM_TASK_TYPE_END
   } ;

   #define OM_TASK_TYPE_ADD_HOST_STR          "ADD_HOST"
   #define OM_TASK_TYPE_REMOVE_HOST_STR       "REMOVE_HOST"
   #define OM_TASK_TYPE_ADD_BUSINESS_STR      "ADD_BUSINESS"
   #define OM_TASK_TYPE_REMOVE_BUSINESS_STR   "REMOVE_BUSINESS"
   #define OM_TASK_TYPE_SSQL_EXEC_STR         "SSQL_EXEC"
   #define OM_TASK_TYPE_EXTEND_BUSINESS_STR   "EXTEND_BUSINESS"
   #define OM_TASK_TYPE_SHRINK_BUSINESS_STR   "SHRINK_BUSINESS"
   #define OM_TASK_TYPE_DEPLOY_PACKAGE_STR    "DEPLOY_PACKAGE"

   const CHAR *getTaskTypeStr( INT32 taskType ) ;

   enum omTaskStatus
   {
      OM_TASK_STATUS_INIT      = 0,
      OM_TASK_STATUS_RUNNING   = 1,
      OM_TASK_STATUS_ROLLBACK  = 2,
      OM_TASK_STATUS_CANCEL    = 3,
      OM_TASK_STATUS_FINISH    = 4,

      OM_TASK_STATUS_END
   } ;

   #define OM_TASK_STATUS_INIT_STR       "INIT"
   #define OM_TASK_STATUS_RUNNING_STR    "RUNNING"
   #define OM_TASK_STATUS_ROLLBACK_STR   "ROLLBACK"
   #define OM_TASK_STATUS_CANCEL_STR     "CANCEL"
   #define OM_TASK_STATUS_FINISH_STR     "FINISH"

   const CHAR *getTaskStatusStr( INT32 taskStatus ) ;

   #define  OM_REST_LOGIN_HTML               "login.html"
   #define  OM_REST_INDEX_HTML               "index.html"
   #define  OM_REST_FAVICON_ICO              "favicon.ico"

   #define  OM_REST_REDIRECT_LOGIN           "<!DOCTYPE html><html><head>"\
                                             "<meta http-equiv=\"refresh\" content=\"0;url="\
                                             OM_REST_LOGIN_HTML"\"></head></html>"

   #define  OM_REST_REDIRECT_INDEX           "<!DOCTYPE html><html><head>"\
                                             "<meta http-equiv=\"refresh\" content=\"0;url="\
                                             OM_REST_INDEX_HTML"\"></head></html>"

   #define  OM_REST_RES_RETCODE              OP_ERRNOFIELD
   #define  OM_REST_RES_DESP                 OP_ERRDESP_FIELD
   #define  OM_REST_RES_DETAIL               OP_ERR_DETAIL
   #define  OM_REST_RES_LOCAL                "local"

#if defined _WINDOWS
   #define  OM_DEFAULT_INSTALL_PATH          "C:\\Program Files\\sequoiadb\\"
#else
   #define  OM_DEFAULT_INSTALL_PATH          "/opt/sequoiadb/"
#endif

#if defined _WINDOWS
   #define  OM_DEFAULT_INSTALL_ROOT_PATH     "C:\\Program Files\\"
#else
   #define  OM_DEFAULT_INSTALL_ROOT_PATH     "/opt/"
#endif

   #define  OM_DEFAULT_SDB_USER              "sdbadmin"
   #define  OM_DEFAULT_SDB_PASSWD            "sdbadmin"
   #define  OM_DEFAULT_SDB_USERGROUP         "sdbadmin_group"
   #define  OM_REST_LANGUAGE_EN              "en"
   #define  OM_REST_LANGUAGE_ZH_CN           "zh-CN"
   #define  OM_MIN_DISK_FREE_SIZE            (600)
   #define  OM_DEFAULT_LOCAL_HOST            "localhost"
   #define  OM_AGENT_DEFAULT_PORT            SDBCM_DFT_PORT

   /*
      get business config, operation type is extend
   */
   #define OM_REST_DEPLOYMOD_HORIZONTAL      "horizontal"
   #define OM_REST_DEPLOYMOD_VERTICAL        "vertical"


   #define  OM_MSG_TIMEOUT_TWO_HOUR          (2 * 3600 * 1000)
   #define  OM_MSG_TIMEOUT_TEN_SECS          (10 * 1000)

   #define  OM_BASICCHECK_INTERVAL           OM_MSG_TIMEOUT_TWO_HOUR
   #define  OM_INSTALL_AGET_INTERVAL         OM_MSG_TIMEOUT_TWO_HOUR
   #define  OM_CHECK_HOST_INTERVAL           OM_MSG_TIMEOUT_TWO_HOUR
   #define  OM_WAIT_AGENT_UNISTALL_INTERVAL  OM_MSG_TIMEOUT_TWO_HOUR
   #define  OM_WAIT_SCAN_RES_INTERVAL        OM_MSG_TIMEOUT_TWO_HOUR

   #define  OM_NOTIFY_TASK_TIMEOUT           OM_MSG_TIMEOUT_TEN_SECS
   #define  OM_QUERY_HOST_STATUS_TIMEOUT     OM_MSG_TIMEOUT_TEN_SECS

   #define  OM_WAIT_PROGRESS_RES_INTERVAL    (3000)
   #define  OM_WAIT_AGENT_EXIT_RES_INTERVAL  (1000)
   #define  OM_WAIT_UPDATE_HOST_INTERVAL     (OM_MSG_TIMEOUT_TWO_HOUR)

   #define OM_SIZE_MEGABIT                   ( 1024 * 1024 )

   #define OM_INT32_LENGTH                   (20)
   #define OM_INT64_LENGTH                   (20)

   #define  OM_DEFAULT_LOGIN_USER            "admin"
   #define  OM_DEFAULT_LOGIN_PASSWD          "admin"

   #define  OM_DEFAULT_PLUGIN_USER           "plugin"

   #define  OM_REST_FIELD_COMMAND            "cmd"
   #define  OM_CREATE_CLUSTER_REQ            "create cluster"
   #define  OM_QUERY_CLUSTER_REQ             "query cluster"
   #define  OM_LOGIN_REQ                     "login"
   #define  OM_LOGOUT_REQ                    "logout"
   #define  OM_CHANGE_PASSWD_REQ             "change passwd"
   #define  OM_CHECK_SESSION_REQ             "check session"
   #define  OM_SCAN_HOST_REQ                 "scan host"
   #define  OM_CHECK_HOST_REQ                "check host"
   #define  OM_ADD_HOST_REQ                  "add host"
   #define  OM_LIST_HOST_REQ                 "list hosts"
   #define  OM_QUERY_HOST_REQ                "query host"
   #define  OM_QUERY_HOST_STATUS_REQ         "query host status"
   #define  OM_LIST_BUSINESS_TYPE_REQ        "list business type"
   #define  OM_GET_BUSINESS_TEMPLATE_REQ     "get business template"
   #define  OM_CONFIG_BUSINESS_REQ           "get business config"
   #define  OM_INSTALL_BUSINESS_REQ          "add business"
   #define  OM_LIST_NODE_REQ                 "list nodes"
   #define  OM_GET_NODE_CONF_REQ             "get node configure"
   #define  OM_QUERY_NODE_CONF_REQ           "query node configure"
   #define  OM_LIST_BUSINESS_REQ             "list businesses"
   #define  OM_LIST_HOST_BUSINESS_REQ        "list host businesses"
   #define  OM_QUERY_BUSINESS_REQ            "query business"
   #define  OM_REMOVE_CLUSTER_REQ            "remove cluster"
   #define  OM_REMOVE_HOST_REQ               "remove host"
   #define  OM_REMOVE_BUSINESS_REQ           "remove business"
   #define  OM_PREDICT_CAPACITY_REQ          "predict capacity"
   #define  OM_GET_LOG_REQ                   "get log"
   #define  OM_LIST_TASK_REQ                 "list tasks"
   #define  OM_SET_BUSINESS_AUTH_REQ         "set business authority"
   #define  OM_REMOVE_BUSINESS_AUTH_REQ      "remove business authority"
   #define  OM_QUERY_BUSINESS_AUTH_REQ       "query business authority"
   #define  OM_DISCOVER_BUSINESS_REQ         "discover business"
   #define  OM_UNDISCOVER_BUSINESS_REQ       "undiscover business"

   #define  OM_TASK_STRATEGY_LIST_REQ        "list task strategy"
   #define  OM_TASK_STRATEGY_ADD_REQ         "add task strategy"
   #define  OM_TASK_STRATEGY_UPDATE_NICE_REQ "update task strategy nice"
   #define  OM_TASK_STRATEGY_ADD_IPS_REQ     "add task strategy ips"
   #define  OM_TASK_STRATEGY_DEL_IPS_REQ     "del task strategy ips"
   #define  OM_TASK_STRATEGY_DEL_REQ         "del task strategy"

   #define  OM_UPDATE_HOST_INFO_REQ          "update host info"

   #define  OM_SSQL_EXEC_REQ                 "ssql exec"
   #define  OM_GET_SYSTEM_INFO_REQ           "get system info"
   #define  OM_EXTEND_BUSINESS_REQ           "extend business"
   #define  OM_SHRINK_BUSINESS_REQ           "shrink business"
   #define  OM_SYNC_BUSINESS_CONF_REQ        "sync business configure"
   #define  OM_GRANT_SYSCONF_REQ             "grant sysconf"
   #define  OM_UNBIND_BUSINESS_REQ           "unbind business"
   #define  OM_UNBIND_HOST_REQ               "unbind host"
   #define  OM_DEPLOY_PACKAGE_REQ            "deploy package"

   #define  OM_CREATE_RELATIONSHIP_REQ       "create relationship"
   #define  OM_REMOVE_RELATIONSHIP_REQ       "remove relationship"
   #define  OM_LIST_RELATIONSHIP_REQ         "list relationship"

   #define  OM_REGISTER_PLUGIN_REQ           "register plugin"
   #define  OM_LIST_PLUGIN_REQ               "list plugins"


   #define  OM_REST_CLUSTER_INFO             "ClusterInfo"
   #define  OM_BSON_FIELD_CLUSTER_DESC       OM_CLUSTER_FIELD_DESC
   #define  OM_BSON_FIELD_SDB_USER           OM_CLUSTER_FIELD_SDBUSER
   #define  OM_BSON_FIELD_SDB_PASSWD         OM_CLUSTER_FIELD_SDBPASSWD
   #define  OM_BSON_FIELD_SDB_USERGROUP      OM_CLUSTER_FIELD_SDBUSERGROUP
   #define  OM_BSON_FIELD_INSTALL_PATH       OM_CLUSTER_FIELD_INSTALLPATH
   #define  OM_BSON_FIELD_GRANTCONF          OM_CLUSTER_FIELD_GRANTCONF
   #define  OM_REST_HEAD_SESSIONID           "SdbSessionID"
   #define  OM_REST_HEAD_LANGUAGE            "SdbLanguage"
   #define  OM_REST_FIELD_LOGIN_NAME         "User"
   #define  OM_REST_FIELD_LOGIN_PASSWD       "Passwd"
   #define  OM_REST_FIELD_TIMESTAMP          "Timestamp"
   #define  OM_REST_FIELD_NEW_PASSWD         "NewPasswd"

   #define  OM_REST_FIELD_TASK_ID            "TaskID"
   #define  OM_REST_FIELD_RULE_ID            "RuleID"
   #define  OM_REST_FIELD_USER_NAME          "UserName"
   #define  OM_REST_FIELD_TASK_NAME          "TaskName"
   #define  OM_REST_FIELD_IPS                "IPs"
   #define  OM_REST_FIELD_NICE               "Nice"

   #define  OM_REST_FIELD_ADDRESS            "Address"
   #define  OM_BSON_FIELD_HOST_INFO          OM_REST_FIELD_HOST_INFO
   #define  OM_BSON_FIELD_HOST_IP            OM_HOST_FIELD_IP
   #define  OM_BSON_FIELD_HOST_NAME          OM_HOST_FIELD_NAME
   #define  OM_BSON_FIELD_HOST_USER          OM_HOST_FIELD_USER
   #define  OM_BSON_FIELD_HOST_PASSWD        OM_HOST_FIELD_PASSWORD
   #define  OM_BSON_FIELD_HOST_SSHPORT       OM_HOST_FIELD_SSHPORT
   #define  OM_BSON_FIELD_AGENT_PORT         OM_HOST_FIELD_AGENT_PORT
   #define  OM_BSON_FIELD_SCAN_STATUS        "Status"
   #define  OM_SCAN_HOST_STATUS_FINISH       "finish"
   #define  OM_BSON_FIELD_OS                 OM_HOST_FIELD_OS
   #define  OM_BSON_FIELD_OMA                OM_HOST_FIELD_OMA
   #define  OM_BSON_FIELD_MEMORY             "Memory"
   #define  OM_BSON_FIELD_DISK               OM_HOST_FIELD_DISK
   #define  OM_BSON_FIELD_DISK_NAME          OM_HOST_FIELD_DISK_NAME
   #define  OM_BSON_FIELD_DISK_SIZE          OM_HOST_FIELD_DISK_SIZE
   #define  OM_BSON_FIELD_DISK_MOUNT         OM_HOST_FIELD_DISK_MOUNT
   #define  OM_BSON_FIELD_DISK_FREE_SIZE     OM_HOST_FIELD_DISK_FREE_SIZE
   #define  OM_BSON_FIELD_DISK_USED          OM_HOST_FIELD_DISK_USED
   #define  OM_BSON_FIELD_DISK_CANUSE        "CanUse"
   #define  OM_BSON_FIELD_CPU                OM_HOST_FIELD_CPU
   #define  OM_BSON_FIELD_NET                OM_HOST_FIELD_NET
   #define  OM_BSON_FIELD_PORT               OM_HOST_FIELD_PORT
   #define  OM_BSON_FIELD_SERVICE            OM_HOST_FIELD_SERVICE
   #define  OM_BSON_FIELD_SAFETY             OM_HOST_FIELD_SAFETY
   #define  OM_BSON_FIELD_CONFIG             OM_CONFIGURE_FIELD_CONFIG
   #define  OM_BSON_FIELD_NEEDUNINSTALL      "IsNeedUninstall"
   #define  OM_BSON_FIELD_PATCKET_PATH       "InstallPacket"
   #define  OM_PACKET_SUBPATH                "packet"
   #define  OM_BSON_FIELD_CPU_SYS            "Sys"
   #define  OM_BSON_FIELD_CPU_IDLE           "Idle"
   #define  OM_BSON_FIELD_CPU_OTHER          "Other"
   #define  OM_BSON_FIELD_CPU_USER           "User"
   #define  OM_BSON_FIELD_CPU_MEGABIT        "Megabit"
   #define  OM_BSON_FIELD_CPU_UNIT           "Unit"
   #define  OM_BSON_FIELD_NET_MEGABIT        OM_BSON_FIELD_CPU_MEGABIT
   #define  OM_BSON_FIELD_NET_UNIT           OM_BSON_FIELD_CPU_UNIT
   #define  OM_BSON_FIELD_NET_NAME           "Name"
   #define  OM_BSON_FIELD_NET_RXBYTES        "RXBytes"
   #define  OM_BSON_FIELD_NET_RXPACKETS      "RXPackets"
   #define  OM_BSON_FIELD_NET_RXERRORS       "RXErrors"
   #define  OM_BSON_FIELD_NET_RXDROPS        "RXDrops"
   #define  OM_BSON_FIELD_NET_TXBYTES        "TXBytes"
   #define  OM_BSON_FIELD_NET_TXPACKETS      "TXPackets"
   #define  OM_BSON_FIELD_NET_TXERRORS       "TXErrors"
   #define  OM_BSON_FIELD_NET_TXDROPS        "TXDrops"
   #define  OM_BSON_FIELD_NET_IP             OM_HOST_FIELD_IP
   #define  OM_BUSINESS_CONFIG_SUBDIR        "config"
   #define  OM_BUSINESS_FILE_NAME            "business"
   #define  OM_BSON_BUSINESS_LIST            "BusinessList"
   #define  OM_BSON_TASKID                   "TaskID"
   #define  OM_BSON_TASKTYPE                 "TaskType"
   #define  OM_BSON_FIELD_SVCNAME            FIELD_NAME_SERVICE_NAME
   #define  OM_BSON_FIELD_ROLE               FIELD_NAME_ROLE
   #define  OM_REST_BUSINESS_NAME            OM_BSON_BUSINESS_NAME
   #define  OM_REST_SVCNAME                  FIELD_NAME_SERVICE_NAME
   #define  OM_REST_HOST_NAME                OM_BSON_FIELD_HOST_NAME
   #define  OM_REST_ISFORCE                  "IsForce"
   #define  OM_SDB_AUTH_USER                 "AuthUser"
   #define  OM_SDB_AUTH_PASSWD               "AuthPasswd"
   #define  OM_BSON_FIELD_VALID_SIZE         "ValidSize"
   #define  OM_BSON_FIELD_TOTAL_SIZE         "TotalSize"
   #define  OM_BSON_FIELD_REDUNDANCY_RATE    "RedundancyRate"
   #define  OM_REST_LOG_NAME                 "Name"

   #define  OM_REST_WEB_SERVICE_PORT         "WebServicePort"
   #define  OM_REST_DBNAME                   "DbName"
   #define  OM_REST_DBUSER                   OM_BSON_FIELD_HOST_USER
   #define  OM_REST_DBPASSWD                 OM_BSON_FIELD_HOST_PASSWD
   #define  OM_REST_SQL                      "Sql"
   #define  OM_REST_RESULT_FORMAT            "ResultFormat"

   #define  OM_BSON_FIELD_DBUSER             "DbUser"
   #define  OM_BSON_FIELD_DBPASSWD           "DbPasswd"

   #define OM_BSON_FIELD_HOSTS               "Hosts"

   #define OM_NODE_ROLE_STANDALONE           SDB_ROLE_STANDALONE_STR
   #define OM_NODE_ROLE_COORD                SDB_ROLE_COORD_STR
   #define OM_NODE_ROLE_CATALOG              SDB_ROLE_CATALOG_STR
   #define OM_NODE_ROLE_DATA                 SDB_ROLE_DATA_STR
   #define OM_NODE_COORDLIST                 "Coord"

   #define OM_DEPLOY_MOD_STANDALONE          "standalone"
   #define OM_DEPLOY_MOD_DISTRIBUTION        "distribution"

   #define  OM_CONF_PATH_STR                 "conf"
   #define  OM_LOG_PATH_STR                  "log"

   #define  OM_CONFIG_FILE_TYPE              ".xml"
   #define  OM_XMLATTR_KEY                   "<xmlattr>"
   #define  OM_XMLATTR_TYPE                  "<xmlattr>.type"
   #define  OM_XMLATTR_TYPE_ARRAY            "array"
   #define  OM_EXTEND_TEMPLATE_FILE_NAME     "_"OM_FIELD_OPERATION_EXTEND
   #define  OM_TEMPLATE_FILE_NAME            "_template"
   #define  OM_REST_BUSINESS_TYPE            OM_BSON_BUSINESS_TYPE
   #define  OM_BSON_DEPLOY_MOD_LIST          "DeployModList"
   #define  OM_BSON_DEPLOY_MOD               "DeployMod"
   #define  OM_BSON_SEPARATE_CONFIG          "SeparateConfig"
   #define  OM_BSON_PROPERTY_ARRAY           "Property"
   #define  OM_BSON_PROPERTY_NAME            "Name"
   #define  OM_BSON_PROPERTY_TYPE            "Type"
   #define  OM_BSON_PROPERTY_DEFAULT         "Default"
   #define  OM_BSON_PROPERTY_VALID           "Valid"
   #define  OM_BSON_PROPERTY_DISPLAY         "Display"
   #define  OM_BSON_PROPERTY_EDIT            "Edit"
   #define  OM_BSON_PROPERTY_DESC            "Desc"
   #define  OM_BSON_PROPERTY_LEVEL           "Level"
   #define  OM_BSON_PROPERTY_WEBNAME         "WebName"
   #define  OM_CONFIG_ITEM_FILE_NAME         "_config"
   #define  OM_XML_CONFIG                    "config"
   #define  OM_REST_TEMPLATE_INFO            "TemplateInfo"
   #define  OM_BSON_PROPERTY_VALUE           "Value"

   #define  OM_PRE_CHECK_HOST                "pre-check host"
   #define  OM_POST_CHECK_HOST               "post-check host"
   #define  OM_UPDATE_HOSTNAME_REQ           "update hostname"
   #define  OM_AGENT_UPDATE_TASK             "update task"
   #define  OM_QUERY_TASK_REQ                "query task"
   #define  OM_NOTIFY_TASK                   "notify task"
   #define  OM_SSQL_GET_MORE_REQ             "ssql getmore"
   #define  OM_INTERRUPT_TASK_REQ            "interrupt task"

   #define  OM_REST_HEAD_CLUSTERNAME         "sdbClusterName"
   #define  OM_REST_HEAD_BUSINESSNAME        "sdbBusinessName"


   #define REST_KEY_NAME_COLLECTIONSPACE     "Collectionspace"
   #define REST_KEY_NAME_COLLECTION          "Collectionname"
   #define REST_KEY_NAME_SUBCLNAME           "Subclname"
   #define REST_KEY_NAME_ORDERBY             "Orderby"
   #define REST_KEY_NAME_LIMIT               "Limit"
   #define REST_KEY_NAME_MATCHER             "Matcher"
   #define REST_KEY_NAME_FLAG                "Flag"
   #define REST_KEY_NAME_INSERTOR            "Insertor"
   #define REST_KEY_NAME_UPDATOR             "Updator"
   #define REST_KEY_NAME_DELETOR             "Deletor"
   #define REST_KEY_NAME_SET_ON_INSERT       "Setoninsert"
   #define REST_KEY_NAME_SQL                 "Sql"
   #define REST_KEY_NAME_LOWBOUND            "Lowbound"
   #define REST_KEY_NAME_UPBOUND             "Upbound"
   #define REST_KEY_NAME_CODE                "Code"
   #define REST_KEY_NAME_FUNCTION            "Function"

   #define REST_VALUE_FLAG_UPDATE_KEEP_SK          "SDB_UPDATE_KEEP_SHARDINGKEY"
   #define REST_VALUE_FLAG_QUERY_KEEP_SK_IN_UPDATE "SDB_QUERY_KEEP_SHARDINGKEY_IN_UPDATE"
   #define REST_VALUE_FLAG_QUERY_FORCE_HINT        "SDB_QUERY_FORCE_HINT"
   #define REST_VALUE_FLAG_QUERY_PARALLED          "SDB_QUERY_PARALLED"
   #define REST_VALUE_FLAG_QUERY_WITH_RETURNDATA   "SDB_QUERY_WITH_RETURNDATA"
   #define REST_FLAG_SEP "|"

}

#endif // OM_DEF_HPP__

