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

   Source File Name = omagentDef.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/29/2014  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef OMAGENT_DEF_HPP__
#define OMAGENT_DEF_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "msgDef.hpp"

namespace engine
{

   /*
      [ sdbcm.conf ] Config Param Define
   */
   #define SDBCM_CONF_DFTPORT          "defaultPort"
   #define SDBCM_CONF_PORT             "_Port"
   #define SDBCM_RESTART_COUNT         "RestartCount"
   #define SDBCM_RESTART_INTERVAL      "RestartInterval"       // minute
   #define SDBCM_AUTO_START            "AutoStart"
   #define SDBCM_DIALOG_LEVEL          "DiagLevel"
   #define SDBCM_CONF_OMADDR           "OMAddress"
   #define SDBCM_CONF_ISGENERAL        "IsGeneral"
   #define SDBCM_ENABLE_WATCH          "EnableWatch"

   #define SDBCM_DFT_PORT              11790
   #define SDBCM_OPTION_PREFIX         "--"

   /*
      CM REMOTE CODE DEFINE
   */
   enum CM_REMOTE_OP_CODE
   {
      SDBSTART       = 1,
      SDBSTOP        = 2,
      SDBADD         = 3,
      SDBMODIFY      = 4,
      SDBRM          = 5,
      SDBSTARTALL    = 6,
      SDBSTOPALL     = 7,
      SDBGETCONF     = 8,
      SDBCLEARDATA   = 9,
      SDBTEST        = 10,
   } ;

   /*
      SDB STOP RETURN RC
   */
   #define STOPFAIL              1
   #define STOPPART              3

   /*
      cm define
   */
   #define SDBCM_CONF_DIR_NAME         "conf"
   #define SDBCM_LOCAL_DIR_NAME        "local"
   #define SDBCM_LOG_DIR_NAME          "log"
   #define SDBOMA_SCRIPT_DIR_NAME      "script"

   #define SDBCM_EXE_FILE_NAME         "sdbcm"
   #define SDBCM_CFG_FILE_NAME         SDBCM_EXE_FILE_NAME".conf"
   #define SDBCM_DIALOG_FILE_NAME      SDBCM_EXE_FILE_NAME".log"

   #define SDB_CM_ROOT_PATH            ".." OSS_FILE_SEP SDBCM_CONF_DIR_NAME OSS_FILE_SEP
   #define SDBCM_CONF_PATH_FILE        SDB_CM_ROOT_PATH SDBCM_CFG_FILE_NAME
   #define SDBCM_LOCAL_PATH            SDB_CM_ROOT_PATH SDBCM_LOCAL_DIR_NAME
   #define SDBCM_LOG_PATH              SDB_CM_ROOT_PATH SDBCM_LOG_DIR_NAME
   #define SDBOMA_SCRIPT_PATH          SDB_CM_ROOT_PATH SDBOMA_SCRIPT_DIR_NAME

#if defined (_LINUX)
      #define SDBSTARTPROG             "sdbstart"
      #define SDBSTOPPROG              "sdbstop"
      #define SDBSDBCMPROG             SDBCM_EXE_FILE_NAME
#elif defined (_WINDOWS)
      #define SDBSTARTPROG             "sdbstart.exe"
      #define SDBSTOPPROG              "sdbstop.exe"
      #define SDBSDBCMPROG             SDBCM_EXE_FILE_NAME".exe"
#endif

   #define SDB_OMA_USER                "OMA_ADMIN"
   #define SDB_OMA_USERPASSWD          "OMA_ADMIN_PASSWD"

   /*
      oma sync command define
   */
   #define OMA_CMD_SCAN_HOST                          OM_SCAN_HOST_REQ
   #define OMA_CMD_PRE_CHECK_HOST                     OM_PRE_CHECK_HOST
   #define OMA_CMD_CHECK_HOST                         OM_CHECK_HOST_REQ
   #define OMA_CMD_POST_CHECK_HOST                    OM_POST_CHECK_HOST

   #define OMA_CMD_HANDLE_TASK_NOTIFY                 OM_NOTIFY_TASK
   #define OMA_CMD_REMOVE_HOST                        OM_REMOVE_HOST_REQ
   #define OMA_CMD_UPDATE_HOSTS                       OM_UPDATE_HOSTNAME_REQ
   #define OMA_CMD_QUERY_HOST_STATUS                  OM_QUERY_HOST_STATUS_REQ

   #define OMA_CMD_SYNC_BUSINESS_CONF                 OM_SYNC_BUSINESS_CONF_REQ

   #define OMA_CMD_CREATE_RELATIONSHIP                OM_CREATE_RELATIONSHIP_REQ
   #define OMA_CMD_REMOVE_RELATIONSHIP                OM_REMOVE_RELATIONSHIP_REQ

   #define OMA_CMD_START_PLUGIN                       "start plugins"
   #define OMA_CMD_STOP_PLUGIN                        "stop plugins"

   /*
      oma background command
   */

   #define OMA_CMD_ADD_HOST                           OM_ADD_HOST_REQ
   #define OMA_CMD_REMOVE_HOST                        OM_REMOVE_HOST_REQ
   #define OMA_CMD_CHECK_ADD_HOST_INFO                "check add host info"

   #define OMA_CMD_INSTALL_TMP_COORD                  "install temporary coord"
   #define OMA_CMD_REMOVE_TMP_COORD                   "remove temporary coord"
   #define OMA_CMD_INSTALL_STANDALONE                 "install standalone"
   #define OMA_CMD_INSTALL_COORD                      "install coord"
   #define OMA_CMD_INSTALL_CATALOG                    "install catalog"
   #define OMA_CMD_INSTALL_DATA_NODE                  "install data node"
   #define OMA_CMD_RM_STANDALONE                      "remove standalone"
   #define OMA_CMD_RM_CATA_RG                         "remove cataloggroup"
   #define OMA_CMD_RM_COORD_RG                        "remove coordgroup"
   #define OMA_CMD_RM_DATA_RG                         "remove datagroup"
   #define OMA_CMD_EXTEND_SEQUOIADB                   "extend sequoiadb"
   #define OMA_CMD_SHRINK_BUSINESS                    "shrink business"
   #define OMA_CMD_DEOLOY_PACKAGE                     "deploy package"
   #define OMA_CMD_ADD_BUSINESS                       "add business"
   #define OMA_CMD_REMOVE_BUSINESS                    "remove business"
   #define OMA_ROLLBACK_STANDALONE                    "rollback installed standalone"
   #define OMA_ROLLBACK_CATALOG                       "rollback installed catalog"
   #define OMA_ROLLBACK_COORD                         "rollback installed coord"
   #define OMA_ROLLBACK_DATA_RG                       "rollback installed data groups"
   #define OMA_CMD_INSTALL_ZOOKEEPER                  "install zookeeper node"
   #define OMA_CMD_REMOVE_ZOOKEEPER                   "remove zookeeper node"
   #define OMA_CMD_CHECK_ZOOKEEPER                    "check zookeeper node"
   #define OMA_CMD_CHECK_ZOOKEEPER_ENV                "check install zookeeper env"
   #define OMA_CMD_INSTALL_SEQUOIASQL_OLAP            "install sequoiasql olap"
   #define OMA_CMD_REMOVE_SEQUOIASQL_OLAP             "remove sequoiasql olap"
   #define OMA_CMD_CHECK_SEQUOIASQL_OLAP              "check sequoiasql olap"
   #define OMA_CMD_TRUST_SEQUOIASQL_OLAP              "trust sequoiasql olap"
   #define OMA_CMD_CHECK_HDFS_SEQUOIASQL_OLAP         "check HDFS for sequoiasql olap"
   #define OMA_CMD_INIT_CLUSTER_SEQUOIASQL_OLAP       "init cluster for sequoiasql olap"

   #define OMA_CMD_SSQL_EXEC                          "sequoiasql exec"
   #define OMA_CMD_CLEAN_SSQL_EXEC                    "clean sequoiasql exec"
   #define OMA_CMD_GET_PSQL                           "get psql"


   #define OMA_INIT_ENV                               "init for exeuting js"

   #define OMA_CMD_UPDATE_TASK                        "update task"


   /*
      oma js file
   */
   #define FILE_DEFINE                      "define.js"
   #define FILE_COMMON                      "common.js"
   #define FILE_LOG                         "log.js"
   #define FILE_FUNC                        "func.js"

   #define FILE_SCAN_HOST                   "scanHost.js"

   #define FILE_CHECK_HOST_INIT             "checkHostInit.js"
   #define FILE_CHECK_HOST                  "checkHost.js"
   #define FILE_CHECK_HOST_ITEM             "checkHostItem.js"
   #define FILE_CHECK_HOST_FINAL            "checkHostFinal.js"

   #define FILE_ADD_HOST                    "addHost.js"
   #define FIEL_ADD_HOST_CHECK_INFO         "addHostCheckInfo.js"
   #define FILE_REMOVE_HOST                 "removeHost.js"
   #define FILE_UPDATE_HOSTS_INFO           "updateHostsInfo.js"

   #define FILE_QUERY_HOSTSTATUS            "queryHostStatus.js"
   #define FILE_QUERY_HOSTSTATUS_ITEM       "queryHostStatusItem.js"

   #define FILE_INSTALL_STANDALONE          "installStandalone.js"
   #define FILE_INSTALL_CATALOG             "installCatalog.js"
   #define FILE_INSTALL_COORD               "installCoord.js"
   #define FILE_INSTALL_DATANODE            "installDataNode.js"
   #define FILE_INSTALL_TMP_COORD           "installTmpCoord.js"

   #define FILE_REMOVE_STANDALONE           "removeStandalone.js"
   #define FILE_REMOVE_CATALOG_RG           "removeCatalogRG.js"
   #define FILE_REMOVE_COORD_RG             "removeCoordRG.js"
   #define FILE_REMOVE_DATA_RG              "removeDataRG.js"
   #define FILE_REMOVE_TMP_COORD            "removeTmpCoord.js"

   #define FILE_ROLLBACK_STANDALONE         "rollbackStandalone.js"
   #define FILE_ROLLBACK_CATALOG            "rollbackCatalog.js"
   #define FILE_ROLLBACK_COORD              "rollbackCoord.js"
   #define FILE_ROLLBACK_DATA_RG            "rollbackDataRG.js"

   #define FILE_ADD_BUSINESS                "addBusiness.js"
   #define FILE_REMOVE_BUSINESS             "removeBusiness.js"
   #define FILE_EXTEND_SEQUOIADB            "extendSequoiaDB.js"
   #define FILE_SHRINK_BUSINESS             "shrinkBusiness.js"

   #define FILE_DEPLOY_PACKAGE              "deployPackage.js"

   #define FILE_SYNC_BUSINESS_CONF          "syncBusinessConf.js"

   #define FILE_CREATE_RELATIONSHIP         "createRelationship.js"
   #define FILE_REMOVE_RELATIONSHIP         "removeRelationship.js"

   #define FILE_INIT_ENV                    "initEnv.js"
   #define FILE_INSTALL_ZOOKEEPER           "installZNode.js"
   #define FILE_REMOVE_ZOOKEEPER            "removeZNode.js"
   #define FILE_CHECK_ZOOKEEPER             "checkZNodes.js"
   #define FILE_CHECK_ZOOKEEPER_ENV         "checkZNEnv.js"

   #define FILE_SEQUOIASQL_OLAP_COMMON      "ssqlOlapCommon.js"
   #define FILE_SEQUOIASQL_OLAP_CHECK       "ssqlOlapCheck.js"
   #define FILE_SEQUOIASQL_OLAP_CHECK_HDFS  "ssqlOlapCheckHdfs.js"
   #define FILE_SEQUOIASQL_OLAP_TRUST       "ssqlOlapTrust.js"
   #define FILE_SEQUOIASQL_OLAP_CONFIG      "ssqlOlapConfig.js"
   #define FILE_SEQUOIASQL_OLAP_INSTALL     "ssqlOlapInstall.js"
   #define FILE_SEQUOIASQL_OLAP_INIT        "ssqlOlapInit.js"
   #define FILE_SEQUOIASQL_OLAP_REMOVE      "ssqlOlapRemove.js"

   #define FILE_RUN_PSQL                    "runPsql.js"
   #define FILE_CLEAN_SSQL_EXEC             "cleanPsql.js"
   #define FILE_GET_PSQL                    "getPsql.js"

   #define FILE_START_PLUGINS               "startPlugins.js"
   #define FILE_STOP_PLUGINS                "stopPlugins.js"

   /*
      oma js argument type
   */
   #define JS_ARG_BUS                       "BUS_JSON"
   #define JS_ARG_SYS                       "SYS_JSON"
   #define JS_ARG_ENV                       "ENV_JSON"
   #define JS_ARG_OTHER                     "OTHER_JSON"

   /*
      oma create role
   */
   #define ROLE_STANDALONE                  "standalone"
   #define ROLE_COORD                       "coord"
   #define ROLE_CATA                        "catalog"
   #define ROLE_DATA                        "data"

   /*
      oma deplay mode
   */
   #define DEPLAY_SA                        "standalone"
   #define DEPLAY_DB                        "distribution"

   #define EXTEND_HORZ                      "horizontal"
   #define EXTEND_VERT                      "vertical"

   /*
      oma misc
   */
   #define OMA_BUFF_SIZE                    (1024)
   #define JS_FILE_NAME_LEN                 (512)
   #define WAITING_TIME                     (3000)

   /*
      remote command define
   */
   #define OMA_REMOTE_SYS_PING                    "system ping"
   #define OMA_REMOTE_SYS_TYPE                    "system type"
   #define OMA_REMOTE_SYS_GET_RELEASE_INFO        "system get release info"
   #define OMA_REMOTE_SYS_GET_HOSTS_MAP           "system get hosts map"
   #define OMA_REMOTE_SYS_GET_AHOST_MAP           "system get a host map"
   #define OMA_REMOTE_SYS_ADD_AHOST_MAP           "system add a host map"
   #define OMA_REMOTE_SYS_DEL_AHOST_MAP           "system delete a host map"
   #define OMA_REMOTE_SYS_GET_CPU_INFO            "system get cpu info"
   #define OMA_REMOTE_SYS_SNAPSHOT_CPU_INFO       "system snapshot cpu info"
   #define OMA_REMOTE_SYS_GET_MEM_INFO            "system get mem info"
   #define OMA_REMOTE_SYS_SNAPSHOT_MEM_INFO       "system snapshot mem info"
   #define OMA_REMOTE_SYS_GET_DISK_INFO           "system get disk info"
   #define OMA_REMOTE_SYS_SNAPSHOT_DISK_INFO      "system snapshot disk info"
   #define OMA_REMOTE_SYS_GET_NETCARD_INFO        "system get netcard info"
   #define OMA_REMOTE_SYS_SNAPSHOT_NETCARD_INFO   "system snapshot netcard info"
   #define OMA_REMOTE_SYS_GET_IPTABLES_INFO       "system get ip tables info"
   #define OMA_REMOTE_SYS_GET_HOSTNAME            "system get hostname"
   #define OMA_REMOTE_SYS_SNIFF_PORT              "system sniff port"
   #define OMA_REMOTE_SYS_GET_PID                 "system get pid"
   #define OMA_REMOTE_SYS_GET_TID                 "system get tid"
   #define OMA_REMOTE_SYS_GET_EWD                 "system get ewd"
   #define OMA_REMOTE_SYS_LIST_PROC               "system list process"
   #define OMA_REMOTE_SYS_KILL_PROCESS            "system kill process"
   #define OMA_REMOTE_SYS_ADD_USER                "system add user"
   #define OMA_REMOTE_SYS_SET_USER_CONFIGS        "system set user configs"
   #define OMA_REMOTE_SYS_DEL_USER                "system del user"
   #define OMA_REMOTE_SYS_ADD_GROUP               "system add group"
   #define OMA_REMOTE_SYS_DEL_GROUP               "system del group"
   #define OMA_REMOTE_SYS_LIST_LOGIN_USERS        "system list login users"
   #define OMA_REMOTE_SYS_LIST_ALL_USERS          "system list all users"
   #define OMA_REMOTE_SYS_LIST_GROUPS             "system list all groups"
   #define OMA_REMOTE_SYS_GET_CURRENT_USER        "system get current user"
   #define OMA_REMOTE_SYS_GET_PROC_ULIMIT_CONFIGS "system get proc ulimit configs"
   #define OMA_REMOTE_SYS_SET_PROC_ULIMIT_CONFIGS "system set proc ulimit configs"
   #define OMA_REMOTE_SYS_GET_SYSTEM_CONFIGS      "system get system configs"
   #define OMA_REMOTE_SYS_GET_USER_ENV            "system get user env"
   #define OMA_REMOTE_SYS_RUN_SERVICE             "system run service"
   #define OMA_REMOTE_SYS_BUILD_TRUSTY            "system build trusty"
   #define OMA_REMOTE_SYS_REMOVE_TRUSTY           "system remove trusty"
   #define OMA_REMOTE_CMD_RUN                     "cmd run"
   #define OMA_REMOTE_CMD_START                   "cmd start"
   #define OMA_REMOTE_CMD_RUN_JS                  "cmd run js"
   #define OMA_REMOTE_FILE_OPEN                   "file open"
   #define OMA_REMOTE_FILE_READ                   "file read"
   #define OMA_REMOTE_FILE_WRITE                  "file write"
   #define OMA_REMOTE_FILE_SEEK                   "file seek"
   #define OMA_REMOTE_FILE_CLOSE                  "file close"
   #define OMA_REMOTE_FILE_REMOVE                 "file remove"
   #define OMA_REMOTE_FILE_ISEXIST                "file is exist"
   #define OMA_REMOTE_FILE_COPY                   "file copy"
   #define OMA_REMOTE_FILE_MOVE                   "file move"
   #define OMA_REMOTE_FILE_MKDIR                  "file mkdir"
   #define OMA_REMOTE_FILE_FIND                   "file find"
   #define OMA_REMOTE_FILE_CHMOD                  "file chmod"
   #define OMA_REMOTE_FILE_CHOWN                  "file chown"
   #define OMA_REMOTE_FILE_CHGRP                  "file chgrp"
   #define OMA_REMOTE_FILE_SET_UMASK              "file set umask"
   #define OMA_REMOTE_FILE_GET_UMASK              "file get umask"
   #define OMA_REMOTE_FILE_LIST                   "file list"
   #define OMA_REMOTE_FILE_GET_PATH_TYPE          "file get path type"
   #define OMA_REMOTE_FILE_IS_EMPTYDIR            "file is empty dir"
   #define OMA_REMOTE_FILE_STAT                   "file stat"
   #define OMA_REMOTE_FILE_MD5                    "file md5"
   #define OMA_REMOTE_FILE_GET_CONTENT_SIZE       "file get content size"
   #define OMA_REMOTE_FILE_GET_PERMISSION         "file get permission"
   #define OMA_REMOTE_FILE_READ_LINE              "file read line"
   #define OMA_REMOTE_OMA_TEST                    "oma test"
   #define OMA_REMOTE_OMA_GET_OMA_INSTALL_FILE    "oma get oma install file"
   #define OMA_REMOTE_OMA_GET_OMA_INSTALL_INFO    "oma get oma install info"
   #define OMA_REMOTE_OMA_GET_OMA_CONFIG_FILE     "oma get oma config file"
   #define OMA_REMOTE_OMA_GET_OMA_CONFIGS         "oma get oma configs"
   #define OMA_REMOTE_OMA_SET_OMA_CONFIGS         "oma set oma configs"
   #define OMA_REMOTE_OMA_GET_A_OMA_SVC_NAME      "oma get a oma svc name"
   #define OMA_REMOTE_OMA_ADD_A_OMA_SVC_NAME      "oma add a oma svc name"
   #define OMA_REMOTE_OMA_DEL_A_OMA_SVC_NAME      "oma del a oma svc name"
   #define OMA_REMOTE_OMA_LIST_NODES              "oma list nodes"
   #define OMA_REMOTE_OMA_GET_NODE_CONFIGS        "oma get node configs"
   #define OMA_REMOTE_OMA_SET_NODE_CONFIGS        "oma set node configs"
   #define OMA_REMOTE_OMA_UPDATE_NODE_CONFIGS     "oma update node configs"
   #define OMA_REMOTE_OMA_RELOAD_CONFIGS          CMD_NAME_RELOAD_CONFIG
}

#endif // OMAGENT_DEF_HPP__

