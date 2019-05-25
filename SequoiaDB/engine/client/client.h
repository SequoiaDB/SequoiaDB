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
/** \file client.h
    \brief C Client Driver
*/

#ifndef CLIENT_H__
#define CLIENT_H__
#include "core.h"
#include "ossTypes.h"
#include "bson/bson.h"
#include "jstobs.h"
#include "spd.h"
#include "clientDef.h"

SDB_EXTERN_C_START

#define SDB_PAGESIZE_4K           4096
#define SDB_PAGESIZE_8K           8192
#define SDB_PAGESIZE_16K          16384
#define SDB_PAGESIZE_32K          32768
#define SDB_PAGESIZE_64K          65536
/** 0 means using database's default pagesize, it 64k now */
#define SDB_PAGESIZE_DEFAULT      0

enum _SDB_LOB_OPEN_MODE
{
   SDB_LOB_CREATEONLY = 0x00000001, /**< Open a new lob only */
   SDB_LOB_READ       = 0x00000004, /**< Open an existing lob to read */
   SDB_LOB_WRITE      = 0x00000008  /**< Open an existing lob to write */
} ;
typedef enum _SDB_LOB_OPEN_MODE SDB_LOB_OPEN_MODE ;

enum _SDB_LOB_SEEK
{
   SDB_LOB_SEEK_SET = 0, /**< Seek from the beginning of file */
   SDB_LOB_SEEK_CUR,     /**< Seek from the current place */
   SDB_LOB_SEEK_END      /**< Seek from the end of file  */
} ;
typedef enum _SDB_LOB_SEEK SDB_LOB_SEEK ;

#define SDB_INVALID_HANDLE       ((ossValuePtr) 0)
typedef ossValuePtr sdbConnectionHandle   ;
typedef ossValuePtr sdbCSHandle           ;
typedef ossValuePtr sdbCollectionHandle   ;
typedef ossValuePtr sdbCursorHandle       ;
typedef ossValuePtr sdbReplicaGroupHandle ;
typedef ossValuePtr sdbNodeHandle  ;
typedef ossValuePtr sdbDomainHandle ;
typedef ossValuePtr sdbLobHandle ;
typedef ossValuePtr sdbDCHandle ;

/** Callback function when the reply message is error **/
typedef void (*ERROR_ON_REPLY_FUNC)( const CHAR *pErrorObj,
                                     UINT32 objSize,
                                     INT32 flag,
                                     const CHAR *pDescription,
                                     const CHAR *pDetail ) ;

/** \fn INT32 sdbSetErrorOnReplyCallback ( ERROR_ON_REPLY_FUNC func )
    \brief Set the callback function when reply message if error from server
    \param [in] func The callback function when called on reply error
*/
SDB_EXPORT void sdbSetErrorOnReplyCallback( ERROR_ON_REPLY_FUNC func ) ;

/** sdbReplicaNodeHandle will be deprecated in version 2.x, use sdbNodeHandle instead of it. */
typedef sdbNodeHandle             sdbReplicaNodeHandle ;

/** sdbCreateReplicaNode will be deprecated in version 2.x, use sdbCreateNode instead of it. */
#define sdbCreateReplicaNode      sdbCreateNode
/** sdbRemoveReplicaNode will be deprecated in version 2.x, use sdbRemoveNode instead of it. */
#define sdbRemoveReplicaNode      sdbRemoveNode
/** sdbGetReplicaNodeMaster will be deprecated in version 2.x, use sdbGetNodeMaster instead of it. */
#define sdbGetReplicaNodeMaster   sdbGetNodeMaster
/** sdbGetReplicaNodeSlave will be deprecated in version 2.x, use sdbGetNodeSlave instead of it. */
#define sdbGetReplicaNodeSlave    sdbGetNodeSlave
/** sdbGetReplicaNodeByName will be deprecated in version 2.x, use sdbGetNodeByName instead of it. */
#define sdbGetReplicaNodeByName   sdbGetNodeByName
/** sdbGetReplicaNodeByHost will be deprecated in version 2.x, use sdbGetNodeByHost instead of it. */
#define sdbGetReplicaNodeByHost   sdbGetNodeByHost
/** sdbGetReplicaNodeSddr will be deprecated in version 2.x, use sdbGetNodeAddr instead of it. */
#define sdbGetReplicaNodeSddr     sdbGetNodeAddr
/** sdbStartReplicaNode will be deprecated in version 2.x, use sdbStartNode instead of it. */
#define sdbStartReplicaNode       sdbStartNode
/** sdbStopReplicaNode will be deprecated in version 2.x, use sdbStopNode instead of it. */
#define sdbStopReplicaNode        sdbStopNode
/** sdbReleaseReplicaNode will be deprecated in version 2.x, use sdbReleaseNode instead of it. */
#define sdbReleaseReplicaNode     sdbReleaseNode

/** Force to use specified hint to query, if database have no index assigned by the hint, fail to query. */
#define QUERY_FORCE_HINT                  0x00000080
/** Enable parallel sub query, each sub query will finish scanning diffent part of the data. */
#define QUERY_PARALLED                    0x00000100
/** In general, query won't return data until cursor gets from database, when add this flag, return data in query response, it will be more high-performance */
#define QUERY_WITH_RETURNDATA             0x00000200
/** Enable prepare more data when query */
#define QUERY_PREPARE_MORE                0x00004000
/** The sharding key in update rule is not filtered, when executing queryAndUpdate. */
#define QUERY_KEEP_SHARDINGKEY_IN_UPDATE  0x00008000

/** The sharding key in update rule is not filtered, when executing update or upsert. */
#define UPDATE_KEEP_SHARDINGKEY           QUERY_KEEP_SHARDINGKEY_IN_UPDATE

/** \fn INT32 initClient ( sdbClientConf* config ) ;
    \brief set client global configuration such as cache strategy to improve performance
    \param [in] config The configuration struct, see detail of sdbClientConf
    \retval SDB_OK open cache strategy Success
    \retval Others Fail
*/
SDB_EXPORT INT32 initClient( sdbClientConf* config ) ;

/** \fn INT32 sdbConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                           const CHAR *pUsrName, const CHAR *pPasswd ,
                           sdbConnectionHandle *handle ) ;
    \brief Connect to database
    \param [in] pHostName The Host Name or IP Address of Database Server
    \param [in] pServiceName The Service Name or Port of Database Server
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \param [out] handle The database connection handle,
                      when fail to connect, *handle == SDB_INVALID_HANDLE and error code
                      is return
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                              const CHAR *pUsrName, const CHAR *pPasswd ,
                              sdbConnectionHandle *handle ) ;

/** \fn INT32 sdbConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                            const CHAR *pUsrName, const CHAR *pPasswd ,
                            sdbConnectionHandle *handle )
    \brief Connect to database used a random valid address in the array.
    \param [in] pConnAddrs The array of the coord's address
    \param [in] arrSize The size of the array
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \param [out] handle The database connection handle
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                               const CHAR *pUsrName, const CHAR *pPasswd ,
                               sdbConnectionHandle *handle ) ;

/** \fn INT32 sdbSecureConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                           const CHAR *pUsrName, const CHAR *pPasswd ,
                           sdbConnectionHandle *handle ) ;
    \brief Connect to database with SSL
    \param [in] pHostName The Host Name or IP Address of Database Server
    \param [in] pServiceName The Service Name or Port of Database Server
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \param [out] handle The database connection handle,
                      when fail to connect, *handle == SDB_INVALID_HANDLE and error code
                      is return
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbSecureConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                              const CHAR *pUsrName, const CHAR *pPasswd ,
                              sdbConnectionHandle *handle ) ;

/** \fn INT32 sdbSecureConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                            const CHAR *pUsrName, const CHAR *pPasswd ,
                            sdbConnectionHandle *handle )
    \brief Connect to database used a random valid address in the array, with SSL.
    \param [in] pConnAddrs The array of the coord's address
    \param [in] arrSize The size of the array
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \param [out] handle The database connection handle
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbSecureConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                               const CHAR *pUsrName, const CHAR *pPasswd ,
                               sdbConnectionHandle *handle ) ;

/** \fn void sdbDisconnect ( sdbConnectionHandle handle )
    \brief Disconnect to database
    \param [in] handle The database connection handle
*/
SDB_EXPORT void sdbDisconnect ( sdbConnectionHandle handle ) ;

/** \fn void sdbGetLastErrorObj ( bson *obj )
    \brief Get the error object info for the last operation in the thread
    \param [out] obj The return error bson object
    \retval SDB_OK Get error object Success
    \retval SDB_DMS_EOC There is no error object
    \retval Others Get error object Fail
*/
SDB_EXPORT INT32 sdbGetLastErrorObj( bson *obj ) ;

/** \fn void sdbCleanLastErrorObj ()
    \brief Clean the last error object info in the thread
*/
SDB_EXPORT void sdbCleanLastErrorObj() ;

/** \fn INT32 sdbCreateUsr( sdbConnectionHandle cHandle, const CHAR *pUsrName,
                            const CHAR *pPasswd ) ;
    \brief Create an account
    \param [in] cHandle The database connection handle
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbCreateUsr( sdbConnectionHandle cHandle, const CHAR *pUsrName,
                               const CHAR *pPasswd ) ;

/** \fn INT32 sdbRemoveUsr( sdbConnectionHandle cHandle, const CHAR *pUsrName,
                            const CHAR *pPasswd ) ;
    \brief Delete an account
    \param [in] cHandle The database connection handle
    \param [in] pUsrName The User's Name of the account
    \param [in] pPasswd The Password  of the account
    \retval SDB_OK Connection Success
    \retval Others Connection Fail
*/
SDB_EXPORT INT32 sdbRemoveUsr( sdbConnectionHandle cHandle, const CHAR *pUsrName,
                               const CHAR *pPasswd ) ;

/* \fn INT32 sdbModifyConfig ( sdbConnectionHandle cHandle,
                               bson *config )
    \brief Modify config for the current node
    \param [in] cHandle The connection handle
    \param [in] config The new configurations
    \retval SDB_OK Modify Success
    \retval Others Modify Fail

SDB_EXPORT INT32 sdbModifyConfig ( sdbConnectionHandle cHandle,
                                   bson *config ) ;*/

/* \fn INT32 sdbModifyNodeConfig ( sdbConnectionHandle cHandle,
                                   INT32 nodeID,
                                   bson *config )
    \brief Modify config for a given node
    \param [in] cHandle The connection handle
    \param [in] nodeID The node id that want to be modified
    \param [in] config The new configurations
    \retval SDB_OK Modify Success
    \retval Others Modify Fail

SDB_EXPORT INT32 sdbModifyNodeConfig ( sdbConnectionHandle cHandle,
                                       INT32 nodeID,
                                       bson *config ) ;*/

/** \fn INT32 sdbGetDataBlocks ( sdbCollectionHandle cHandle,
                                bson *condition,
                                bson *select,
                                bson *orderBy,
                                bson *hint,
                                INT64 numToSkip,
                                INT64 numToReturn,
                                sdbCursorHandle *handle )
    \brief Get the data blocks' infomation for concurrent query
    \param [in] cHandle The connection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint The hint, The collection full name
                    eg: {"Collection":"foo.bar"}
                    while "foo.bar" is the full name of current collection
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \deprecated this API only support in java
*/
SDB_EXPORT INT32 sdbGetDataBlocks ( sdbCollectionHandle cHandle,
                                    bson *condition,
                                    bson *select,
                                    bson *orderBy,
                                    bson *hint,
                                    INT64 numToSkip,
                                    INT64 numToReturn,
                                    sdbCursorHandle *handle );

/** \fn INT32 sdbGetQueryMeta ( sdbCollectionHandle cHandle,
                               bson *condition,
                               bson *orderBy,
                               bson *hint,
                               INT64 numToSkip,
                               INT64 numToReturn,
                               sdbCursorHandle *handle )
    \brief Get the index blocks' or data blocks' infomations for concurrent query
    \param [in] condition The matching rule, return all the documents if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [out] handle The handle of query result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \deprecated this API only support in java
*/
SDB_EXPORT INT32 sdbGetQueryMeta ( sdbCollectionHandle cHandle,
                                   bson *condition,
                                   bson *orderBy,
                                   bson *hint,
                                   INT64 numToSkip,
                                   INT64 numToReturn,
                                   sdbCursorHandle *handle ) ;

/** \fn INT32 sdbGetSnapshot ( sdbConnectionHandle cHandle,
                               INT32 snapType,
                               bson *condition,
                               bson *selector,
                               bson *orderBy,
                               sdbCursorHandle *handle )
    \brief Get the snapshot
    \param [in] cHandle The connection handle
    \param [in] snapType The snapshot type as below

        SDB_SNAP_CONTEXTS         : Get the snapshot of all the contexts
        SDB_SNAP_CONTEXTS_CURRENT : Get the snapshot of current context
        SDB_SNAP_SESSIONS         : Get the snapshot of all the sessions
        SDB_SNAP_SESSIONS_CURRENT : Get the snapshot of current session
        SDB_SNAP_COLLECTIONS      : Get the snapshot of all the collections
        SDB_SNAP_COLLECTIONSPACES : Get the snapshot of all the collection spaces
        SDB_SNAP_DATABASE         : Get the snapshot of the database
        SDB_SNAP_SYSTEM           : Get the snapshot of the system
        SDB_SNAP_CATA             : Get the snapshot of the catalog
        SDB_SNAP_TRANSACTIONS     : Get snapshot of transactions in current session
        SDB_SNAP_TRANSACTIONS_CURRENT : Get snapshot of all the transactions
        SDB_SNAP_ACCESSPLANS      : Get the snapshot of cached access plans
        SDB_SNAP_HEALTH           : Get snapshot of node health detection
        SDB_SNAP_CONFIG           : Get snapshot of node configuration

    \param [in] condition The matching rule, match all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetSnapshot ( sdbConnectionHandle cHandle,
                                  INT32 snapType,
                                  bson *condition,
                                  bson *selector,
                                  bson *orderBy,
                                  sdbCursorHandle *handle ) ;

/** \fn INT32 sdbResetSnapshot ( sdbConnectionHandle cHandle,
 *                               bson *options )
    \brief Reset the snapshot
    \param [in] cHandle The connection handle
    \param [in] options The control options:

        Type            : (String) Specify the snapshot type to be reset.( defalut is "all" )
                          "sessions"
                          "sessions current"
                          "database"
                          "health"
                          "all"
        SessionID       : (INT32) Specify the session ID to be reset.
        Other options   : Some of other options are as below: (please visit the official website to
                          search "Location Elements" for more detail.)
                          GroupID   :INT32,
                          GroupName :String,
                          NodeID    :INT32,
                          HostName  :String,
                          svcname   :String,
                          ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbResetSnapshot ( sdbConnectionHandle cHandle,
                                    bson *options ) ;

/** \fn INT32 sdbTraceStart ( sdbConnectionHandle cHandle,
                              UINT32 traceBufferSize,
                              CHAR *component,
                              CHAR *breakpoint )
    \brief Start trace with given trace buffer size and component list
    \param [in] cHandle The connection handle
    \param [in] traceBufferSize The size for trace buffer on bytes
    \param [in] component The trace component list as below, NULL for all components, separated by comma (,)

        auth   : Authentication
        bps    : BufferPool Services
        cat    : Catalog Services
        cls    : Cluster Services
        dps    : Data Protection Services
        mig    : Migration Services
        msg    : Messaging Services
        net    : Network Services
        oss    : Operating System Services
        pd     : Problem Determination
        rtn    : RunTime
        sql    : SQL Parser
        tools  : Tools
        bar    : Backup And Recovery
        client : Client
        coord  : Coord Services
        dms    : Data Management Services
        ixm    : Index Management Services
        mon    : Monitoring Services
        mth    : Methods Services
        opt    : Optimizer
        pmd    : Process Model
        rest   : RESTful Services
        spt    : Scripting
        util   : Utilities
    \param [in] breakpoint The functions need to break, separated by comma (,)
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTraceStart ( sdbConnectionHandle cHandle,
                                 UINT32 traceBufferSize,
                                 CHAR * component,
                                 CHAR * breakPoint ,
                                 UINT32 *tids,
                                 UINT32 nTids ) ;
/** \fn INT32 sdbTraceResume ( sdbConnectionHandle cHandle )
    \brief Resume trace
    \param [in] cHandle The connection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTraceResume ( sdbConnectionHandle cHandle ) ;

/** \fn INT32 sdbTraceStop ( sdbConnectionHandle cHandle,
                             const CHAR *pDumpFileName )
    \brief Stop trace and dump into file
    \param [in] cHandle The connection handle
    \param [in] pDumpFileName The file to dump, NULL for stop only
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTraceStop ( sdbConnectionHandle cHandle,
                                const CHAR *pDumpFileName ) ;

/** \fn INT32 sdbTraceStatus ( sdbConnectionHandle cHandle,
                               sdbCursorHandle *handle )
    \brief Get the current status for trace
    \param [in] cHandle The connection handle
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTraceStatus ( sdbConnectionHandle cHandle,
                                  sdbCursorHandle *handle ) ;

/** \fn INT32 sdbGetList ( sdbConnectionHandle cHandle,
                           INT32 listType,
                           bson *condition,
                           bson *selector,
                           bson *orderBy,
                           sdbCursorHandle *handle )
    \brief Get the specified list
    \param [in] cHandle The collection handle
    \param [in] listType The list type as below

        SDB_LIST_CONTEXTS         : Get all contexts list
        SDB_LIST_CONTEXTS_CURRENT : Get contexts list for the current session
        SDB_LIST_SESSIONS         : Get all sessions list
        SDB_LIST_SESSIONS_CURRENT : Get the current session
        SDB_LIST_COLLECTIONS      : Get all collections list
        SDB_LIST_COLLECTIONSPACES : Get all collecion spaces' list
        SDB_LIST_STORAGEUNITS     : Get storage units list
        SDB_LIST_GROUPS           : Get replicaGroup list ( only applicable in sharding env )
        SDB_LIST_STOREPROCEDURES  : Get all the stored procedure list
        SDB_LIST_DOMAINS          : Get all the domains list
        SDB_LIST_TASKS            : Get all the running split tasks ( only applicable in sharding env )
        SDB_LIST_TRANSACTIONS     : Get all the transactions information.
        SDB_LIST_TRANSACTIONS_CURRENT : Get the transactions information of current session.

    \param [in] condition The matching rule, match all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetList ( sdbConnectionHandle cHandle,
                              INT32 listType,
                              bson *condition,
                              bson *selector,
                              bson *orderBy,
                              sdbCursorHandle *handle ) ;

/** \fn INT32 sdbGetCollection ( sdbConnectionHandle cHandle,
                                 const CHAR *pCollectionFullName,
                                 sdbCollectionHandle *handle )
    \brief Get the specified collection
    \param [in] cHandle The database connection handle
    \param [in] pCollectionFullName The full name of collection
    \param [out] handle The collection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCollection ( sdbConnectionHandle cHandle,
                                    const CHAR *pCollectionFullName,
                                    sdbCollectionHandle *handle ) ;

/** \fn INT32 sdbGetCollectionSpace ( sdbConnectionHandle cHandle,
                                      const CHAR *pCollectionSpaceName,
                                      sdbCSHandle *handle )
    \brief Get the specified collection space
    \param [in] cHandle The database connection handle
    \param [in] pCollectionSpaceName The name of collection space
    \param [out] handle The collection space handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCollectionSpace ( sdbConnectionHandle cHandle,
                                         const CHAR *pCollectionSpaceName,
                                         sdbCSHandle *handle ) ;

/** \fn INT32 sdbGetReplicaGroup ( sdbConnectionHandle cHandle,
                                   const CHAR *pRGName,
                                   sdbReplicaGroupHandle *handle )
    \brief Get the specified replica group
    \param [in] cHandle The database connection handle
    \param [in] pRGName The name of replica group
    \param [out] handle The replica group handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetReplicaGroup ( sdbConnectionHandle cHandle,
                                      const CHAR *pRGName,
                                      sdbReplicaGroupHandle *handle ) ;

/** \fn INT32 sdbGetReplicaGroup1 ( sdbConnectionHandle cHandle,
                                    UINT32 id,
                                    sdbReplicaGroupHandle *handle )
    \brief Get the specified replica group
    \param [in] cHandle The database connection handle
    \param [in] id The id of the replica group
    \param [out] handle The replica group handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetReplicaGroup1 ( sdbConnectionHandle cHandle,
                                       UINT32 id,
                                       sdbReplicaGroupHandle *handle ) ;

/** \fn INT32 sdbGetReplicaGroupName ( sdbReplicaGroupHandle cHandle,
                                       CHAR **ppRGName )
    \brief Get the specified replica group's name
    \param [in] cHandle The replica group handle
    \param [out] ppRGName The replica group name
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \deprecated This api will be deprecated at version 2.x
*/
SDB_EXPORT INT32 sdbGetReplicaGroupName ( sdbReplicaGroupHandle cHandle,
                                          CHAR **ppRGName ) ;

/** \fn INT32 sdbGetRGName ( sdbReplicaGroupHandle cHandle,
                             CHAR *pBuffer, INT32 size )
    \brief Get the specified replica group's name
    \param [in] cHandle The replica group handle
    \param [in] pBuffer The buffer for output replica group name
    \param [in] size the size of the buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetRGName ( sdbReplicaGroupHandle cHandle,
                                CHAR *pBuffer, INT32 size ) ;

/** \fn BOOLEAN sdbIsReplicaGroupCatalog ( sdbReplicaGroupHandle cHandle )
    \brief Test whether the specified replica group is catalog
    \param [in] cHandle The replica group handle
    \retval TRUE The replica group is catalog
    \retval FALSE The replica group is not catalog
*/
SDB_EXPORT BOOLEAN sdbIsReplicaGroupCatalog ( sdbReplicaGroupHandle cHandle ) ;

/** \fn INT32 sdbCreateCollectionSpace ( sdbConnectionHandle cHandle,
                                         const CHAR *pCollectionSpaceName,
                                         INT32 iPageSize,
                                         sdbCSHandle *handle )
    \brief Create the specified collection space
    \param [in] cHandle The database connection handle
    \param [in] pCollectionSpaceName The name of collection space
    \param [in] iPageSize The Page Size as below

        SDB_PAGESIZE_4K
        SDB_PAGESIZE_8K
        SDB_PAGESIZE_16K
        SDB_PAGESIZE_32K
        SDB_PAGESIZE_64K
        SDB_PAGESIZE_DEFAULT
    \param [out] handle The collection space handle
                                when fail to create collection space,
                                *handle == -1 and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateCollectionSpace ( sdbConnectionHandle cHandle,
                                            const CHAR *pCollectionSpaceName,
                                            INT32 iPageSize,
                                            sdbCSHandle *handle ) ;

/** \fn INT32 sdbCreateCollectionSpaceV2 ( sdbConnectionHandle cHandle,
                                           const CHAR *pCollectionSpaceName,
                                           bson *options,
                                           sdbCSHandle *handle )
    \brief Create the specified collection space
    \param [in] cHandle The database connection handle
    \param [in] pCollectionSpaceName The name of collection space
    \param [in] options The options specified by user, e.g. {"PageSize": 4096, "Domain": "mydomain"}.

        PageSize   : Assign the pagesize of the collection space
        Domain     : Assign which domain does current collection space belong to
    \param [out] handle The collection space handle
                                when fail to create collection space,
                                *handle == -1 and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateCollectionSpaceV2 ( sdbConnectionHandle cHandle,
                                              const CHAR *pCollectionSpaceName,
                                              bson *options,
                                              sdbCSHandle *handle ) ;

/** \fn INT32 sdbDropCollectionSpace ( sdbConnectionHandle cHandle,
                                       const CHAR *pCollectionSpaceName )
    \brief Drop the specified collection space
    \param [in] cHandle The database connection handle
    \param [in] pCollectionSpaceName The name of collection space
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDropCollectionSpace ( sdbConnectionHandle cHandle,
                                          const CHAR *pCollectionSpaceName ) ;

/** \fn INT32 sdbCreateReplicaGroup ( sdbConnectionHandle cHandle,
                                      const CHAR *pRGName,
                                      sdbReplicaGroupHandle *handle )
    \brief Create the specified replica group
    \param [in] cHandle The database connection handle
    \param [in] pRGName The name of the replica group
    \param [out] handle The replica group handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateReplicaGroup ( sdbConnectionHandle cHandle,
                                         const CHAR *pRGName,
                                         sdbReplicaGroupHandle *handle ) ;

/** \fn INT32 sdbRemoveReplicaGroup ( sdbConnectionHandle cHandle,
                                      const CHAR *pRGName )
    \brief Remove the specified replica group
    \param [in] cHandle The database connection handle
    \param [in] pRGName The name of the replica group
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRemoveReplicaGroup ( sdbConnectionHandle cHandle,
                                         const CHAR *pRGName ) ;

/** \fn INT32 sdbStartReplicaGroup ( sdbReplicaGroupHandle cHandle )
    \brief Start and activate the specified replica group
    \param [in] cHandle The replica group handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbStartReplicaGroup ( sdbReplicaGroupHandle cHandle ) ;

/** \fn INT32 sdbGetNodeMaster ( sdbReplicaGroupHandle cHandle,
                                 sdbNodeHandle *handle )
    \brief Get the master node of the specified replica group
    \param [in] cHandle The replica group handle
    \param [out] handle The master node handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeMaster ( sdbReplicaGroupHandle cHandle,
                                    sdbNodeHandle *handle ) ;

/** \fn INT32 sdbGetNodeSlave ( sdbReplicaGroupHandle cHandle,
                                sdbNodeHandle *handle )
    \brief Get one of slave node of the specified replica group,
           if no slave exists then get master
    \param [in] cHandle The replica group handle
    \param [out] handle The slave node handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeSlave ( sdbReplicaGroupHandle cHandle,
                                   sdbNodeHandle *handle ) ;

/** \fn INT32 sdbGetNodeSlave1 ( sdbReplicaGroupHandle cHandle,
                                 const INT32 *positionsArray,
                                 INT32 positionsCount,
                                 sdbNodeHandle *handle )
    \brief Get one of slave node of the specified replica group,
           if no slave exists then get master
    \param [in] cHandle The replica group handle
    \param [in] positionsArray The array of node's position, the array elements
                can be 1-7.
    \param [in] positionsCount The amount of node's position.
    \param [out] handle The slave node handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeSlave1 ( sdbReplicaGroupHandle cHandle,
                                    const INT32 *positionsArray,
                                    INT32 positionsCount,
                                    sdbNodeHandle *handle ) ;


/** \fn INT32 sdbGetNodeByName ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pNodeName,
                                 sdbNodeHandle *handle )
    \brief Get the node from the specified replica group
    \param [in] cHandle The replica group handle
    \param [in] pNodeName The name of the node, with the format of "hostname:port".
    \param [out] handle The node handle, when fail to get node,
                      *handle == -1, and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeByName ( sdbReplicaGroupHandle cHandle,
                                    const CHAR *pNodeName,
                                    sdbNodeHandle *handle ) ;

/** \fn INT32 sdbGetNodeByHost ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pHostName,
                                 const CHAR *pServiceName,
                                 sdbNodeHandle *handle )
    \brief Get the node from the specified replica group
    \param [in] cHandle The replica group handle
    \param [in] pHostName The host of node
    \param [in] pServiceName The service name of the node
    \param [out] handle The node handle, when fail to get node,
                      *handle == -1, and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeByHost ( sdbReplicaGroupHandle cHandle,
                                    const CHAR *pHostName,
                                    const CHAR *pServiceName,
                                    sdbNodeHandle *handle ) ;

/** \fn INT32 sdbGetNodeAddr ( sdbNodeHandle cHandle,
                               const CHAR **ppHostName,
                               const CHAR **ppServiceName,
                               const CHAR **ppNodeName,
                               INT32 *pNodeID )
    \brief Get the host and service name for the specified node
    \param [in] cHandle The node handle
    \param [out] ppHostName The hostname for the node
    \param [out] ppServiceName The servicename for the node
    \param [out] ppNodeName The name for the node
    \param [out] pNodeID The id for the node
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetNodeAddr ( sdbNodeHandle cHandle,
                                  const CHAR **ppHostName,
                                  const CHAR **ppServiceName,
                                  const CHAR **ppNodeName,
                                  INT32 *pNodeID ) ;

/** \fn INT32 sdbStartNode ( sdbNodeHandle cHandle )
    \brief Start up the specified node
    \param [in] cHandle The node handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbStartNode ( sdbNodeHandle cHandle ) ;

/** \fn INT32 sdbStopNode ( sdbNodeHandle cHandle )
    \brief Stop the specified node
    \param [in] cHandle The node handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbStopNode ( sdbNodeHandle cHandle ) ;

/** \fn INT32 sdbStopReplicaGroup ( sdbReplicaGroupHandle cHandle )
    \brief Stop the specified replica group
    \param [in] cHandle The replica group handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbStopReplicaGroup ( sdbReplicaGroupHandle cHandle ) ;

/** \fn INT32 sdbCreateReplicaCataGroup ( sdbReplicaGroupHandle cHandle,
                                          const CHAR *pHostName,
                                          const CHAR *pServiceName,
                                          const CHAR *pDatabasePath,
                                          bson *configure )
    \brief Create a catalog replica group
    \param [in] cHandle The database connection handle
    \param [in] pHostName The hostname for the catalog replica group
    \param [in] pServiceName The servicename for the catalog replica group
    \param [in] pDatabasePath The path for the catalog replica group
    \param [in] configure The configurations for the catalog replica group
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateReplicaCataGroup ( sdbConnectionHandle cHandle,
                                             const CHAR *pHostName,
                                             const CHAR *pServiceName,
                                             const CHAR *pDatabasePath,
                                             bson *configure );

/** \fn INT32 sdbCreateNode ( sdbReplicaGroupHandle cHandle,
                              const CHAR *pHostName,
                              const CHAR *pServiceName,
                              const CHAR *pDatabasePath,
                              bson *configure )
    \brief Create node in a given replica group
    \param [in] cHandle The replica group handle
    \param [in] pHostName The hostname for the node
    \param [in] pServiceName The servicename for the node
    \param [in] pDatabasePath The database path for the node
    \param [in] configure The configurations for the node
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateNode ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pHostName,
                                 const CHAR *pServiceName,
                                 const CHAR *pDatabasePath,
                                 bson *configure ) ;

/** \fn INT32 sdbRemoveNode ( sdbReplicaGroupHandle cHandle,
                              const CHAR *pHostName,
                              const CHAR *pServiceName,
                              bson *configure )
    \brief remove node in a given replica group
    \param [in] cHandle The replica group handle
    \param [in] pHostName The hostname for the node
    \param [in] pServiceName The servicename for the node
    \param [in] configure The configurations for the node
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRemoveNode ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pHostName,
                                 const CHAR *pServiceName,
                                 bson *configure ) ;

/** \fn INT32 sdbListCollectionSpaces ( sdbConnectionHandle cHandle,
                                        sdbCursorHandle *handle )
    \brief List all collection space of current database(include temporary collection space)
    \param [in] cHandle The database connection handle
    \param [out] handle The cursor handle of all collection space names
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListCollectionSpaces ( sdbConnectionHandle cHandle,
                                           sdbCursorHandle *handle ) ;

/** \fn INT32 sdbListCollections ( sdbConnectionHandle cHandle,
                                   sdbCursorHandle *handle )
    \brief List all collection of current database(not include temporary collection of temporary collection space)
    \param [in] cHandle The database connection handle
    \param [out] handle The cursor handle of all collection names
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListCollections ( sdbConnectionHandle cHandle,
                                      sdbCursorHandle *handle ) ;

/** \fn INT32 sdbListReplicaGroups ( sdbConnectionHandle cHandle,
                                     sdbCursorHandle *handle )
    \brief List all the replica groups of current database
    \param [in] cHandle The database connection handle
    \param [out] handle The cursor handle of all replica groups
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListReplicaGroups ( sdbConnectionHandle cHandle,
                                        sdbCursorHandle *handle ) ;

/** \fn INT32 sdbGetCollection1 ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName,
                                     sdbCollectionHandle *handle )
    \brief Get the specified collection of current collection space
    \param [in] cHandle The collection space handle
    \param [in] pCollectionName The collection name
    \param [out] handle The collection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/

/** \fn INT32 sdbFlushConfigure( sdbConnectionHandle cHandle,
                                 bson *options )
    \brief flush the options to configure file.
    \param [in] cHandle The connection handle
    \param [in] options The configure infomation, pass {"Global":true} or {"Global":false}
                    In cluster environment, passing {"Global":true} will flush data's and catalog's configuration file,
                    while passing {"Global":false} will flush coord's configuration file.
                    In stand-alone environment, both them have the same behaviour.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbFlushConfigure( sdbConnectionHandle cHandle,
                                    bson *options ) ;

/** \fn INT32 sdbCrtJSProcedure( sdbConnectionHandle cHandle,
                                 const CHAR *code )
    \brief create a store procedure.
    \param [in] cHandle The collection space space handle
    \param [in] code The code of store procedures
    \retval SDB_OK Operation Success
    \retval Others  Operation Fail
*/
SDB_EXPORT INT32 sdbCrtJSProcedure( sdbConnectionHandle cHandle,
                                    const CHAR *code ) ;

/** \fn INT32 sdbRmProcedure( sdbConnectionHandle cHandle,
                              const CHAR *spName )
    \brief remove a store procedure.
    \param [in] cHandle The collection space space handle
    \param [in] spName The name of store procedure
    \retval SDB_OK Operation Success
    \retval Others  Operation Fail
*/
SDB_EXPORT INT32 sdbRmProcedure( sdbConnectionHandle cHandle,
                                 const CHAR *spName ) ;


/** \fn INT32 sdbListProcedures( sdbConnectionHandle cHandle,
                                 bson *condition,
                                 sdbCursorHandle *handle )
    \brief List store procedures.
    \param [in] cHandle The collection space space handle
    \param [in] condition The condition of list
    \param [out] handle The cursor handle
    \retval SDB_OK Operation Success
    \retval Others  Operation Fail
*/
SDB_EXPORT INT32 sdbListProcedures( sdbConnectionHandle cHandle,
                                    bson *condition,
                                    sdbCursorHandle *handle ) ;

/** \fn INT32 sdbEvalJS( sdbConnectionHandle cHandle,
                         const CHAR *code,
                         SDB_SPD_RES_TYPE *type,
                         sdbCursorHandle *handle,
                         bson *errmsg )
   \brief Eval a func.
   \      type is declared in spd.h. see SDB_FMP_RES_TYPE.
   \param [in] cHandle The collection space space handle
   \param [in] code The code to eval
   \param [out] type The type of value
   \param [out] handle The cursor handle of current eval
   \param [out] errmsg The errmsg from eval
   \retval SDB_OK Operation Success
   \retval Others  Operation Fail
*/

SDB_EXPORT INT32 sdbEvalJS( sdbConnectionHandle cHandle,
                            const CHAR *code,
                            SDB_SPD_RES_TYPE *type,
                            sdbCursorHandle *handle,
                            bson *errmsg ) ;

/** \fn INT32 sdbGetCollection1 ( sdbCSHandle cHandle,
                                  const CHAR *pCollectionName,
                                  sdbCollectionHandle *handle )
    \brief Get the specified collection
    \param [in] cHandle The database connection handle
    \param [in] pCollectionName The name of collection
    \param [out] handle The collection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCollection1 ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName,
                                     sdbCollectionHandle *handle ) ;

/** \fn INT32 sdbCreateCollection ( sdbCSHandle cHandle,
                                    const CHAR *pCollectionName,
                                    sdbCollectionHandle *handle )
    \brief Create the specified collection in current collection space
           This function creates a non-sharded collection with default replsize
    \param [in] cHandle The collection space handle
    \param [in] pCollectionName The collection name
    \param [out] handle The collection handle,
                      when fail to create collection,
                      *handle == -1 and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateCollection ( sdbCSHandle cHandle,
                                       const CHAR *pCollectionName,
                                       sdbCollectionHandle *handle ) ;

/** \fn INT32 sdbCreateCollection1 ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName,
                                     bson *options,
                                     sdbCollectionHandle *handle )
    \brief Create the specified collection in current collection space
    \param [in] cHandle The collection space handle
    \param [in] pCollectionName The collection name
    \param [in] options The options for creating collection,
                including "ShardingKey", "ReplSize", "IsMainCL" and "Compressed" informations,
                no options, if null
    \param [out] handle The collection handle
                      when fail to create collection,
                      *handle == -1 and error code is return
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateCollection1 ( sdbCSHandle cHandle,
                                        const CHAR *pCollectionName,
                                        bson *options,
                                        sdbCollectionHandle *handle ) ;

/** \fn INT32 sdbAlterCollection ( sdbCollectionHandle cHandle,
                                  bson *options  )
    \brief Alter the specified collection
    \param [in] cHandle The colleciton handle
    \param [in] options The options are as following:

        ReplSize     : Assign how many replica nodes need to be synchronized when a write request(insert, update, etc) is executed
        ShardingKey  : Assign the sharding key
        ShardingType : Assign the sharding type
        Partition    : When the ShardingType is "hash", need to assign Partition, it's the bucket number for hash, the range is [2^3,2^20]
                       e.g. {RepliSize:0, ShardingKey:{a:1}, ShardingType:"hash", Partition:1024}
    \note Can't alter attributes about split in partition collection; After altering a collection to
          be a partition collection, need to split this collection manually
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAlterCollection ( sdbCollectionHandle cHandle,
                                      bson *options  ) ;

/** \fn INT32 sdbDropCollection ( sdbCSHandle cHandle,
                                  const CHAR *pCollectionName )
    \brief Drop the specified collection in current collection space
    \param [in] cHandle The collection space handle
    \param [in] pCollectionName The collection name
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDropCollection ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName ) ;

/** \fn INT32 sdbGetCSName ( sdbCSHandle cHandle,
                             CHAR *pCSName, INT32 size )
    \brief Get the specified collection space name
    \param [in] cHandle The collection space handle
    \param [in] pBuffer The buffer for output cs name
    \param [in] size The size of the buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCSName ( sdbCSHandle cHandle,
                                CHAR *pBuffer, INT32 size ) ;

/** \fn INT32 sdbRenameCollection( sdbCSHandle cHandle,
                                   const CHAR *pOldName,
                                   const CHAR *pNewName,
                                   bson *options )
    \brief Rename collection name
    \param [in] cHandle The collection space handle
    \param [in] pOldName The old collection short name
    \param [in] pNewName The new collection short name
    \param [in] options Reserved
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRenameCollection( sdbCSHandle cHandle,
                                      const CHAR *pOldName,
                                      const CHAR *pNewName,
                                      bson *options ) ;

/** \fn INT32 sdbGetCLName ( sdbCollectionHandle cHandle,
                             CHAR *pCLName, INT32 size )
    \brief Get the specified collection name
    \param [in] cHandle The collection handle
    \param [in] pBuffer The buffer for output cl name
    \param [in] size The size of the buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCLName ( sdbCollectionHandle cHandle,
                                CHAR *pBuffer, INT32 size ) ;

/** \fn INT32 sdbGetCLFullName ( sdbCollectionHandle cHandle,
                                 CHAR *pBuffer, INT32 size )
    \brief Get the specified collection full name
    \param [in] cHandle The collection handle
    \param [in] pBuffer The buffer for output cl full name
    \param [in] size The size of the buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCLFullName ( sdbCollectionHandle cHandle,
                                    CHAR *pBuffer, INT32 size ) ;

/** \fn INT32 sdbSplitCollection ( sdbCollectionHandle cHandle,
                                   const CHAR *pSourceRG,
                                   const CHAR *pTargetRG,
                                   const bson *pSplitCondition,
                                   const bson *pSplitEndCondition )
    \brief Split the specified collection from source replica group to target by range
    \param [in] cHandle The collection handle
    \param [in] pSourceRG The source replica group name
    \param [in] pTargetRG The target replica group name
    \param [in] pSplitCondition The split condition
    \param [in] splitEndCondition The split end condition or null
              eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"Hash",Partition:2^10},
              we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
              the target replica group will get the records whose age's hash value are in [30,60). If splitEndCondition is null,
              they are in [30,max).
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSplitCollection ( sdbCollectionHandle cHandle,
                                      const CHAR *pSourceRG,
                                      const CHAR *pTargetRG,
                                      const bson *pSplitCondition,
                                      const bson *pSplitEndCondition ) ;

/** \fn INT32 sdbSplitCLAsync ( sdbCollectionHandle cHandle,
                                const CHAR *pSourceRG,
                                const CHAR *pTargetRG,
                                const bson *pSplitCondition,
                                const bson *pSplitEndCondition,
                                SINT64 *taskID )
    \brief Split the specified collection from source replica group to target by range
    \param [in] cHandle The collection handle
    \param [in] pSourceRG The source replica group name
    \param [in] pTargetRG The target replica group name
    \param [in] pSplitCondition The split condition
    \param [in] splitEndCondition The split end condition or null
              eg:If we create a collection with the option {ShardingKey:{"age":1},ShardingType:"Hash",Partition:2^10},
              we can fill {age:30} as the splitCondition, and fill {age:60} as the splitEndCondition. when split,
              the target replica group will get the records whose age's hash value are in [30,60). If splitEndCondition is null,
              they are in [30,max).
    \param [out] taskID The id of current task
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSplitCLAsync ( sdbCollectionHandle cHandle,
                                   const CHAR *pSourceRG,
                                   const CHAR *pTargetRG,
                                   const bson *pSplitCondition,
                                   const bson *pSplitEndCondition,
                                   SINT64 *taskID ) ;

/** \fn INT32 sdbSplitCollectionByPercent ( sdbCollectionHandle cHandle,
                                            const CHAR *pSourceRG,
                                            const CHAR *pTargetRG,
                                            FLOAT64 percent )
    \brief Split the specified collection from source replica group to target by percent
    \param [in] cHandle The collection handle
    \param [in] pSourceRG The source replica group name
    \param [in] pTargetRG The target replica group name
    \param [in] percent The split percent, Range:(0.0, 100.0]
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSplitCollectionByPercent ( sdbCollectionHandle cHandle,
                                               const CHAR *pSourceRG,
                                               const CHAR *pTargetRG,
                                               FLOAT64 percent ) ;

/** \fn INT32 sdbSplitCLByPercentAsync ( sdbCollectionHandle cHandle,
                                         const CHAR *pSourceRG,
                                         const CHAR *pTargetRG,
                                         FLOAT64 percent,
                                         SINT64 *taskID )
    \brief Split the specified collection from source replica group to target by percent
    \param [in] cHandle The collection handle
    \param [in] pSourceRG The source replica group name
    \param [in] pTargetRG The target replica group name
    \param [in] percent The split percent, Range:(0.0, 100.0]
    \param [out] taskID The id of current task
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSplitCLByPercentAsync ( sdbCollectionHandle cHandle,
                                            const CHAR *pSourceRG,
                                            const CHAR *pTargetRG,
                                            FLOAT64 percent,
                                            SINT64 *taskID ) ;

/** \fn INT32 sdbCreateIndex ( sdbCollectionHandle cHandle,
                               bson *indexDef,
                               const CHAR *pIndexName,
                               BOOLEAN isUnique,
                               BOOLEAN isEnforced )
    \brief Create the index in current collection
    \param [in] cHandle The collection handle
    \param [in] indexDef The bson structure of index element, e.g. {name:1, age:-1}
    \param [in] pIndexName The index name
    \param [in] isUnique Whether the index elements are unique or not
    \param [in] isEnforced Whether the index is enforced unique
                           This element is meaningful when isUnique is set to true
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateIndex ( sdbCollectionHandle cHandle,
                                  bson *indexDef,
                                  const CHAR *pIndexName,
                                  BOOLEAN isUnique,
                                  BOOLEAN isEnforced ) ;

/** \fn INT32 sdbCreateIndex1 ( sdbCollectionHandle cHandle,
                                bson *indexDef,
                                const CHAR *pIndexName,
                                BOOLEAN isUnique,
                                BOOLEAN isEnforced,
                                INT32 sortBufferSize )
    \brief Create the index in current collection
    \param [in] cHandle The collection handle
    \param [in] indexDef The bson structure of index element, e.g. {name:1, age:-1}
    \param [in] pIndexName The index name
    \param [in] isUnique Whether the index elements are unique or not
    \param [in] isEnforced Whether the index is enforced unique
                           This element is meaningful when isUnique is set to true
    \param [in] sortBufferSize The size of sort buffer used when creating index, the unit is MB,
                               zero means don't use sort buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateIndex1 ( sdbCollectionHandle cHandle,
                                   bson *indexDef,
                                   const CHAR *pIndexName,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforced,
                                   INT32 sortBufferSize ) ;

/** \fn INT32 sdbGetIndexes ( sdbCollectionHandle cHandle,
                              const CHAR *pIndexName,
                              sdbCursorHandle *handle )
    \brief Get all of or one of the indexes in current collection
    \param [in] cHandle The collection handle
    \param [in] pIndexName The index name, returns all of the indexes if this parameter is null
    \param [out] handle The cursor handle of returns
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetIndexes ( sdbCollectionHandle cHandle,
                                 const CHAR *pIndexName,
                                 sdbCursorHandle *handle ) ;

/** \fn INT32 sdbDropIndex ( sdbCollectionHandle cHandle,
                             const CHAR *pIndexName )
    \brief Drop the index in current collection
    \param [in] cHandle The collection handle
    \param [in] pIndexName The index name
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDropIndex ( sdbCollectionHandle cHandle,
                                const CHAR *pIndexName ) ;

/** \fn INT32 sdbGetCount ( sdbCollectionHandle cHandle,
                            bson *condition,
                            SINT64 *count )
    \brief Get the count of documents in specified collection
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return the count of all documents if this parameter is null
    \param [out] count The count of matching documents
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCount ( sdbCollectionHandle cHandle,
                               bson *condition,
                               SINT64 *count );


/** \fn INT32 sdbGetCount1 ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *hint,
                             SINT64 *count )
    \brief Get the count of documents in specified collection
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return the count of all documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [out] count The count of matching documents
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetCount1 ( sdbCollectionHandle cHandle,
                                bson *condition,
                                bson *hint,
                                SINT64 *count );

/** \fn INT32 sdbInsert ( sdbCollectionHandle cHandle,
                          bson *obj )
    \brief Insert a bson object into current collection
    \param [in] cHandle The collection handle
    \param [in] obj The inserted bson object, cannot be null
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbInsert ( sdbCollectionHandle cHandle,
                             bson *obj ) ;

/** \fn INT32 sdbInsert1 ( sdbCollectionHandle cHandle,
                           bson *obj, bson_iterator *id )
    \brief Insert a bson object into current collection
    \param [in] cHandle The collection handle
    \param [in] obj The inserted bson object, cannot be null
    \param [out] id The object id of inserted bson object in current collection
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbInsert1 ( sdbCollectionHandle cHandle,
                              bson *obj, bson_iterator *id ) ;

/** The flags represent whether bulk insert continue when hitting index key duplicate error */
#define FLG_INSERT_CONTONDUP  0x00000001

/** \fn INT32 sdbBulkInsert ( sdbCollectionHandle cHandle,
                              SINT32 flags, bson **obj, SINT32 num )
    \brief Insert a bulk of bson objects into current collection
    \param [in] cHandle The collection handle
    \param [in] flags FLG_INSERT_CONTONDUP or 0. While FLG_INSERT_CONTONDUP
                is set, if some records hit index key duplicate error,
                database will skip them and go on inserting. However, while 0
                is set, database will stop inserting in that case, and return
                errno code.
    \param [in] obj The array of inserted bson objects, cannot be null
    \param [in] num The number of inserted bson objects
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \code
      INT32 rc = 0 ;
      INT32 i = 0 ;
      const INT32 num = 10 ;
      bson* obj[num] ;
      for ( i = 0; i < num; i++ )
      {
         obj[i] = bson_create();
         rc = bson_append_int( obj[i], "num", i ) ;
         if ( rc != 0 )
            printf ( "something wrong.\n" ) ;
         rc = bson_finish ( obj[i] ) ;
         if ( rc != 0 )
            printf ( "something wrong.\n" ) ;
      }
      rc = sdbBulkInsert ( cl, 0, obj, num ) ;
      if ( rc )
         printf ( "something wrong, rc = %d.\n", rc ) ;
      for ( i = 0; i < num; i++ )
      {
         bson_dispose ( obj[i] ) ;
      }
    \endcode

*/
SDB_EXPORT INT32 sdbBulkInsert ( sdbCollectionHandle cHandle,
                                 SINT32 flags, bson **obj, SINT32 num ) ;

/** \fn INT32 sdbUpdate ( sdbCollectionHandle cHandle,
                          bson *rule,
                          bson *condition,
                          bson *hint )
    \brief Update the matching documents in current collection
    \param [in] cHandle The collection handle
    \param [in] rule The updating rule, cannot be null
    \param [in] condition The matching rule, update all the documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note It won't work to update the "ShardingKey" field, but the other fields take effect
*/
SDB_EXPORT INT32 sdbUpdate ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint ) ;

/** \fn INT32 sdbUpdate1 ( sdbCollectionHandle cHandle,
                          bson *rule,
                          bson *condition,
                          bson *hint,
                          INT32 flag )
    \brief Update the matching documents in current collection
    \param [in] cHandle The collection handle
    \param [in] rule The updating rule, cannot be null
    \param [in] condition The matching rule, update all the documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] flag The update flag, default to be 0. Please see the definition of follow flags for more detail.
    \code
        UPDATE_KEEP_SHARDINGKEY
    \endcode
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note When flag is set to 0, it won't work to update the "ShardingKey" field, but the
              other fields take effect
*/
SDB_EXPORT INT32 sdbUpdate1 ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint,
                             INT32 flag ) ;

/** \fn INT32 sdbUpsert ( sdbCollectionHandle cHandle,
                          bson *rule,
                          bson *condition,
                          bson *hint )
    \brief Update the matching documents in current collection, insert if no matching
    \param [in] cHandle The collection handle
    \param [in] rule The updating rule, cannot be null
    \param [in] condition The matching rule, update all the documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note It won't work to upsert the "ShardingKey" field, but the other fields take effect
*/
SDB_EXPORT INT32 sdbUpsert ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint ) ;

/** \fn INT32 sdbUpsert1 ( sdbCollectionHandle cHandle,
                           bson *rule,
                           bson *condition,
                           bson *hint,
                           bson *setOnInsert )
    \brief Update the matching documents in current collection, insert if no matching
    \param [in] cHandle The collection handle
    \param [in] rule The updating rule, cannot be null
    \param [in] condition The matching rule, update all the documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] setOnInsert The setOnInsert assigns the specified values to the fileds when insert
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note It won't work to upsert the "ShardingKey" field, but the other fields take effect
*/
SDB_EXPORT INT32 sdbUpsert1 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              bson *setOnInsert ) ;

/** \fn INT32 sdbUpsert2 ( sdbCollectionHandle cHandle,
                           bson *rule,
                           bson *condition,
                           bson *hint,
                           bson *setOnInsert,
                           INT32 flag )
    \brief Update the matching documents in current collection, insert if no matching
    \param [in] cHandle The collection handle
    \param [in] rule The updating rule, cannot be null
    \param [in] condition The matching rule, update all the documents if this parameter is null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] setOnInsert The setOnInsert assigns the specified values to the fileds when insert
    \param [in] flag The update flag, default to be 0. Please see the definition of follow flags for more detail.
    \code
        UPDATE_KEEP_SHARDINGKEY
    \endcode
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note When flag is set to 0, it won't work to update the "ShardingKey" field, but the
              other fields take effect
*/
SDB_EXPORT INT32 sdbUpsert2 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              bson *setOnInsert,
                              INT32 flag ) ;

/** \fn INT32 sdbDelete ( sdbCollectionHandle cHandle,
                          bson *condition,
                          bson *hint )
    \brief Delete the matching documents in current collection, never rollback if failed
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, delete all the documents if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDelete ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *hint ) ;

/** \fn INT32 sdbQuery1 ( sdbCollectionHandle cHandle,
                          bson *condition,
                          bson *select,
                          bson *orderBy,
                          bson *hint,
                          INT64 numToSkip,
                          INT64 numToReturn,
                          INT32 flag,
                          sdbCursorHandle *handle )
    \brief Get the matching documents in current collection
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [in] flag The query flag, default to be 0. Please see the definition of follow flags for more detail. Usage: e.g. set ( QUERY_FORCE_HINT | QUERY_WITH_RETURNDATA ) to param flag
    \code
        QUERY_FORCE_HINT
        QUERY_PARALLED
        QUERY_WITH_RETURNDATA
    \endcode
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbQuery1 ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *select,
                             bson *orderBy,
                             bson *hint,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             INT32 flags,
                             sdbCursorHandle *handle ) ;


/** \fn INT32 sdbQuery ( sdbCollectionHandle cHandle,
                         bson *condition,
                         bson *select,
                         bson *orderBy,
                         bson *hint,
                         INT64 numToSkip,
                         INT64 numToReturn,
                         sdbCursorHandle *handle )
    \brief Get the matching documents in current collection
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbQuery ( sdbCollectionHandle cHandle,
                            bson *condition,
                            bson *select,
                            bson *orderBy,
                            bson *hint,
                            INT64 numToSkip,
                            INT64 numToReturn,
                            sdbCursorHandle *handle ) ;

/** \fn INT32 sdbQueryAndUpdate ( sdbCollectionHandle cHandle,
                                  bson *condition,
                                  bson *select,
                                  bson *orderBy,
                                  bson *hint,
                                  bson *update,
                                  INT64 numToSkip,
                                  INT64 numToReturn,
                                  INT32 flag,
                                  BOOLEAN returnNew,
                                  sdbCursorHandle *handle )
    \brief Get the matching documents in current collection and update
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] update The update rule, can't be null
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [in] flag The query flag, default to be 0. Please see the definition of follow flags for more detail. Usage: e.g. set ( QUERY_FORCE_HINT | QUERY_WITH_RETURNDATA ) to param flag
    \code
        QUERY_FORCE_HINT
        QUERY_PARALLED
        QUERY_WITH_RETURNDATA
        QUERY_KEEP_SHARDINGKEY_IN_UPDATE
    \endcode
    \param [in] returnNew When TRUE, returns the updated document rather than the original
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbQueryAndUpdate ( sdbCollectionHandle cHandle,
                                     bson *condition,
                                     bson *select,
                                     bson *orderBy,
                                     bson *hint,
                                     bson *update,
                                     INT64 numToSkip,
                                     INT64 numToReturn,
                                     INT32 flag,
                                     BOOLEAN returnNew,
                                     sdbCursorHandle *handle ) ;

/** \fn INT32 sdbQueryAndRemove ( sdbCollectionHandle cHandle,
                                  bson *condition,
                                  bson *select,
                                  bson *orderBy,
                                  bson *hint,
                                  INT64 numToSkip,
                                  INT64 numToReturn,
                                  INT32 flag,
                                  sdbCursorHandle *handle )
    \brief Get the matching documents in current collection and remove
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [in] flag The query flag, default to be 0. Please see the definition of follow flags for more detail. Usage: e.g. set ( QUERY_FORCE_HINT | QUERY_WITH_RETURNDATA ) to param flag
    \code
        QUERY_FORCE_HINT
        QUERY_PARALLED
        QUERY_WITH_RETURNDATA
    \endcode
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbQueryAndRemove ( sdbCollectionHandle cHandle,
                                     bson *condition,
                                     bson *select,
                                     bson *orderBy,
                                     bson *hint,
                                     INT64 numToSkip,
                                     INT64 numToReturn,
                                     INT32 flag,
                                     sdbCursorHandle *handle ) ;

/** \fn INT32 sdbExplain ( sdbCollectionHandle cHandle,
                           bson *condition,
                           bson *select,
                           bson *orderBy,
                           bson *hint,
                           INT32 flag,
                           INT64 numToSkip,
                           INT64 numToReturn,
                           bson *options,
                           sdbCursorHandle *handle )
    \brief get access plan of query
    \param [in] cHandle The collection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] select The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [in] flag The query flag, default to be 0. Please see the definition of follow flags for more detail. Usage: e.g. set ( QUERY_FORCE_HINT | QUERY_WITH_RETURNDATA ) to param flag
    \code
        QUERY_FORCE_HINT
        QUERY_PARALLED
        QUERY_WITH_RETURNDATA
    \endcode
    \param [in] numToSkip Skip the first numToSkip documents, never skip if this parameter is 0
    \param [in] numToReturn Only return numToReturn documents, return all if this parameter is -1
    \param [in] options the rules of explain, the options are as below:

        Run     : Whether execute query explain or not, true for excuting query explain then get
                  the data and time information; false for not excuting query explain but get the
                  query explain information only. e.g. {Run:true}
    \param [out] handle The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbExplain ( sdbCollectionHandle cHandle,
                              bson *condition,
                              bson *select,
                              bson *orderBy,
                              bson *hint,
                              INT32 flag,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              bson *options,
                              sdbCursorHandle *handle ) ;

/** \fn INT32 sdbNext ( sdbCursorHandle cHandle,
                        bson *obj )
    \brief Return the next document of current cursor, and move forward
    \param [in] cHandle The cursor handle
    \param [out] obj The return bson object
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbNext ( sdbCursorHandle cHandle,
                           bson *obj ) ;

/** \fn INT32 sdbCurrent ( sdbCursorHandle cHandle,
                           bson *obj )
    \brief Return the current document of cursor, and don't move
    \param [in] cHandle The cursor handle
    \param [out] obj The return bson object
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCurrent ( sdbCursorHandle cHandle,
                              bson *obj ) ;

/* \fn INT32 sdbUpdateCurrent ( sdbCursorHandle cHandle, bson *rule )
    \brief Update the current document of cursor
    \param [in] cHandle The cursor handle
    \param [in] rule The updating rule, cannot be null
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
/*
SDB_EXPORT INT32 sdbUpdateCurrent ( sdbCursorHandle cHandle,
                                    bson *rule ) ;
*/
/* \fn INT32 sdbDeleteCurrent ( sdbCursorHandle cHandle )
    \brief Delete the current document of cursor
    \param [in] cHandle The cursor handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
/*
SDB_EXPORT INT32 sdbDeleteCurrent ( sdbCursorHandle cHandle ) ;
*/

/** \fn INT32 sdbCloseCursor( sdbCursorHandle cHandle )
    \brief Close the cursor's connection to database, we can't use this handle to get
                 data again.
    \param [in] cHandle The cursor handle
    \note Don't call this method after the connection handle had been released.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCloseCursor ( sdbCursorHandle cHandle ) ;

/** \fn INT32 sdbCloseAllCursors( sdbConnectionHandle cHandle )
    \brief Close all the cursors in current thread, we can't use those cursors to get
           data anymore.
    \param [in] cHandle The database connection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCloseAllCursors ( sdbConnectionHandle cHandle ) ;


/** \fn INT32 sdbExec( sdbConnectionHandle cHandle,
                       const CHAR *sql,
                       sdbCursorHandle *result )
    \brief Executing SQL command.
    \param [in] cHandle The database connection handle
    \param [in] sql The SQL command.
    \param [out] result The return cursor handle of matching documents.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbExec( sdbConnectionHandle cHandle,
                          const CHAR *sql,
                          sdbCursorHandle *result );

/** \fn INT32 sdbExecUpdate( sdbConnectionHandle cHandle,
                                const CHAR *sql )
    \brief Executing SQL command for updating.
    \param [in] cHandle The database connection handle
    \param [in] sql The SQL command.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbExecUpdate( sdbConnectionHandle cHandle,
                                const CHAR *sql ) ;

/** \fn INT32 sdbTransactionBegin( sdbConnectionHandle cHandle )
    \brief Transaction begin.
    \param [in] cHandle The database connection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTransactionBegin( sdbConnectionHandle cHandle ) ;

/** \fn INT32 sdbTransactionCommit( sdbConnectionHandle cHandle )
    \brief Transaction commit.
    \param [in] cHandle The database connection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTransactionCommit( sdbConnectionHandle cHandle ) ;

/** \fn INT32 sdbTransactionRollback( sdbConnectionHandle cHandle )
    \brief Transaction rollback.
    \param [in] cHandle The database connection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTransactionRollback( sdbConnectionHandle cHandle ) ;

/** \fn void sdbReleaseConnection ( sdbConnectionHandle cHandle )
    \brief Release the database connection handle
    \param [in] cHandle The database connection handle
*/
SDB_EXPORT void sdbReleaseConnection ( sdbConnectionHandle cHandle ) ;

/** \fn void sdbReleaseCollection ( sdbCollectionHandle cHandle )
    \brief Release collection handle, the cursor handle of this collection will still available
    \param [in] cHandle The collection handle
*/
SDB_EXPORT void sdbReleaseCollection ( sdbCollectionHandle cHandle ) ;

/** \fn void sdbReleaseCS ( sdbCSHandle cHandle )
    \brief Release the collection space handle, the collecion and cursor handle of this collection space will still available
    \param [in] cHandle The database connection handle
*/
SDB_EXPORT void sdbReleaseCS ( sdbCSHandle cHandle ) ;

/** \fn void sdbReleaseCursor ( sdbCursorHandle cHandle )
    \brief Release the cursor handle
    \param [in] cHandle The cursor handle
*/
SDB_EXPORT void sdbReleaseCursor ( sdbCursorHandle cHandle ) ;

/** \fn void sdbReleaseReplicaGroup ( sdbReplicaGroupHandle cHandle )
    \brief Release the replica group handle
    \param [in] cHandle The replica group handle
*/
SDB_EXPORT void sdbReleaseReplicaGroup ( sdbReplicaGroupHandle cHandle ) ;

/** \fn void sdbReleaseNode ( sdbNodeHandle cHandle )
    \brief Release the node handle
    \param [in] cHandle The node handle
*/
SDB_EXPORT void sdbReleaseNode ( sdbNodeHandle cHandle ) ;

/** \fn void sdbReleaseDomain ( sdbDomainHandle cHandle )
    \brief Release the domain handle
    \param [in] cHandle the domain handle
*/
SDB_EXPORT void sdbReleaseDomain ( sdbDomainHandle cHandle ) ;

/* \fn void sdbReleaseDC ( sdbDCHandle cHandle )
    \brief Release the data center handle
    \param [in] cHandle the data center handle
*/
SDB_EXPORT void sdbReleaseDC ( sdbDCHandle cHandle ) ;

/** \fn INT32 sdbAggregate ( sdbCollectionHandle cHandle,
                             bson **obj, SINT32 num,
                             sdbCursorHandle *handle )
    \brief Execute aggregate operation in specified collection
    \param [in] cHandle The collection handle
    \param [in] obj The array of bson objects
    \param [in] num The number of inserted bson objects
    \param [out] handle The cursor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \code
        INT32 rc = 0 ;
        INT32 i = 0 ;
        sdbCollectionHandle cl ;
        sdbCursorHandle cursor ;
        const INT32 num = 2 ;
        bson* obj[num] ;
        const CHAR* pArr[num] = {
          "{ $match: { $and: [ { no: { $gt: 1002 } },{ no: { $lt: 1015 } },{ dep
  : \"IT Academy\" } ] } }",
          "{ $project: { no: 1, \"info.name\": 1, major: 1 } }"
        } ;

        for ( i = 0; i < num; i++ )
        {
           obj[i] = bson_create();
           jsonToBson ( obj[i], pArr[i] ) ;
        }
        if ( rc )
           printf ( "something wrong, rc = %d.\n", rc ) ;
        for ( i = 0; i < num; i++ )
        {
           bson_print( obj[i] ) ;
           bson_dispose ( obj[i] ) ;
        }
   \endcode
*/
SDB_EXPORT INT32 sdbAggregate ( sdbCollectionHandle cHandle,
                                bson **obj, SINT32 num,
                                sdbCursorHandle *handle ) ;

/** \fn INT32 sdbAttachCollection ( sdbCollectionHandle cHandle,
                                    const CHAR *subClFullName,
                                    bson *options)
    \brief Attach the specified collection
    \param [in] cHandle The collection handle
    \param [in] subClFullName The name of the subcollection
    \param [in] options The low boudary and up boudary
                eg: {"LowBound":{a:1},"UpBound":{a:100}}
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAttachCollection ( sdbCollectionHandle cHandle,
                                       const CHAR *subClFullName,
                                       bson *options) ;

/** \fn INT32 sdbDetachCollection ( sdbCollectionHandle cHandle,
                                    const CHAR *subClFullName)
    \brief Detach the specified collection.
    \param [in] cHandle The collection handle
    \param [in] subClFullName The name of the subcollection
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDetachCollection ( sdbCollectionHandle cHandle,
                                       const CHAR *subClFullName) ;

/** \fn INT32 sdbBackupOffline ( sdbConnectionHandle cHandle,
                                 bson *options)
    \brief Backup the whole database or specifed replica group.
    \param [in] cHandle The connection handle
    \param [in] options Contains a series of backup configuration infomations. Backup the whole cluster if null. The "options" contains 5 options as below. All the elements in options are optional. eg: {"GroupName":["RGName1", "RGName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName", "Description":description, "EnsureInc":true, "OverWrite":true}

        GroupID     : The id(s) of replica group(s) which to be backuped
        GroupName   : The replica groups which to be backuped
        Path        : The backup path, if not assign, use the backup path assigned in the configuration file,
                      the path support to use wildcard(%g/%G:group name, %h/%H:host name, %s/%S:service name). e.g.  {Path:"/opt/sequoiadb/backup/%g"}
        isSubDir    : Whether the path specified by paramer "Path" is a subdirectory of the path specified in the configuration file, default to be false
        Name        : The name for the backup
        Prefix      : The prefix of name for the backup, default to be null. e.g. {Prefix:"%g_bk_"}
        EnableDateDir : Whether turn on the feature which will create subdirectory named to current date like "YYYY-MM-DD" automatically, default to be false
        Description : The description for the backup
        EnsureInc   : Whether excute increment synchronization, default to be false
        OverWrite   : Whether overwrite the old backup file, default to be false
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbBackupOffline ( sdbConnectionHandle cHandle,
                                    bson *options) ;

/** \fn INT32 sdbListBackup ( sdbConnectionHandle cHandle,
                              bson *options,
                              bson *condition,
                              bson *selector,
                              bson *orderBy,
                              sdbCursorHandle *handle );
    \brief List the backups.
    \param [in] cHandle The connection handle
    \param [in] options Contains configuration information for listing backups, list all the backups in the default backup path if null. The "options" contains several options as below. All the elements in options are optional. eg: {"GroupName":["RGName1", "RGName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}

        GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
        GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
        Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
        Name        : Specified the name of backup, default to list all the backups.
        IsSubDir    : Specified the "Path" is a subdirectory of the backup path assigned in the configuration file or not, default to be false.
        Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
        Detail      : Display the detail of the backups or not, default to be false.
    \param [in] condition The matching rule, return all the documents if null
    \param [in] selector The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListBackup ( sdbConnectionHandle cHandle,
                                 bson *options,
                                 bson *condition,
                                 bson *selector,
                                 bson *orderBy,
                                 sdbCursorHandle *handle ) ;

/** \fn INT32 sdbRemoveBackup ( sdbConnectionHandle cHandle,
                                bson *options) ;
    \brief Remove the backups.
    \param [in] cHandle The connection handle
    \param [in] options Contains configuration infomations for removing backups, remove all the backups in the default backup path if null. The "options" contains several options as below. All the elements in options are optional. eg: {"GroupName":["RGName1", "RGName2"], "Path":"/opt/sequoiadb/backup", "Name":"backupName"}

        GroupID     : Specified the group id of the backups, default to list all the backups of all the groups.
        GroupName   : Specified the group name of the backups, default to list all the backups of all the groups.
        Path        : Specified the path of the backups, default to use the backup path asigned in the configuration file.
        Name        : Specified the name of backup, default to list all the backups.
        IsSubDir    : Specified the "Path" is a subdirectory of the backup path assigned in the configuration file or not, default to be false.
        Prefix      : Specified the prefix name of the backups, support for using wildcards("%g","%G","%h","%H","%s","%s"),such as: Prefix:"%g_bk_", default to not using wildcards.
        Detail      : Display the detail of the backups or not, default to be false.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRemoveBackup ( sdbConnectionHandle cHandle,
                                   bson *options) ;

/** \fn INT32 sdbListTasks ( sdbConnectionHandle cHandle,
                             bson *condition,
                             bson *selector,
                             bson *orderBy,
                             bson *hint,
                             sdbCursorHandle *handle ) ;
    \brief List the tasks.
    \param [in] cHandle The connection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] selector The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListTasks ( sdbConnectionHandle cHandle,
                                bson *condition,
                                bson *selector,
                                bson *orderBy,
                                bson *hint,
                                sdbCursorHandle *handle ) ;

/** \fn INT32 sdbWaitTasks ( sdbConnectionHandle cHandle,
                             const SINT64 *taskIDs,
                             SINT32 num ) ;
    \brief Wait the tasks to finish.
    \param [in] cHandle The connection handle
    \param [in] taskIDs The array of task id
    \param [in] num The number of task id
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbWaitTasks ( sdbConnectionHandle cHandle,
                                const SINT64 *taskIDs,
                                SINT32 num );

/** \fn INT32 sdbCancelTask ( sdbConnectionHandle cHandle,
                              SINT64 taskID,
                              BOOLEAN isAsync ) ;
    \brief Cancel the specified task.
    \param [in] cHandle The connection handle
    \param [in] taskID The task id
    \param [in] isAsync The operation "cancel task" is async or not,
                               "true" for async, "false" for sync. Default sync.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCancelTask ( sdbConnectionHandle cHandle,
                                 SINT64 taskID,
                                 BOOLEAN isAsync ) ;

/** \fn INT32 sdbSetSessionAttr ( sdbConnectionHandle cHandle,
                                  bson *options ) ;
    \brief Set the attributes of the session.
    \param [in] cHandle The connection handle
    \param [in] options The configuration options for session.The options are as below:

        PreferedInstance : Preferred instance for read request in the current session. Could be single value in "M", "m", "S", "s", "A", "a", 1-255, or BSON Array to include multiple values.
                           e.g. { "PreferedInstance" : [ 1, 7 ] }.
                           "M", "m": read and write instance( master instance ). If multiple numeric instances are given with "M", matched master instance will be chosen in higher priority. If multiple numeric instances are given with "M" or "m", master instance will be chosen if no numeric instance is matched.
                           "S", "s": read only instance( slave instance ). If multiple numeric instances are given with "S", matched slave instances will be chosen in higher priority. If multiple numeric instances are given with "S" or "s", slave instance will be chosen if no numeric instance is matched.
                           "A", "a": any instance.
                           1-255: the instance with specified instance ID.
                           If multiple alphabet instances are given, only first one will be used.
                           If matched instance is not found, will choose instance by random.
        PreferedInstanceMode : The mode to choose query instance when multiple preferred instances are found in the current session.
                               e.g. { "PreferedInstanceMode : "random" }.
                               "random": choose the instance from matched instances by random.
                               "ordered": choose the instance from matched instances by the order of "PreferedInstance".
        Timeout : The timeout (in ms) for operations in the current session. -1 means no timeout for operations.
                  e.g. { "Timeout" : 10000 }.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSetSessionAttr ( sdbConnectionHandle cHandle,
                                     bson *options ) ;

/** \fn INT32 sdbGetSessionAttr ( sdbConnectionHandle cHandle,
                                  bson * result ) ;
    \brief Set the attributes of the session.
    \param [in] cHandle The connection handle
    \param [out] result The return bson object
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetSessionAttr ( sdbConnectionHandle cHandle,
                                     bson * result ) ;

/** \fn INT32 sdbIsValid( sdbConnectionHandle cHandle, BOOLEAN *result )
    \brief Judge whether the connection is valid.
    \param [out] result the output result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbIsValid( sdbConnectionHandle cHandle, BOOLEAN *result ) ;

SDB_EXPORT INT32 _sdbMsg ( sdbConnectionHandle cHandle, const CHAR *msg ) ;

/** \fn INT32 sdbCreateDomain ( sdbConnectionHandle cHandle,
                                const CHAR *pDomainName,
                                bson *options,
                                sdbDomainHandle *handle ) ;
    \brief Create a domain.
    \param [in] cHandle The database connection handle
    \param [in] pDomainName The name of the domain
    \param [in] options The options for the domain. The options are as below:

        Groups:    The list of replica groups' names which the domain is going to contain.
                   eg: { "Groups": [ "group1", "group2", "group3" ] }
                   If this argument is not included, the domain will contain all replica groups in the cluster.
        AutoSplit: If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
                   the data of this collection will be split(hash split) into all the groups in this domain automatically.
                   However, it won't automatically split data into those groups which were add into this domain later.
                   eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
    \param [out] handle The domain handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateDomain ( sdbConnectionHandle cHandle,
                                   const CHAR *pDomainName,
                                   bson *options,
                                   sdbDomainHandle *handle ) ;

/** \fn INT32 sdbDropDomain ( sdbConnectionHandle cHandle,
                              const CHAR *pDomainName ) ;
    \brief Drop a domain.
    \param [in] cHandle The database connection handle
    \param [in] pDomainName The name of the domain
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDropDomain ( sdbConnectionHandle cHandle,
                                 const CHAR *pDomainName ) ;

/** \fn INT32 sdbGetDomain ( sdbConnectionHandle cHandle,
                             const CHAR *pDomainName,
                             sdbDomainHandle *handle ) ;
    \brief Get a domain.
    \param [in] cHandle The database connection handle
    \param [in] pDomainName The name of the domain
    \param [out] handle The domain handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetDomain ( sdbConnectionHandle cHandle,
                                const CHAR *pDomainName,
                                sdbDomainHandle *handle ) ;

/** \fn INT32 sdbListDomains ( sdbConnectionHandle cHandle,
                                  bson *condition,
                                  bson *selector,
                                  bson *orderBy,
                                  sdbCursorHandle *handle )
    \brief List the domains.
    \param [in] cHandle The connection handle
    \param [in] condition The matching rule, return all the documents if null
    \param [in] selector The selective rule, return the whole document if null
    \param [in] orderBy The ordered rule, never sort if null
    \param [in] hint Specified the index used to scan data. e.g. {"":"ageIndex"} means
                    using index "ageIndex" to scan data(index scan);
                    {"":null} means table scan. when hint is null,
                    database automatically match the optimal index to scan data
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListDomains ( sdbConnectionHandle cHandle,
                                  bson *condition,
                                  bson *selector,
                                  bson *orderBy,
                                  sdbCursorHandle *handle ) ;

/** \fn INT32 sdbAlterDomain( sdbDomainHandle cHandle,
                              const bson *options ) ;
    \brief alter the current domain.
    \param [in] cHandle The domain handle
    \param [in] options The options user wants to alter

        Groups:    The list of replica groups' names which the domain is going to contain.
                   eg: { "Groups": [ "group1", "group2", "group3" ] }, it means that domain
                   changes to contain "group1" "group2" or "group3".
                   We can add or remove groups in current domain. However, if a group has data
                   in it, remove it out of domain will be failing.
        AutoSplit: Alter current domain to have the ability of automatically split or not.
                   If this option is set to be true, while creating collection(ShardingType is "hash") in this domain,
                   the data of this collection will be split(hash split) into all the groups in this domain automatically.
                   However, it won't automatically split data into those groups which were add into this domain later.
                   eg: { "Groups": [ "group1", "group2", "group3" ], "AutoSplit: true" }
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAlterDomain( sdbDomainHandle cHandle,
                                 const bson *options ) ;

/** \fn INT32 sdbListCollectionSpacesInDomain( sdbDomainHandle cHandle,
                                               sdbCursorHandle *cursor ) ;
    \brief list the collection spaces in domain.
    \param [in] cHandle The domain handle
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListCollectionSpacesInDomain( sdbDomainHandle cHandle,
                                                  sdbCursorHandle *cursor ) ;

/** \fn INT32 sdbListCollectionsInDomain( sdbDomainHandle cHandle,
                                          sdbCursorHandle *cursor ) ;
    \brief list the collections in domain.
    \param [in] cHandle The domain handle
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListCollectionsInDomain( sdbDomainHandle cHandle,
                                             sdbCursorHandle *cursor ) ;

/** \fn INT32 sdbListGroupsInDomain( sdbDomainHandle cHandle,
                                     sdbCursorHandle *cursor ) ;
    \brief list the groups in domain.
    \param [in] cHandle The domain handle
    \param [out] handle The cusor handle of result
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListGroupsInDomain( sdbDomainHandle cHandle,
                                        sdbCursorHandle *cursor ) ;

/** \fn INT32 sdbInvalidateCache( sdbConnectionHandle cHandle,
                                  bson *condition )
    \brief invalidate cache on specified nodes.
    \param [in] cHandle The connection handle
    \param [in] condition The destination we want to invalidate.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbInvalidateCache( sdbConnectionHandle cHandle,
                                     bson *condition ) ;

/** \fn INT32 sdbForceSession( sdbConnectionHandle cHandle,
                               SINT64 sessionID )
    \brief interrupte the session
    \param [in] cHandle The connection handle
    \param [in] sessionID The id of the session which we want to inerrupt
    \param [in] options The location information, such as NodeID, HostName and svcname
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbForceSession( sdbConnectionHandle cHandle,
                                  SINT64 sessionID,
                                  bson *options ) ;

/** \fn INT32 sdbOpenLob( sdbCollectionHandle cHandle,
                          const bson_oid_t *oid,
                          INT32 mode,
                          sdbLobHandle *lobHandle )
    \brief create a large object or open a large object to read or write
    \param [in] cHandle The collection handle
    \param [in] oid The object id
    \param [in] mode The open mode: SDB_LOB_CREATEONLY/SDB_LOB_READ/SDB_LOB_WRITE
    \param [out] lobHandle The handle of object
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbOpenLob( sdbCollectionHandle cHandle,
                             const bson_oid_t *oid,
                             INT32 mode,
                             sdbLobHandle *lobHandle ) ;

/** \fn INT32 sdbWriteLob( sdbLobHandle lobHandle,
                           const CHAR *buf,
                           UINT32 len )
    \brief write lob
    \param [in] lobHandle The large object handle
    \param [in] buf The buf of write
    \param [in] len The length of write
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbWriteLob( sdbLobHandle lobHandle,
                              const CHAR *buf,
                              UINT32 len ) ;

/** \fn INT32 sdbLockLob( sdbLobHandle lobHandle,
                          INT64 offset,
                          INT64 length )
    \brief lock LOB section for write mode
    \param [in] lobHandle The large object handle
    \param [in] offset The lock start position
    \param [in] length The lock length, -1 means lock to the end of lob
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbLockLob( sdbLobHandle lobHandle,
                             INT64 offset,
                             INT64 length ) ;

/** \fn INT32 sdbLockAndSeekLob( sdbLobHandle lobHandle,
                                 INT64 offset,
                                 INT64 length )
    \brief lock LOB section for write mode and seek to the offset position
    \param [in] lobHandle The large object handle
    \param [in] offset The lock start position
    \param [in] length The lock length, -1 means lock to the end of lob
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbLockAndSeekLob( sdbLobHandle lobHandle,
                                    INT64 offset,
                                    INT64 length ) ;

/** \fn INT32 sdbReadLob( sdbLobHandle lobHandle,
                          UINT32 len,
                          CHAR *buf,
                          UINT32 *read )
    \brief read lob
    \param [in] lobHandle The large object handle
    \param [in] len The length want to read
    \param [out] buf Put the data into buf
    \param [out] read The length of read
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbReadLob( sdbLobHandle lobHandle,
                             UINT32 len,
                             CHAR *buf,
                             UINT32 *read ) ;

/** \fn INT32 sdbCloseLob( sdbLobHandle *lobHandle )
    \brief close lob
    \param [in] lobHandle The large object handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCloseLob( sdbLobHandle *lobHandle ) ;

/** \fn INT32 sdbRemoveLob( sdbCollectionHandle cHandle,
                            const bson_oid_t *oid )
    \brief remove lob
    \param [in] cHandle The handle of collection
    \param [in] oid The large object id
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRemoveLob( sdbCollectionHandle cHandle,
                               const bson_oid_t *oid ) ;

/** \fn INT32 sdbTruncateLob( sdbCollectionHandle cHandle,
                            const bson_oid_t *oid, INT64 length )
    \brief truncate lob
    \param [in] cHandle The handle of collection
    \param [in] oid The large object id
    \param [in] length The truncate length
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTruncateLob( sdbCollectionHandle cHandle,
                               const bson_oid_t *oid, INT64 length ) ;

/** \fn INT32 sdbGetLobSize( sdbLobHandle lobHandle,
                             SINT64 *size )
    \brief get the lob's size
    \param [in] lobHandle The large object handle
    \param [out] size The size of lob
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetLobSize( sdbLobHandle lobHandle,
                                SINT64 *size ) ;

/** \fn INT32 sdbGetLobCreateTime( sdbLobHandle lobHandle,
                                   UINT64 *millis )
    \brief get lob's create time
    \param [in] lobHandle The large object handle
    \param [out] millis The create time in milliseconds of lob
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetLobCreateTime( sdbLobHandle lobHandle,
                                      UINT64 *millis ) ;

/** \fn INT32 sdbGetLobModificationTime( sdbLobHandle lobHandle,
                                         UINT64 *millis )
    \brief get lob's last modification time
    \param [in] lobHandle The large object handle
    \param [out] millis The modification time in milliseconds of lob
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetLobModificationTime( sdbLobHandle lobHandle,
                                            UINT64 *millis ) ;

/** \fn INT32 sdbSeekLob( sdbLobHandle lobHandle,
                          SINT64 size,
                          SDB_LOB_SEEK whence )
    \brief seek the place to read or write
    \param [in] lobHandle The large object handle
    \param [in] size The size of seek
    \param [in] whence The whence of seek
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSeekLob( sdbLobHandle lobHandle,
                             SINT64 size,
                             SDB_LOB_SEEK whence ) ;

/** \fn INT32 sdbListLobs( sdbCollectionHandle cHandle,
                           sdbCursorHandle *cursor )
    \brief list all the lobs' meta data in current collection
    \param [in] cHandle The collection handle
    \param [out] cursor The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListLobs( sdbCollectionHandle cHandle,
                              sdbCursorHandle *cursor ) ;

/** \fn INT32 sdbListLobPieces( sdbCollectionHandle cHandle,
                                sdbCursorHandle *cursor )
    \brief list all the pieces in the lob
    \param [in] cHandle The collection handle
    \param [out] cursor The cursor handle of current query
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbListLobPieces( sdbCollectionHandle cHandle,
                                   sdbCursorHandle *cursor ) ;


/** \fn INT32 sdbReelect( sdbReplicaGroupHandle cHandle,
                          const bson *options )
    \param [in] cHandle The replica group handle
    \param [in] options The options of reelection
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbReelect( sdbReplicaGroupHandle cHandle,
                             const bson *options ) ;

/** \fn INT32 sdbForceStepUp( sdbConnectionHandle cHandle,
                              const bson *options )
    \param [in] cHandle The connection handle
    \param [in] options The options of step up
    \retval SDB_OK Operation Success
   \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbForceStepUp( sdbConnectionHandle cHandle,
                                 const bson *options ) ;

/** \fn INT32 sdbTruncateCollection( sdbConnectionHandle cHandle,
                                    const CHAR *fullName )
    \brief truncate the collection
    \param [in] cHandle The handle of connection.
    \param [in] fullName The full name of collection to be truncated, eg: foo.bar.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbTruncateCollection( sdbConnectionHandle cHandle,
                                        const CHAR *fullName ) ;

/* \fn INT32 sdbPop( sdbConnectionHandle cHandle,
                      const CHAR *fullName,
                      bson *options )
    \brief pop records from capped collection
    \param [in] cHandle The handle of connection.
    \param [in] fullName The full name of collection to be popped, eg: foo.bar.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbPop( sdbConnectionHandle cHandle,
                         const CHAR *fullName,
                         bson *options ) ;


/** \fn INT32 sdbDetachNode( sdbReplicaGroupHandle cHandle,
                             const CHAR *hostName,
                             const CHAR *serviceName,
                             const bson *options )
    \brief detach a node from the group
    \param [in] cHandle The handle of group.
    \param [in] hostName The host name of node.
    \param [in] serviceName The service name of node.
    \param [in] optoins The options of detach.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDetachNode( sdbReplicaGroupHandle cHandle,
                                const CHAR *hostName,
                                const CHAR *serviceName,
                                const bson *options ) ;

/** \fn INT32 sdbAttachNode( sdbReplicaGroupHandle cHandle,
                             const CHAR *hostName,
                             const CHAR *serviceName,
                             const bson *options )
    \brief attach a node to the group
    \param [in] cHandle The handle of group.
    \param [in] hostName The host name of node.
    \param [in] serviceName The service name of node.
    \param [in] optoins The options of attach.
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAttachNode( sdbReplicaGroupHandle cHandle,
                                const CHAR *hostName,
                                const CHAR *serviceName,
                                const bson *options ) ;

/** \fn INT32 sdbCreateIdIndex( sdbCollectionHandle cHandle, const bson *args )
    \brief Create $id index in collection
    \param [in] cHandle The collection handle.
    \param [in] args The arguments of creating id index. set it as null if no args. e.g.{SortBufferSize:64}

        SortBufferSize     : The size of sort buffer used when creating index, the unit is MB,
                             zero means don't use sort buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateIdIndex( sdbCollectionHandle cHandle,
                                   const bson *args ) ;

/** \fn INT32 sdbDropIdIndex( sdbCollectionHandle cHandle )
    \brief Drop $id index in collection
    \param [in] cHandle The collection handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
    \note delete, update and upsert do not work after index "$id" was drop
*/
SDB_EXPORT INT32 sdbDropIdIndex( sdbCollectionHandle cHandle ) ;

/* \fn INT32 sdbGetDCName( sdbDCHandle cHandle, CHAR *pBuffer, INT32 size )
    \brief Get the name of the data center
    \param [in] cHandle The data center handle
    \param [in] pBuffer The output buffer
    \param [in] size The size of the output buffer
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetDCName( sdbDCHandle cHandle, CHAR *pBuffer, INT32 size ) ;

/* \fn INT32 sdbGetDC( sdbConnectionHandle cHandle, sdbDCHandle *handle )
    \brief Get the data center
    \param [in] cHandle The connection handle
    \param [out] handle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetDC( sdbConnectionHandle cHandle, sdbDCHandle *handle ) ;

/* \fn INT32 sdbGetDCDetail( sdbDCHandle cHandle, bson *retInfo )
    \brief Get the detail of data center
    \param [in] cHandle The connection handle
    \param [out] retInfo The the detail of data center
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbGetDCDetail( sdbDCHandle cHandle, bson *retInfo ) ;

/* \fn INT32 sdbActivateDC( sdbDCHandle cHandle )
    \brief Activate the data center
    \param [in] cHandle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbActivateDC( sdbDCHandle cHandle ) ;

/* \fn INT32 sdbDeactivateDC( sdbDCHandle cHandle )
    \brief Deactivate the data center
    \param [in] cHandle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDeactivateDC( sdbDCHandle cHandle ) ;

/* \fn INT32 sdbEnableReadOnly( sdbDCHandle cHandle, BOOLEAN isReadOnly )
    \brief Enable data center works in readonly mode or not
    \param [in] cHandle The data center handle
    \param [in] isReadOnly Whether to use readonly mode or not
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbEnableReadOnly( sdbDCHandle cHandle, BOOLEAN isReadOnly ) ;

/* \fn INT32 sdbCreateImage( sdbDCHandle cHandle, const CHAR *pCataAddrList )
    \brief Create image in data center
    \param [in] cHandle The data center handle
    \param [in] pCataAddrList Catalog address list of remote data center, e.g. "192.168.20.165:30003",
                "192.168.20.165:30003,192.168.20.166:30003"
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbCreateImage( sdbDCHandle cHandle, const CHAR *pCataAddrList ) ;

/* \fn INT32 sdbRemoveImage( sdbDCHandle cHandle )
    \brief Remove image in data center
    \param [in] cHandle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRemoveImage( sdbDCHandle cHandle ) ;

/* \fn INT32 sdbEnableImage( sdbDCHandle cHandle )
    \brief Enable image in data center
    \param [in] cHandle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbEnableImage( sdbDCHandle cHandle ) ;

/* \fn INT32 sdbDisableImage( sdbDCHandle cHandle )
    \brief Disable image in data center
    \param [in] cHandle The data center handle
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDisableImage( sdbDCHandle cHandle ) ;

/* \fn INT32 sdbAttachGroups( sdbDCHandle cHandle, bson *info )
    \brief Attach specified groups to data center
    \param [in] cHandle The data center handle
    \param [in] info The information of groups to attach, e.g. {Groups:[["group1", "group1"], ["group2", "group2"]]}
    \code
      bson obj ;
      bson_init( &obj ) ;
      bson_append_start_array( &obj, "Groups" ) ;

      bson_append_start_array( &obj, "0" ) ;
      bson_append_string( &obj, "0", "group1" ) ;
      bson_append_string( &obj, "1", "group1" ) ;
      bson_append_finish_array( &obj ) ;

      bson_append_start_array( &obj, "0" ) ;
      bson_append_string( &obj, "0", "group2" ) ;
      bson_append_string( &obj, "1", "group2" ) ;
      bson_append_finish_array( &obj ) ;

      bson_append_finish_array( &obj ) ;

      rc = bson_finish( &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      rc = sdbAttachGroups( dc, &obj ) ;
      ASSERT_EQ( SDB_OK, rc ) ;

      bson_destroy( &obj ) ;
    \endcode
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAttachGroups( sdbDCHandle cHandle, bson *info ) ;

/* \fn INT32 sdbDetachGroups( sdbDCHandle cHandle, bson *info )
    \brief Detach specified groups from data center
    \param [in] cHandle The data center handle
    \param [in] info The information of groups to detach, e.g. {Groups:[["a", "a"], ["b", "b"]]}
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDetachGroups( sdbDCHandle cHandle, bson *info ) ;

/** \fn INT32 sdbSyncDB( sdbConnectionHandle cHandle, bson *options )
    \brief sync database which are specified
    \param [in] cHandle The database connection handle
    \param [in] options The control options:

        Deep: (INT32) Flush with deep mode or not. 1 in default.
              0 for non-deep mode,1 for deep mode,-1 means use the configuration with server
        Block: (Bool) Flush with block mode or not. false in default.
        CollectionSpace: (String) Specify the collectionspace to sync.
                         If not set, will sync all the collectionspaces and logs,
                         otherwise, will only sync the collectionspace specified.
        Some of other options are as below:(only take effect in coordinate nodes,
                       please visit the official website to search "sync" or
                       "Location Elements" for more detail.)
        GroupID:INT32,
        GroupName:String,
        NodeID:INT32,
        HostName:String,
        svcname:String,
        ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSyncDB( sdbConnectionHandle cHandle,
                            bson *options ) ;

/** \fn INT32 sdbLoadCollectionSpace( sdbConnectionHandle cHandle,
                                      const CHAR *csName,
                                      bson *options )
    \brief Load the specified collection space to database from file
    \param [in] cHandle The database connection handle
    \param [in] csName The specified collection space name
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbLoadCollectionSpace( sdbConnectionHandle cHandle,
                                         const CHAR *csName,
                                         bson *options ) ;

/** \fn INT32 sdbUnloadCollectionSpace( sdbConnectionHandle cHandle,
                                        const CHAR *csName,
                                        bson *options )
    \brief Unload the specified collection space from database
    \param [in] cHandle The database connection handle
    \param [in] csName The specified collection space name
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbUnloadCollectionSpace( sdbConnectionHandle cHandle,
                                           const CHAR *csName,
                                           bson *options ) ;

/** \fn INT32 sdbSetPDLevel( sdbConnectionHandle cHandle,
                             INT32 pdLevel,
                             bson *options )
    \brief Set the node's diagnostic level
    \param [in] cHandle The database connection handle
    \param [in] pdLevel Diagnostic level, value:[0~5]
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbSetPDLevel( sdbConnectionHandle cHandle,
                                INT32 pdLevel,
                                bson *options ) ;

/** \fn INT32 sdbReloadConfig( sdbConnectionHandle cHandle,
                               bson *options )
    \brief Force the node to reload configs online
    \param [in] cHandle The database connection handle
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbReloadConfig( sdbConnectionHandle cHandle,
                                  bson *options ) ;

/** \fn INT32 sdbUpdateConfig( sdbConnectionHandle cHandle,
                               bson *configs, bson *options )
    \brief Force the node to update configs online
    \param [in] cHandle The database connection handle
    \param [in] configs the specific configuration parameters to update
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbUpdateConfig( sdbConnectionHandle cHandle,
                                  bson *configs, bson *options ) ;

/** \fn INT32 sdbDeleteConfig( sdbConnectionHandle cHandle,
                               bson *configs, bson *options )
    \brief Force the node to delete configs online
    \param [in] cHandle The database connection handle
    \param [in] configs the specific configuration parameters to delete
    \param [in] options The control options:(Only take effect in coordinate nodes)
                GroupID:INT32,
                GroupName:String,
                NodeID:INT32,
                HostName:String,
                svcname:String,
                ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbDeleteConfig( sdbConnectionHandle cHandle,
                                  bson *configs, bson *options ) ;

/** \fn INT32 sdbRenameCollectionSpace( sdbConnectionHandle cHandle,
                                        const CHAR *pOldName,
                                        const CHAR *pNewName,
                                        bson *options )
    \brief Rename the collectionspace name
    \param [in] cHandle The database connection handle
    \param [in] pOldName The old collection space name
    \param [in] pNewName The new collection space name
    \param [in] options Reserved
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbRenameCollectionSpace( sdbConnectionHandle cHandle,
                                           const CHAR *pOldName,
                                           const CHAR *pNewName,
                                           bson *options ) ;

/** \fn void sdbSetConnectionInterruptFunc( sdbConnectionHandle cHandle,
 *                                          socketInterruptFunc func )
 *  \param [in] cHandle The handle of connection.
 *  \param [in] func The function that check the app is interrupt or not
 *  \retval void
 */
SDB_EXPORT void sdbSetConnectionInterruptFunc(
                                          sdbConnectionHandle cHandle,
                                          socketInterruptFunc func ) ;

/** \fn INT32 sdbAnalyze( sdbConnectionHandle cHandle, bson *options )
    \brief Analyze collection or index to collect statistics information
    \param [in] cHandle The database connection handle
    \param [in] options The control options:

        CollectionSpace : (String) Specify the collection space to be analyzed.
        Collection      : (String) Specify the collection to be analyzed.
        Index           : (String) Specify the index to be analyzed.
        Mode            : (Int32) Specify the analyze mode (default is 1):
                          Mode 1 will analyze with data samples.
                          Mode 2 will analyze with full data.
                          Mode 3 will generate default statistics.
                          Mode 4 will reload statistics into memory cache.
                          Mode 5 will clear statistics from memory cache.
        Other options   : Some of other options are as below:(only take effect
                          in coordinate nodes, please visit the official website
                          to search "analyze" or "Location Elements" for more
                          detail.)
                          GroupID:INT32,
                          GroupName:String,
                          NodeID:INT32,
                          HostName:String,
                          svcname:String,
                          ...
    \retval SDB_OK Operation Success
    \retval Others Operation Fail
*/
SDB_EXPORT INT32 sdbAnalyze( sdbConnectionHandle cHandle,
                             bson *options ) ;

SDB_EXTERN_C_END
#endif

