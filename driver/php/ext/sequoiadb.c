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

#include "php_sequoiadb.h"
#include "class_sequoiadb.h"
#include "class_securesdb.h"
#include "class_cs.h"
#include "class_cl.h"
#include "class_cursor.h"
#include "class_group.h"
#include "class_node.h"
#include "class_domain.h"
#include "class_lob.h"
#include "class_int64.h"
#include "class_objectid.h"
#include "class_date.h"
#include "class_timestamp.h"
#include "class_regex.h"
#include "class_binary.h"
#include "class_decimal.h"

/* ========== PHP entry pointer ========== */

//Sdb object
zend_class_entry *pSequoiadbSdb ;
zend_class_entry *pSecureSdb ;
zend_class_entry *pSequoiadbCs ;
zend_class_entry *pSequoiadbCl ;
zend_class_entry *pSequoiadbCursor ;
zend_class_entry *pSequoiadbGroup ;
zend_class_entry *pSequoiadbNode ;
zend_class_entry *pSequoiadbDomain ;
zend_class_entry *pSequoiadbLob ;

//Sdb Type
zend_class_entry *pSequoiadbInt64 ;
zend_class_entry *pSequoiadbId ;
zend_class_entry *pSequoiadbData ;
zend_class_entry *pSequoiadbTimeStamp ;
zend_class_entry *pSequoiadbRegex ;
zend_class_entry *pSequoiadbBinary ;
zend_class_entry *pSequoiadbMinKey ;
zend_class_entry *pSequoiadbMaxKey ;
zend_class_entry *pSequoiadbDecimal ;

//resource id
INT32 connectionDesc ;
INT32 csDesc ;
INT32 clDesc ;
INT32 cursorDesc ;
INT32 groupDesc ;
INT32 nodeDesc ;
INT32 domainDesc ;
INT32 lobDesc ;
INT32 dateDesc ;
INT32 timestampDesc ;
INT32 decimalDesc ;

PHP_FUNCTION( sdbInitClient ) ;

//Sdb function
const zend_function_entry normalFun[] = {
   //driver function
   PHP_FE( sdbInitClient, NULL )
   PHP_FE_END
};

//Sdb object function
const zend_function_entry sdbFun[] = {
   //driver
   PHP_ME( SequoiaDB, __construct,           NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR )
   PHP_ME( SequoiaDB, install,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getLastErrorMsg,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, cleanLastErrorMsg,     NULL, ZEND_ACC_PUBLIC )
   //db
   PHP_ME( SequoiaDB, connect,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, close,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, isValid,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, syncDB,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, invalidateCache,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, interrupt,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, interruptOperation,    NULL, ZEND_ACC_PUBLIC )
   //snapshot & list
   PHP_ME( SequoiaDB, snapshot,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, resetSnapshot,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, list,                  NULL, ZEND_ACC_PUBLIC )
   //cs
   PHP_ME( SequoiaDB, listCS,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, selectCS,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, createCS,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getCS,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, dropCS,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, renameCS,              NULL, ZEND_ACC_PUBLIC )
   //cl
   PHP_ME( SequoiaDB, listCL,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getCL,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, truncate,              NULL, ZEND_ACC_PUBLIC )
   //domain
   PHP_ME( SequoiaDB, listDomain,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, createDomain,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getDomain,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, dropDomain,            NULL, ZEND_ACC_PUBLIC )
   //group
   PHP_ME( SequoiaDB, listGroup,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getGroup,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, createGroup,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, removeGroup,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, createCataGroup,       NULL, ZEND_ACC_PUBLIC )
   //sql
   PHP_ME( SequoiaDB, execSQL,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, execUpdateSQL,         NULL, ZEND_ACC_PUBLIC )
   //user
   PHP_ME( SequoiaDB, createUser,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, removeUser,            NULL, ZEND_ACC_PUBLIC )
   //config
   PHP_ME( SequoiaDB, flushConfigure,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, updateConfig,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, deleteConfig,          NULL, ZEND_ACC_PUBLIC )
   //procedure
   PHP_ME( SequoiaDB, listProcedure,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, createJsProcedure,     NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, removeProcedure,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, evalJs,                NULL, ZEND_ACC_PUBLIC )
   //transaction
   PHP_ME( SequoiaDB, transactionBegin,      NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, transactionCommit,     NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, transactionRollback,   NULL, ZEND_ACC_PUBLIC )
   //backup
   PHP_ME( SequoiaDB, backup,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, listBackup,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, removeBackup,          NULL, ZEND_ACC_PUBLIC )
   //task
   PHP_ME( SequoiaDB, listTask,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, waitTask,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, cancelTask,            NULL, ZEND_ACC_PUBLIC )
   //session
   PHP_ME( SequoiaDB, setSessionAttr,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, getSessionAttr,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDB, forceSession,          NULL, ZEND_ACC_PUBLIC )
   //analyze
   PHP_ME( SequoiaDB, analyze,               NULL, ZEND_ACC_PUBLIC )
   //Rename
   PHP_MALIAS( SequoiaDB, getError, getLastErrorMsg, NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, getSnapshot, snapshot,     NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, getList,     list,         NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, listCSs,     listCS,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, listCollectionSpaces,
                                       listCS,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, listCollections,
                                       listCL,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, getCollectionSpace,
                                       getCS,        NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, dropCollectionSpace,
                                       dropCS,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, listDomains,
                                   listDomain,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, selectGroup,
                                     getGroup,       NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaDB, backupOffline,
                                       backup,       NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry secureFun[] = {
   PHP_ME( SecuresDB, __construct,   NULL, ZEND_ACC_PUBLIC|ZEND_ACC_CTOR )
   PHP_ME( SecuresDB, connect,       NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry csFun[] = {
   PHP_ME( SequoiaCS, __construct,     NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   //cs
   PHP_ME( SequoiaCS, drop,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, getName,         NULL, ZEND_ACC_PUBLIC )
   //cl
   PHP_ME( SequoiaCS, selectCL,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, createCL,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, getCL,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, listCL,          NULL, ZEND_ACC_PUBLIC )   
   PHP_ME( SequoiaCS, dropCL,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, renameCL,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, alter,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, setDomain,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, getDomainName,   NULL, ZEND_ACC_PUBLIC )   
   PHP_ME( SequoiaCS, removeDomain,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, enableCapped,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, disableCapped,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCS, setAttributes,   NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaCS, selectCollection,
                             selectCL, NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaCS, dropCollection,
                               dropCL, NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry clFun[] = {
   PHP_ME( SequoiaCL, __construct,           NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   //cl
   PHP_ME( SequoiaCL, drop,                  NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, alter,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, enableSharding,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, disableSharding,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, enableCompression,     NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, disableCompression,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, setAttributes,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, split,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, splitAsync,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getFullName,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getCSName,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getName,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, attachCL,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, detachCL,              NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, createAutoIncrement,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, dropAutoIncrement,     NULL, ZEND_ACC_PUBLIC )
   //record
   PHP_ME( SequoiaCL, insert,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, bulkInsert,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, remove,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, update,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, upsert,                NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, find,                  NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, findAndUpdate,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, findAndRemove,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, explain,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, count,                 NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, aggregate,             NULL, ZEND_ACC_PUBLIC )
   //index
   PHP_ME( SequoiaCL, createIndex,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, dropIndex,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getIndex,              NULL, ZEND_ACC_PUBLIC )//deprecated
   PHP_ME( SequoiaCL, getIndexes,            NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getIndexInfo,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, getIndexStat,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, createIdIndex,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, dropIdIndex,           NULL, ZEND_ACC_PUBLIC )
   //lob
   PHP_ME( SequoiaCL, openLob,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, removeLob,             NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, truncateLob,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, listLob,               NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, listLobPieces,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCL, createLobID,           NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaCL, deleteIndex, dropIndex,
                                             NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaCL, getCollectionName, getName,
                                             NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry cursorFun[] = {
   PHP_ME( SequoiaCursor, __construct,    NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   PHP_ME( SequoiaCursor, next,           NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaCursor, current,        NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaCursor, getNext, next, NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry groupFun[] = {
   PHP_ME( SequoiaGroup, __construct,  NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   //group
   PHP_ME( SequoiaGroup, isCatalog,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, getName,      NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, start,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, stop,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, reelect,      NULL, ZEND_ACC_PUBLIC )
   //node
   PHP_ME( SequoiaGroup, getNodeNum,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, getDetail,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, getMaster,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, getSlave,     NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, getNode,      NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, createNode,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, removeNode,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, attachNode,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaGroup, detachNode,   NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry nodeFun[] = {
   PHP_ME( SequoiaNode, __construct,    NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   PHP_ME( SequoiaNode, getName,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaNode, getHostName,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaNode, getServiceName, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaNode, connect,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaNode, start,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaNode, stop,           NULL, ZEND_ACC_PUBLIC )
   PHP_MALIAS( SequoiaNode, getNodeName, getName, NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry domainFun[] = {
   PHP_ME( SequoiaDomain, __construct,  NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   PHP_ME( SequoiaDomain, alter,        NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, addGroups,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, setGroups,    NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, removeGroups, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, setAttributes, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, listCS,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, listCL,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDomain, listGroup,    NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry lobFun[] = {
   PHP_ME( SequoiaLob, __construct,   NULL, ZEND_ACC_PRIVATE|ZEND_ACC_CTOR )
   PHP_ME( SequoiaLob, close,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, getSize,       NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, getCreateTime, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, getModificationTime, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, write,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, read,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, seek,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, lock,          NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, lockAndSeek,   NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, isEof,         NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaLob, getRunTimeDetail, NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

//Sdb type function
const zend_function_entry int64Fun[] = {
   PHP_ME( SequoiaINT64, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaINT64, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry idFun[] = {
   PHP_ME( SequoiaID, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaID, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry dateFun[] = {
   PHP_ME( SequoiaDate, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDate, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry timestampFun[] = {
   PHP_ME( SequoiaTimestamp, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaTimestamp, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry regexFun[] = {
   PHP_ME( SequoiaRegex, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaRegex, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry binaryFun[] = {
   PHP_ME( SequoiaBinary, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaBinary, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

const zend_function_entry minKeyFun[] = {
   PHP_FE_END
};

const zend_function_entry maxKeyFun[] = {
   PHP_FE_END
};

const zend_function_entry decimalFun[] = {
   PHP_ME( SequoiaDecimal, __construct, NULL, ZEND_ACC_PUBLIC )
   PHP_ME( SequoiaDecimal, __toString,  NULL, ZEND_ACC_PUBLIC )
   PHP_FE_END
};

zend_module_entry sequoiadb_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
   STANDARD_MODULE_HEADER,
#endif
   "SequoiaDB",
   normalFun,
   PHP_MINIT(sequoiadb),
   PHP_MSHUTDOWN(sequoiadb),
   PHP_RINIT(sequoiadb),
   PHP_RSHUTDOWN(sequoiadb),
   PHP_MINFO(sequoiadb),
#if ZEND_MODULE_API_NO >= 20010901
   "0.1",
#endif
   STANDARD_MODULE_PROPERTIES
};


#ifdef COMPILE_DL_SEQUOIADB
ZEND_GET_MODULE(sequoiadb)
#endif

PHP_MINIT_FUNCTION(sequoiadb)
{
   zend_class_entry entrySdb ;
   zend_class_entry entrySecure ;
   zend_class_entry enrtyCs ;
   zend_class_entry entryCl ;
   zend_class_entry entryCursor ;
   zend_class_entry entryDomain ;
   zend_class_entry entryLob ;
   zend_class_entry entryGroup ;
   zend_class_entry entryNode ;

   zend_class_entry entryINT64 ;
   zend_class_entry entryID ;
   zend_class_entry entryDate ;
   zend_class_entry entryTimeStamp ;
   zend_class_entry entryRegex ;
   zend_class_entry entryBinary ;
   zend_class_entry entryMinKey ;
   zend_class_entry entryMaxKey ;
   zend_class_entry entryDecimal ;


   //Register resource
   //connect
   PHP_REGISTER_RESOURCE( php_sdb_destroy, NULL,
                          SDB_HANDLE_NAME, connectionDesc ) ;
   //cs
   PHP_REGISTER_RESOURCE( php_cs_destroy, NULL,
                          SDB_CS_HANDLE_NAME, csDesc ) ;
   //cl
   PHP_REGISTER_RESOURCE( php_cl_destroy, NULL,
                          SDB_CL_HANDLE_NAME, clDesc ) ;
   //cursor
   PHP_REGISTER_RESOURCE( php_cursor_destroy, NULL,
                          SDB_CURSOR_HANDLE_NAME, cursorDesc ) ;
   //group
   PHP_REGISTER_RESOURCE( php_group_destroy, NULL,
                          SDB_GROUP_HANDLE_NAME, groupDesc ) ;
   //node
   PHP_REGISTER_RESOURCE( php_node_destroy, NULL,
                          SDB_NODE_HANDLE_NAME, nodeDesc ) ;
   //domain
   PHP_REGISTER_RESOURCE( php_domain_destroy, NULL,
                          SDB_DOMAIN_HANDLE_NAME, domainDesc ) ;
   //lob
   PHP_REGISTER_RESOURCE( php_lob_destroy, NULL,
                          SDB_LOB_HANDLE_NAME, lobDesc ) ;
   //date
   PHP_REGISTER_RESOURCE( php_date_destroy, NULL,
                          SDB_DATE_HANDLE_NAME, dateDesc ) ;
   //timestamp
   PHP_REGISTER_RESOURCE( php_timestamp_destroy, NULL,
                          SDB_TIMESTAMP_HANDLE_NAME, timestampDesc ) ;
   //decimal
   PHP_REGISTER_RESOURCE( php_decimal_destroy, NULL,
                          SDB_DECIMAL_HANDLE_NAME, decimalDesc ) ;

   //Init php class entry
   //Sdb object entry
   INIT_CLASS_ENTRY( entrySdb,       "SequoiaDB",        sdbFun ) ;
   INIT_CLASS_ENTRY( entrySecure,    "SecureSdb",        secureFun ) ;
   INIT_CLASS_ENTRY( enrtyCs,        "SequoiaCS",        csFun ) ;
   INIT_CLASS_ENTRY( entryCl,        "SequoiaCL",        clFun ) ;
   INIT_CLASS_ENTRY( entryCursor,    "SequoiaCursor",    cursorFun ) ;
   INIT_CLASS_ENTRY( entryGroup,     "SequoiaGroup",     groupFun  ) ;
   INIT_CLASS_ENTRY( entryNode,      "SequoiaNode",      nodeFun  ) ;
   INIT_CLASS_ENTRY( entryDomain,    "SequoiaDomain",    domainFun ) ;
   INIT_CLASS_ENTRY( entryLob,       "SequoiaLob",       lobFun ) ;
   //Sdb type entry
   INIT_CLASS_ENTRY( entryINT64,     "SequoiaINT64",     int64Fun  ) ;
   INIT_CLASS_ENTRY( entryID,        "SequoiaID",        idFun ) ;
   INIT_CLASS_ENTRY( entryDate,      "SequoiaDate",      dateFun ) ;
   INIT_CLASS_ENTRY( entryTimeStamp, "SequoiaTimestamp", timestampFun ) ;
   INIT_CLASS_ENTRY( entryRegex,     "SequoiaRegex",     regexFun ) ;
   INIT_CLASS_ENTRY( entryBinary,    "SequoiaBinary",    binaryFun ) ;
   INIT_CLASS_ENTRY( entryMinKey,    "SequoiaMinKey",    minKeyFun ) ;
   INIT_CLASS_ENTRY( entryMaxKey,    "SequoiaMaxKey",    maxKeyFun ) ;
   INIT_CLASS_ENTRY( entryDecimal,   "SequoiaDecimal",   decimalFun ) ;


   //Register internal
   PHP_REGISTER_INTERNAL( entrySdb,       pSequoiadbSdb ) ;
   PHP_REGISTER_INTERNAL( enrtyCs,        pSequoiadbCs ) ;
   PHP_REGISTER_INTERNAL( entryCl,        pSequoiadbCl ) ;
   PHP_REGISTER_INTERNAL( entryCursor,    pSequoiadbCursor ) ;
   PHP_REGISTER_INTERNAL( entryGroup,     pSequoiadbGroup ) ;
   PHP_REGISTER_INTERNAL( entryNode,      pSequoiadbNode ) ;
   PHP_REGISTER_INTERNAL( entryDomain,    pSequoiadbDomain ) ;
   PHP_REGISTER_INTERNAL( entryLob,       pSequoiadbLob ) ;
   PHP_REGISTER_INTERNAL_EX( entrySecure,
                             pSequoiadbSdb,
                             "SequoiaDB",
                             pSecureSdb ) ;

   PHP_REGISTER_INTERNAL( entryINT64,     pSequoiadbInt64 ) ;
   PHP_REGISTER_INTERNAL( entryID,        pSequoiadbId ) ;
   PHP_REGISTER_INTERNAL( entryDate,      pSequoiadbData ) ;
   PHP_REGISTER_INTERNAL( entryTimeStamp, pSequoiadbTimeStamp ) ;
   PHP_REGISTER_INTERNAL( entryRegex,     pSequoiadbRegex ) ;
   PHP_REGISTER_INTERNAL( entryBinary,    pSequoiadbBinary ) ;
   PHP_REGISTER_INTERNAL( entryMinKey,    pSequoiadbMinKey ) ;
   PHP_REGISTER_INTERNAL( entryMaxKey,    pSequoiadbMaxKey ) ;
   PHP_REGISTER_INTERNAL( entryDecimal,   pSequoiadbDecimal ) ;

   //declare class property variable
   //Sdb
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbSdb, "_handle" ) ;
   PHP_DECLARE_PROPERTY_LONG( pSequoiadbSdb, "_return_model", 1 ) ;
   PHP_DECLARE_PROPERTY_LONG( pSequoiadbSdb, "_error", 0 ) ;
   //Collection space
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCs, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCs, "_SequoiaDB" ) ;
   //Collection
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCl, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCl, "_SequoiaDB" ) ;
   //Cursor
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCursor, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbCursor, "_SequoiaDB" ) ;
   //Group
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbGroup, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbGroup, "_SequoiaDB" ) ;
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbGroup, "_name", "" ) ;
   //Node
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbNode, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbNode, "_SequoiaDB" ) ;
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbNode, "_name", "" ) ;
   //Domain
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbDomain, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbDomain, "_SequoiaDB" ) ;
   //Lob
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbLob, "_handle" ) ;
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbLob, "_SequoiaDB" ) ;

   //int64
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbInt64, "INT64", "0" ) ;
   //oid
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbId, "$oid", "" ) ;
   //date
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbData, "$date" ) ;
   //timestamp
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbTimeStamp, "$timestamp" ) ;
   //regex
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbRegex, "$regex", "" ) ;
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbRegex, "$options", "" ) ;
   //binary
   PHP_DECLARE_PUBLIC_STRING( pSequoiadbBinary, "$binary", "" ) ;
   PHP_DECLARE_PROPERTY_LONG( pSequoiadbBinary, "$type", 0 ) ;
   //minKey maxKey
   PHP_DECLARE_PROPERTY_LONG( pSequoiadbMinKey, "$minKey", 1 ) ;
   PHP_DECLARE_PROPERTY_LONG( pSequoiadbMaxKey, "$maxKey", 1 ) ;
   //decimal
   PHP_DECLARE_PROPERTY_NULL( pSequoiadbDecimal, "$decimal" ) ;

   //register constant variable
   //snapshot
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_CONTEXTS",            0 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_CONTEXTS_CURRENT",    1 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_SESSIONS",            2 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_SESSIONS_CURRENT",    3 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_COLLECTIONS",         4 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_COLLECTIONSPACES",    5 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_DATABASE",            6 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_SYSTEM",              7 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_CATALOG",             8 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_TRANSACTIONS",        9 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_TRANSACTIONS_CURRENT",10 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_ACCESSPLANS",         11 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_HEALTH",              12 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_CONFIGS",             13 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_SVCTASKS",            14 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_SEQUENCES",           15 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_QUERIES",             18 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_LATCHWAITS",          19 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_LOCKWAITS",           20 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_INDEXSTATS",          21 ) ;

   /* waste */
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_COLLECTION",          4 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_COLLECTIONSPACE",     5 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_CATA",                8 ) ;
   
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_TRANSACTION",         9 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_SNAP_TRANSACTION_CURRENT", 10 ) ;

   //list
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_CONTEXTS",            0 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_CONTEXTS_CURRENT",    1 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_SESSIONS",            2 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_SESSIONS_CURRENT",    3 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_COLLECTIONS",         4 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_COLLECTIONSPACES",    5 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_STORAGEUNITS",        6 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_GROUPS",              7 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_STOREPROCEDURES",     8 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_DOMAINS",             9 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_TASKS",               10 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_TRANSACTIONS",        11 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_TRANSACTIONS_CURRENT",12 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_SVCTASKS",            14 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_SEQUENCES",           15 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_USERS",               16 ) ;

   // reserved
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_CL_IN_DOMAIN",        129 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_CS_IN_DOMAIN",        130 ) ;

   /* waste */
   PHP_REGISTER_LONG_CONSTANT( "SDB_LIST_SHARDS",              8 ) ;

   //Insert
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_INSERT_CONTONDUP",     0x00000001 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_INSERT_REPLACEONDUP",  0x00000004 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_INSERT_RETURN_OID",    0x10000000 ) ;

   //find and update | find and remove
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_FIND_FORCE_HINT",      0x00000080 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_FIND_PARALLED",        0x00000100 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_FIND_WITH_RETURNDATA", 0x00000200 ) ;

   //query queryAndUpdate queryAndRemove explain
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_FORCE_HINT",     0x00000080 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_PARALLED",       0x00000100 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_WITH_RETURNDATA",0x00000200 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_PREPARE_MORE",   0x00004000 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_KEEP_SHARDINGKEY_IN_UPDATE", 0x00008000 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_FOR_UPDATE",     0x00010000 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_QUERY_FOR_SHARE",      0x00040000 ) ;

   //update
   PHP_REGISTER_LONG_CONSTANT( "SDB_FLG_UPDATE_KEEP_SHARDINGKEY", 0x00008000 ) ;

   //node
   PHP_REGISTER_LONG_CONSTANT( "SDB_NODE_ALL",      0 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_NODE_ACTIVE",   1 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_NODE_INACTIVE", 2 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_NODE_UNKNOWN",  3 ) ;

   //lob
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_CREATEONLY", 0x00000001 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_READ",       0x00000004 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_WRITE",      0x00000008 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_SHAREREAD",  0x00000040 ) ;

   //lob seek
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_SET", 0 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_CUR", 1 ) ;
   PHP_REGISTER_LONG_CONSTANT( "SDB_LOB_END", 2 ) ;

   return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(sequoiadb)
{
   return SUCCESS;
}

PHP_RINIT_FUNCTION(sequoiadb)
{
   return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(sequoiadb)
{
   return SUCCESS;
}

PHP_MINFO_FUNCTION(sequoiadb)
{
   php_info_print_table_start();
   php_info_print_table_header(2, "SequoiaDB support", "enabled");
   php_info_print_table_end();
}

PHP_FUNCTION( sdbInitClient )
{
   INT32 rc = SDB_OK ;
   BOOLEAN enableCacheStrategy = FALSE ;
   INT32 cacheTimeInterval     = 300 ;
   zval *pConfigure     = NULL ;
   zval *pCacheStrategy = NULL ;
   zval *pTimeInterVal  = NULL ;
   zval *pMaxCache      = NULL ;
   zval *pThisObj       = getThis() ;
   sdbClientConf sdbConf ;
   if( PHP_GET_PARAMETERS( "z", &pConfigure ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( php_getArrayType( pConfigure TSRMLS_CC ) != PHP_ASSOCIATIVE_ARRAY )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if( php_assocArrayFind( pConfigure,
                           "enableCacheStrategy",
                           &pCacheStrategy TSRMLS_CC ) == FAILURE )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   php_assocArrayFind( pConfigure,
                       "cacheTimeInterval",
                       &pTimeInterVal TSRMLS_CC ) ;

   rc = php_zval2Bool( pCacheStrategy, &enableCacheStrategy TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   rc = php_zval2Int( pTimeInterVal, &cacheTimeInterval TSRMLS_CC ) ;
   if( rc )
   {
      goto error ;
   }
   if( cacheTimeInterval < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   sdbConf.enableCacheStrategy = enableCacheStrategy ;
   sdbConf.cacheTimeInterval = (UINT32)cacheTimeInterval ;
   rc = initClient( &sdbConf ) ;

   if( rc )
   {
      goto error ;
   }
done:
   RETVAL_LONG( rc ) ;
   return ;
error:
   goto done ;
}
