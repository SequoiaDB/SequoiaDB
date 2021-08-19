/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = msgMessage.hpp

   Descriptive Name = Message Client Header

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains message structure for
   client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef MSGMESSAGE_HPP_
#define MSGMESSAGE_HPP_

#include "msgMessageFormat.hpp"
#include "sdbInterface.hpp"
#include <vector>
#include <string>

using namespace bson;
using namespace std;

/*
   When cb is not null, will allocate from cb. So must use cb->releaseBuf
   to free the ppBuffer
*/
INT32 msgCheckBuffer ( CHAR **ppBuffer,
                       INT32 *bufferSize,
                       INT32 packetLength,
                       engine::IExecutor *cb = NULL ) ;

void  msgReleaseBuffer( CHAR *pBuffer,
                        engine::IExecutor *cb = NULL ) ;

INT32 extractRC ( BSONObj &obj ) ;

BOOLEAN msgIsInnerOpReply( MsgHeader *pMsg ) ;

#define MSG_GET_INNER_REPLY_RC(msg) \
   ( msgIsInnerOpReply(msg) ? ((MsgOpReply*)(msg))->flags : \
                              ((MsgInternalReplyHeader*)(msg))->res )

#define MSG_GET_INNER_REPLY_DATA(msg) \
   ( msgIsInnerOpReply(msg) ? (const CHAR*)(msg) + sizeof(MsgOpReply) :\
                              (const CHAR*)(msg) + sizeof(MsgInternalReplyHeader) )

#define MSG_GET_INNER_REPLY_STARTFROM(msg) \
   ( msgIsInnerOpReply(msg) ? ((MsgOpReply*)(msg))->startFrom : 0 )

#define MSG_GET_INNER_REPLY_HEADER_LEN(msg) \
   ( msgIsInnerOpReply(msg) ? sizeof(MsgOpReply) : \
                              sizeof(MsgInternalReplyHeader) )

OSS_INLINE BOOLEAN isNoReplyMsg( INT32 opCode )
{
   if ( MSG_CAT_GRP_CHANGE_NTY == opCode ||
        MSG_BS_DISCONNECT == opCode ||
        MSG_BS_INTERRUPTE == opCode ||
        MSG_BS_INTERRUPTE_SELF == opCode )
   {
      return TRUE ;
   }
   return FALSE ;
}

OSS_INLINE BOOLEAN isTransBSMsg( INT32 opCode )
{
   if ( MSG_BS_TRANS_INSERT_REQ == GET_REQUEST_TYPE( opCode ) ||
        MSG_BS_TRANS_UPDATE_REQ == GET_REQUEST_TYPE( opCode ) ||
        MSG_BS_TRANS_DELETE_REQ == GET_REQUEST_TYPE( opCode ) ||
        MSG_BS_TRANS_QUERY_REQ  == GET_REQUEST_TYPE( opCode ) )
   {
      return TRUE ;
   }
   return FALSE ;
}

OSS_INLINE BOOLEAN isTransWriteMsg( INT32 opCode, const MsgHeader *pMsg )
{
   BOOLEAN ret = FALSE ;
   opCode = GET_REQUEST_TYPE( opCode ) ;
   switch ( opCode )
   {
      case MSG_BS_TRANS_INSERT_REQ :
      case MSG_BS_TRANS_UPDATE_REQ :
      case MSG_BS_TRANS_DELETE_REQ :
         ret = TRUE ;
         break ;
      case MSG_BS_TRANS_QUERY_REQ :
         if ( ((MsgOpQuery*)pMsg)->flags & FLG_QUERY_MODIFY )
         {
            ret = TRUE ;
         }
         break ;
      default:
         break ;
   }
   return ret ;
}

/*
 * Extract Commit Message from pBuffer
 * in pBuffer
 * out ppHint
 */
INT32 msgExtractTransCommit ( const CHAR *pBuffer, const CHAR **ppHint ) ;

/*
 * Create Update Message in ppBuffer
 * in/out ppBuffer
 * in/out bufferSize
 * in CollectionName
 * in flag
 * in selector
 * in updator
 * cb : memory allocator
 * return SDB_OK for success
 */
INT32 msgBuildUpdateMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName,
                          SINT32 flag, UINT64 reqID,
                          const BSONObj *selector = NULL,
                          const BSONObj *updator = NULL,
                          const BSONObj *hint = NULL,
                          engine::IExecutor *cb = NULL ) ;
/*
 * Extract Update Message from pBuffer
 * in pBuffer
 * out pflag
 * out ppCollectionName
 * out ppSelector
 * out ppUpdator
 */
INT32 msgExtractUpdate ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppSelector, CHAR **ppUpdator, CHAR **ppHint ) ;

/*
 * Create Insert Message in ppBuffer
 * in/out ppBuffer
 * in/out bufferSize
 * in CollectionName
 * in flag
 * in insertor
 * return SDB_OK for success
 */
INT32 msgBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          const BSONObj *insertor,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          void *pFiller, std::vector< CHAR * > &ObjQueue,
                          engine::netIOVec &ioVec,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgAppendInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                           const BSONObj *insertor,
                           engine::IExecutor *cb = NULL ) ;

/*
 * Extract Insert Message from pBuffer
 * in pBuffer
 * out pflag
 * out ppCollectionName
 * out ppInsertor
 */
INT32 msgExtractInsert ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppInsertor, INT32 &count ) ;

INT32 msgBuildQueryMsg  ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          SINT64 numToSkip, SINT64 numToReturn,
                          const BSONObj *query = NULL,
                          const BSONObj *fieldSelector = NULL,
                          const BSONObj *orderBy = NULL,
                          const BSONObj *hint = NULL,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgExtractQuery  ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         SINT64 *numToSkip, SINT64 *numToReturn,
                         CHAR **ppQuery, CHAR **ppFieldSelector,
                         CHAR **ppOrderBy, CHAR **ppHint ) ;

INT32 msgExtractQueryCommand ( CHAR *pBuffer, SINT32 packetSize, CHAR **ppCommand,
                               SINT32 &len );

INT32 msgBuildGetMoreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                           SINT32 numToReturn,
                           SINT64 contextID, UINT64 reqID,
                           engine::IExecutor *cb = NULL ) ;

INT32 msgExtractGetMore  ( CHAR *pBuffer,
                           SINT32 *numToReturn, SINT64 *contextID ) ;

void msgFillGetMoreMsg ( MsgOpGetMore &getMoreMsg, const UINT32 tid,
                         const SINT64 contextID, const SINT32 numToReturn,
                         const UINT64 reqID );

INT32 msgBuildDeleteMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          const BSONObj *deletor = NULL,
                          const BSONObj *hint = NULL,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgExtractDelete ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppDeletor, CHAR **ppHint ) ;

INT32 msgBuildKillContextsMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                               UINT64 reqID,
                               SINT32 numContexts, SINT64 *pContextIDs,
                               engine::IExecutor *cb = NULL ) ;

INT32 msgExtractKillContexts ( CHAR *pBuffer,
                              SINT32 *numContexts, SINT64 **ppContextIDs ) ;

INT32 msgBuildMsgMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                       UINT64 reqID, CHAR *pMsgStr,
                       engine::IExecutor *cb = NULL ) ;

INT32 msgExtractMsg ( CHAR *pBuffer,
                      CHAR **ppMsgStr ) ;

INT32 msgBuildReplyMsg ( CHAR **ppBuffer, INT32 *bufferSize, INT32 opCode,
                         SINT32 flag, SINT64 contextID, SINT32 startFrom,
                         SINT32 numReturned, UINT64 reqID,
                         vector<BSONObj> *objList,
                         engine::IExecutor *cb = NULL ) ;

INT32 msgBuildReplyMsg ( CHAR **ppBuffer, INT32 *bufferSize, INT32 opCode,
                         SINT32 flag, SINT64 contextID, SINT32 startFrom,
                         SINT32 numReturned, UINT64 reqID,
                         const BSONObj *bsonobj,
                         engine::IExecutor *cb = NULL ) ;

INT32 msgExtractReply ( CHAR *pBuffer, SINT32 *flag, SINT64 *contextID,
                        SINT32 *startFrom, SINT32 *numReturned,
                        vector<BSONObj> &objList ) ;

// Reply need sessionID ( nodeID + reqID ) to reply to sender, so we need to
// pass both
void msgBuildReplyMsgHeader ( MsgOpReply &replyHeader, SINT32 packetLength,
                              INT32 opCode, SINT32 flag, SINT64 contextID,
                              SINT32 startFrom, SINT32 numReturned,
                              MsgRouteID &routeID, UINT64 reqID ) ;

INT32 msgBuildDisconnectMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              UINT64 reqID,
                              engine::IExecutor *cb = NULL ) ;

// Disconnect need sessionID ( nodeID + reqID ) to send to data nodes, so we
// need to pass both
void msgBuildDisconnectMsg ( MsgOpDisconnect &disconnectHeader,
                             MsgRouteID &nodeID, UINT64 reqID );

INT32 msgBuildQueryCatalogReqMsg ( CHAR **ppBuffer, INT32 *pBufferSize,
                                   SINT32 flag, UINT64 reqID, SINT64 numToSkip,
                                   SINT64 numToReturn, UINT32 TID,
                                   const BSONObj *query,
                                   const BSONObj *fieldSelector,
                                   const BSONObj *orderBy,
                                   const BSONObj *hint,
                                   engine::IExecutor *cb = NULL ) ;

INT32 msgBuildQuerySpaceReqMsg ( CHAR **ppBuffer, INT32 *pBufferSize,
                                 SINT32 flag, UINT64 reqID, SINT64 numToSkip,
                                 SINT64 numToReturn, UINT32 TID,
                                 const BSONObj *query,
                                 const BSONObj *fieldSelector,
                                 const BSONObj *orderBy,
                                 const BSONObj *hint,
                                 engine::IExecutor *cb = NULL ) ;


INT32 msgExtractSql( CHAR *pBuffer, CHAR **sql ) ;

// cluster manager
INT32 msgBuildCMRequest ( CHAR **ppBuffer, INT32 *pBufferSize, SINT32 remoCode,
                          const BSONObj *arg1 = NULL,
                          const BSONObj *arg2 = NULL,
                          const BSONObj *arg3 = NULL,
                          const BSONObj *arg4 = NULL,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgExtractCMRequest ( CHAR *pBuffer, SINT32 *remoCode,
                            CHAR **arg1, CHAR **arg2,
                            CHAR **arg3, CHAR **arg4 ) ;

INT32 msgBuildQueryCMDMsg ( CHAR ** ppBuffer,
                            INT32 * pBufferSize,
                            const CHAR * commandName,
                            const BSONObj & boQuery,
                            const BSONObj & boSelect,
                            const BSONObj & boSort,
                            const BSONObj & boHint,
                            UINT64 reqID,
                            engine::IExecutor * cb ) ;

INT32 msgBuildDropCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, UINT64 reqID,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgBuildLinkCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName,
                          const CHAR *subCollectionName,
                          const BSONObj *lowBound, const BSONObj *upBound,
                          UINT64 reqID,
                          engine::IExecutor *cb = NULL ) ;

INT32 msgBuildUnlinkCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                            const CHAR *CollectionName,
                            const CHAR *subCollectionName,
                            UINT64 reqID,
                            engine::IExecutor *cb = NULL ) ;

INT32 msgBuildDropIndexMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                             const CHAR *CollectionName,
                             const CHAR *IndexName,
                             UINT64 reqID,
                             engine::IExecutor *cb = NULL ) ;

INT32 msgBuildSysInfoRequest ( CHAR **ppBuffer, INT32 *pBufferSize,
                               engine::IExecutor *cb = NULL ) ;

INT32 msgExtractSysInfoRequest ( CHAR *pBuffer, BOOLEAN &endianConvert ) ;

INT32 msgBuildSysInfoReply ( CHAR **ppBuffer, INT32 *pBufferSize,
                             engine::IExecutor *cb = NULL ) ;

INT32 msgExtractSysInfoReply ( CHAR *pBuffer, BOOLEAN &endianConvert,
                               INT32 *osType ) ;

INT32 msgBuildSequenceAcquireMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  UINT64 reqID, const BSONObj& options,
                                  engine::IExecutor *cb = NULL ) ;

INT32 msgBuildSequenceCreateMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                 UINT64 reqID, const BSONObj& options,
                                 engine::IExecutor *cb = NULL ) ;

INT32 msgBuildSequenceDropMsg( CHAR **ppBuffer, INT32 *bufferSize,
                               UINT64 reqID, const BSONObj& options,
                               engine::IExecutor *cb = NULL ) ;

INT32 msgBuildSequenceAlterMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                UINT64 reqID, const BSONObj& options,
                                engine::IExecutor *cb = NULL ) ;

INT32 msgBuildSequenceInvalidateCacheMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                          const BSONObj &boQuery, UINT64 reqID,
                                          engine::IExecutor *cb = NULL ) ;

INT32 msgExtractSequenceRequestMsg( CHAR *pBuffer, BSONObj& options ) ;

INT32 msgExtractSequenceAcquireReply( CHAR *pBuffer, BSONObj& options ) ;

INT32 msgBuildTransCommitPreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                  engine::IExecutor *cb = NULL );

INT32 msgBuildTransCommitMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                               engine::IExecutor *cb = NULL ) ;

INT32 msgBuildTransRollbackMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                 engine::IExecutor *cb = NULL ) ;

INT32 msgExtractAggrRequest ( CHAR *pBuffer, CHAR **ppCollectionName,
                              CHAR **ppObjs, INT32 &count,
                              INT32 *pFlags = NULL );

INT32 msgExtractLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                            bson::BSONObj &meta, const MsgLobTuple **tuples,
                            UINT32 *tuplesSize ) ;

INT32 msgExtractTuples( const MsgLobTuple **begin,
                        UINT32 *tuplesSize,
                        const MsgLobTuple **tuple,
                        BOOLEAN *got ) ;

INT32 msgExtractTuplesAndData( const MsgLobTuple **begin, UINT32 *tuplesSize,
                               const MsgLobTuple **tuple, const CHAR **data,
                               BOOLEAN *got ) ;

INT32 msgExtractOpenLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                bson::BSONObj &meta ) ;

INT32 msgExtractWriteLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                 UINT32 *len, SINT64 *offset, const CHAR **data ) ;

INT32 msgExtractReadLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                UINT32 *len, SINT64 *offset ) ;

INT32 msgExtractLockLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                INT64 *offset, INT64 *len ) ;

INT32 msgExtractGetLobRTDetailRequest( const CHAR *pBuffer,
                                       const MsgOpLob **header ) ;


INT32 msgExtractCloseLobRequest( const CHAR *pBuffer, const MsgOpLob **header ) ;

INT32 msgExtractRemoveLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                  BSONObj &obj ) ;
INT32 msgExtractTruncateLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                    BSONObj &obj ) ;
INT32 msgExtractCreateLobIDRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                    BSONObj &obj ) ;


INT32 msgExtractReadResult( const MsgOpReply *header,
                            const MsgLobTuple **begin,
                            UINT32 *tupleSz ) ;

#endif // MSGMESSAGE_HPP_

