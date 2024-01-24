/*******************************************************************************
   Copyright (C) 2023-present SequoiaDB Ltd.

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

#ifndef COMMON_H__
#define COMMON_H__

#include "msg.h"

#ifndef __cplusplus
#include "bson/bson.h"
#endif

SDB_EXTERN_C_START
#define CLIENT_RECORD_ID_FIELD "_id"
#define CLIENT_RECORD_ID_INDEX "$id"
#define CLIENT_RECORD_ID_FIELD_STRLEN 3

#if defined (_WINDOWS)
#define SOCKET_INVALIDSOCKET  INVALID_SOCKET
#else
#define SOCKET_INVALIDSOCKET  -1
#endif

#define SDB_MD5_DIGEST_LENGTH 16
#define CLI_INT_TO_STR_MAX_SIZE 10

#define clientItoa(x,y,z) if (y) { ossSnprintf(y, z, "%d", (INT32)(x) );}

typedef struct _htbNode
{
   UINT64 lastTime ;
   CHAR *name ;
   INT32 version ;
} htbNode ;

typedef struct _hashTable
{
   UINT32  capacity ;
   htbNode **node ;
} hashTable ;

INT32 hash_table_create_node( const CHAR *key, htbNode **node ) ;

INT32 hash_table_destroy_node( htbNode **node ) ;

INT32 hash_table_insert( hashTable *tb, htbNode *node ) ;

INT32 hash_table_remove( hashTable *tb, const CHAR *key, BOOLEAN dropCS ) ;

INT32 hash_table_fetch( hashTable *tb, const CHAR *key, htbNode **node ) ;

INT32 hash_table_create( hashTable **tb, const UINT32 bucketSize ) ;

INT32 hash_table_destroy( hashTable **tb ) ;

INT32 insertCachedObject( hashTable *tb, const CHAR *key ) ;

INT32 insertCachedVersion( hashTable *tb, const CHAR *key, INT32 version ) ;

INT32 removeCachedObject( hashTable *tb, const CHAR *key, BOOLEAN dropCS ) ;

BOOLEAN fetchCachedVersion( hashTable *tb, const CHAR *key,INT32 *pVersion ) ;

BOOLEAN fetchCachedObject( hashTable *tb, const CHAR *key ) ;

INT32 updateCachedObject( const INT32 code, hashTable *tb, const CHAR *key ) ;

INT32 updateCachedVersion( const INT32 code, hashTable *tb, const CHAR *key, INT32 version ) ;

INT32 initCacheStrategy( BOOLEAN enableCacheStrategy,
                         const UINT32 timeInterval ) ;
INT32 initHashTable( hashTable **tb ) ;
INT32 releaseHashTable( hashTable **tb ) ;

/// for regulate query flag
INT32 regulateQueryFlags( INT32 flags ) ;

INT32 eraseSingleFlag( INT32 flags, INT32 erasedFlag ) ;

INT32 clientCheckRetMsgHeader( const CHAR *pSendBuf, const CHAR *pRecvBuf,
                               BOOLEAN endianConvert ) ;


#ifdef __cplusplus

INT32 clientBuildUpdateMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR*selector,
                                const CHAR*updator,
                                const CHAR*hint,
                                BOOLEAN endianConvert ) ;

INT32 clientAppendInsertMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *insertor,
                                 BOOLEAN endianConvert ) ;

INT32 clientAppendHint2InsertMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                     const CHAR *hint,
                                     BOOLEAN endianConvert ) ;

INT32 clientBuildInsertMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR *insertor,
                                const CHAR *hint,
                                BOOLEAN endianConvert ) ;

INT32 clientBuildTransactionCommitMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                          UINT64 reqID,
                                          const CHAR *hint,
                                          BOOLEAN endianConvert ) ;

INT32 clientBuildQueryMsgCpp  ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                SINT64 numToSkip,
                                SINT64 numToReturn,
                                const CHAR *query,
                                const CHAR *fieldSelector,
                                const CHAR *orderBy,
                                const CHAR *hint,
                                BOOLEAN endianConvert ) ;

INT32 clientBuildAdvanceMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                 INT64 contextID, UINT64 reqID,
                                 const CHAR *option,
                                 const CHAR *pBackData,
                                 INT32 backDataSize,
                                 BOOLEAN endianConvert ) ;

INT32 clientBuildDeleteMsgCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *CollectionName,
                                SINT32 flag, UINT64 reqID,
                                const CHAR *deletor,
                                const CHAR *hint,
                                BOOLEAN endianConvert ) ;

INT32 clientBuildAggrRequestCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *CollectionName,
                                 const CHAR *obj,
                                 BOOLEAN endianConvert ) ;

INT32 clientAppendAggrRequestCpp ( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *obj,
                                   BOOLEAN endianConvert ) ;

INT32 clientBuildLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                            INT32 msgType, const CHAR *pMeta,
                            SINT32 flags, SINT16 w,
                            SINT64 contextID, UINT64 reqID,
                            const SINT64 *lobOffset,
                            const UINT32 *len,
                            const CHAR *data,
                            BOOLEAN endianConvert ) ;

INT32 clientBuildOpenLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *pMeta,
                                SINT32 flags, SINT16 w,
                                UINT64 reqID,
                                BOOLEAN endianConvert ) ;

INT32 clientBuildCreateLobIDMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                    const CHAR *pMeta, SINT32 flags, SINT16 w,
                                    UINT64 reqID, BOOLEAN endianConvert ) ;


INT32 clientBuildRemoveLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                  const CHAR *pMeta,
                                  SINT32 flags, SINT16 w,
                                  UINT64 reqID,
                                  BOOLEAN endianConvert ) ;

INT32 clientBuildTruncateLobMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                    const CHAR *pMeta,
                                    SINT32 flags, SINT16 w,
                                    UINT64 reqID,
                                    BOOLEAN endianConvert ) ;

INT32 clientBuildAuthCrtMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                const CHAR *pUsrName,
                                const CHAR *clearTextPasswd,
                                const CHAR *pPasswd,
                                const CHAR *pOptions,
                                UINT64 reqID, BOOLEAN endianConvert,
                                INT32 authVersion ) ;

INT32 clientBuildSeqFetchMsgCpp( CHAR **ppBuffer, INT32 *bufferSize,
                                 const CHAR *seqName, INT32 fetchNum,
                                 UINT64 reqID, BOOLEAN endianConvert ) ;
#else // __cplusplus

INT32 clientBuildUpdateMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             bson *selector, bson *updator,
                             bson *hint, BOOLEAN endianConvert ) ;

INT32 clientAppendInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              bson *insertor, BOOLEAN endianConvert ) ;

INT32 clientBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             bson *insertor,
                             bson *hint,
                             BOOLEAN endianConvert ) ;

INT32 clientAppendHint2InsertMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  bson *hint, BOOLEAN endianConvert ) ;

INT32 clientBuildTransactionCommitMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                       UINT64 reqID,
                                       bson *hint,
                                       BOOLEAN endianConvert ) ;

INT32 clientBuildQueryMsg  ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName, SINT32 flag,
                             UINT64 reqID,
                             SINT64 numToSkip, SINT64 numToReturn,
                             const bson *query, const bson *fieldSelector,
                             const bson *orderBy, const bson *hint,
                             BOOLEAN endianConvert ) ;

INT32 clientBuildAdvanceMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              INT64 contextID, UINT64 reqID,
                              const bson *option,
                              const CHAR *pBackData,
                              INT32 backDataSize,
                              BOOLEAN endianConvert ) ;

INT32 clientBuildDeleteMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName,
                             SINT32 flag, UINT64 reqID,
                             bson *deletor,
                             bson *hint,
                             BOOLEAN endianConvert ) ;

INT32 clientAppendOID ( bson *obj, bson_iterator *ret ) ;

INT32 clientBuildAggrRequest1( CHAR **ppBuffer, INT32 *bufferSize,
                               const CHAR *CollectionName, bson **objs,
                               SINT32 num, BOOLEAN endianConvert ) ;

INT32 clientBuildAggrRequest( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *CollectionName, bson *obj,
                              BOOLEAN endianConvert ) ;

INT32 clientAppendAggrRequest ( CHAR **ppBuffer, INT32 *bufferSize,
                                bson *obj, BOOLEAN endianConvert ) ;

INT32 clientBuildLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         INT32 msgType, const bson *meta,
                         SINT32 flags, SINT16 w, SINT64 contextID,
                         UINT64 reqID, const SINT64 *lobOffset,
                         const UINT32 *len, const CHAR *data,
                         BOOLEAN endianConvert ) ;

INT32 clientBuildOpenLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const bson *meta, SINT32 flags, SINT16 w,
                             UINT64 reqID,
                             BOOLEAN endianConvert ) ;

INT32 clientBuildCreateLobIDMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                    const bson *meta, SINT32 flags, SINT16 w,
                                    UINT64 reqID, BOOLEAN endianConvert ) ;


INT32 clientBuildRemoveLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                               const bson *meta,
                               SINT32 flags, SINT16 w,
                               UINT64 reqID,
                               BOOLEAN endianConvert ) ;

INT32 clientBuildTruncateLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                 const bson *meta,
                                 SINT32 flags, SINT16 w,
                                 UINT64 reqID,
                                 BOOLEAN endianConvert ) ;

INT32 clientBuildAuthCrtMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *pUsrName,
                             const CHAR *clearTextPasswd,
                             const CHAR *pPasswd,
                             const bson *options,
                             UINT64 reqID, BOOLEAN endianConvert,
                             INT32 authVersion ) ;

INT32 clientBuildSeqFetchMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *seqName, INT32 fetchNum,
                              UINT64 reqID, BOOLEAN endianConvert ) ;
#endif // __cplusplus

INT32 clientBuildGetMoreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              SINT32 numToReturn,
                              SINT64 contextID, UINT64 reqID,
                              BOOLEAN endianConvert ) ;

INT32 clientBuildKillContextsMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                   UINT64 reqID, SINT32 numContexts,
                                   const SINT64 *pContextIDs,
                                   BOOLEAN endianConvert ) ;

INT32 clientBuildInterruptMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                UINT64 reqID, BOOLEAN isSelf,
                                BOOLEAN endianConvert ) ;

INT32 clientExtractReply ( CHAR *pBuffer, SINT32 *flag, SINT64 *contextID,
                           SINT32 *startFrom, SINT32 *numReturned,
                           BOOLEAN endianConvert ) ;

INT32 clientBuildDisconnectMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                 UINT64 reqID, BOOLEAN endianConvert ) ;

INT32 clientBuildSqlMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         const CHAR *sql, UINT64 reqID,
                         BOOLEAN endianConvert ) ;

INT32 clientBuildAuthVer0Msg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *pUsrName,
                              const CHAR *pPasswd,
                              UINT64 reqID, BOOLEAN endianConvert ) ;

INT32 clientBuildAuthVer1Step1Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *pUsrName,
                                   UINT64 reqID,
                                   BOOLEAN endianConvert,
                                   const CHAR *clientNonceBase64 ) ;

INT32 clientBuildAuthVer1Step2Msg( CHAR **ppBuffer, INT32 *bufferSize,
                                   const CHAR *pUsrName,
                                   UINT64 reqID,
                                   BOOLEAN endianConvert,
                                   const CHAR *combineNonceBase64,
                                   const CHAR *clientProofBase64,
                                   const CHAR *identify ) ;

INT32 clientBuildAuthDelMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *pUsrName,
                             const CHAR *pPasswd,
                             UINT64 reqID, BOOLEAN endianConvert ) ;

INT32 clientBuildTransactionBegMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                    UINT64 reqID,
                                    BOOLEAN endianConvert ) ;

INT32 clientBuildTransactionRollbackMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                         UINT64 reqID,
                                         BOOLEAN endianConvert ) ;

INT32 clientBuildSysInfoRequest ( CHAR **ppBuffer, INT32 *pBufferSize ) ;

INT32 clientExtractSysInfoReply ( CHAR *pBuffer, BOOLEAN *endianConvert,
                                  INT32 *osType, INT32 *authVersion,
                                  INT16 *peerProtocolVersion,
                                  UINT64 *dbStartTime,
                                  UINT8 *version, UINT8 *subVersion,
                                  UINT8 *fixVersion ) ;

INT32 clientValidateSql( const CHAR *sql, BOOLEAN isExec ) ;

INT32 clientBuildTestMsg( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *msg, UINT64 reqID,
                          BOOLEAN endianConvert ) ;

INT32 clientBuildReadLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                             UINT32 len, SINT64 offset,
                             SINT32 flags, SINT64 contextID,
                             UINT64 reqID,
                             BOOLEAN endianConvert ) ;

INT32 clientBuildGetLobRTimeMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                 SINT32 flags, SINT16 w, SINT64 contextID,
                                 UINT64 reqID, BOOLEAN endianConvert ) ;

INT32 clientBuildWriteLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *buf, UINT32 len,
                              SINT64 offset, SINT32 flags,
                              SINT16 w, SINT64 contextID,
                              UINT64 reqID,
                              BOOLEAN endianConvert ) ;

INT32 clientBuildLockLobMsg( CHAR ** ppBuffer, INT32 *bufferSize,
                             INT64 offset, INT64 length,
                             SINT32 flags, SINT16 w,
                             SINT64 contextID, UINT64 reqID,
                             BOOLEAN endianConvert ) ;

INT32 clientBuildCloseLobMsg( CHAR **ppBuffer, INT32 *bufferSize,
                              SINT32 flags, SINT16 w,
                              SINT64 contextID, UINT64 reqID,
                              BOOLEAN endianConvert ) ;

/*
   Other tool functions
*/
INT32 md5Encrypt( const CHAR *src,
                  CHAR *code,
                  UINT32 size ) ;

BOOLEAN isMd5String( const CHAR *str ) ;

INT32 clientReplicaGroupExtractNode ( const CHAR *data,
                                      CHAR *pHostName,
                                      INT32 hostNameSize,
                                      CHAR *pServiceName,
                                      INT32 serviceNameSize,
                                      INT32 *pNodeID ) ;

INT32 clientSnprintf( CHAR* pBuffer, INT32 bufSize, const CHAR* pFormat, ... ) ;

// For compatible communication with node which uses old version of protocol.
void clientMsgHeaderUpgrade( const MsgHeaderV1 *msgHeader,
                             MsgHeader *newMsgHeader ) ;

void clientMsgHeaderDowngrade( const MsgHeader *msgHeader,
                               MsgHeaderV1 *newMsgHeader ) ;

void clientMsgReplyHeaderUpgrade( const MsgOpReplyV1 *replyHeader,
                                  MsgOpReply *newReplyHeader ) ;

SDB_EXTERN_C_END

#endif // COMMON_H__

