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

   Source File Name = msgMessage.cpp

   Descriptive Name = Messaging Client

   When/how to use: this program may be used on binary and text-formatted
   versions of Messaging component. This file contains functions for building
   and extracting data for client-server communication.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "core.hpp"
#include "msgMessage.hpp"
#include "pd.hpp"
#include "ossMem.hpp"
#include "ossUtil.hpp"
#include "msgDef.h"
#include "pdTrace.hpp"
#include "msgTrace.hpp"
#include <stddef.h>

using namespace engine ;
using namespace bson ;

#define MSG_CHECK_BSON_LENGTH( x )                                  \
   do {                                                             \
      if ( ossRoundUpToMultipleX( x, 4 ) < 8 )                      \
      {                                                             \
         PD_LOG( PDERROR, "Invalid bson length %d", x ) ;           \
         SDB_ASSERT( FALSE, "Msg is invalid" ) ;                    \
         rc = SDB_INVALIDARG ;                                      \
         goto error ;                                               \
      }                                                             \
   } while ( FALSE )

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGCHKBUFF, "msgCheckBuffer" )
INT32 msgCheckBuffer ( CHAR **ppBuffer,
                       INT32 *bufferSize,
                       INT32 packetLength,
                       IExecutor *cb )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGCHKBUFF ) ;
   PD_TRACE2 ( SDB_MSGCHKBUFF, PD_PACK_INT(*bufferSize),
               PD_PACK_INT(packetLength) ) ;

   if ( packetLength > *bufferSize )
   {
      INT32 newSize = ossRoundUpToMultipleX ( packetLength, SDB_PAGE_SIZE ) ;
      if ( newSize < 0 )
      {
         PD_LOG ( PDERROR, "new buffer overflow" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( cb )
      {
         rc = cb->reallocBuff( newSize, ppBuffer, (UINT32*)bufferSize ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to allocate %d bytes buffer, "
                      "rc: %d", newSize, rc ) ;
      }
      else
      {
         CHAR *pOrigMem = *ppBuffer ;
         *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer, newSize ) ;
         if ( !*ppBuffer )
         {
            PD_LOG ( PDERROR, "Failed to allocate %d bytes buffer", newSize ) ;
            rc = SDB_OOM ;
            *ppBuffer = pOrigMem ;
            goto error ;
         }
         *bufferSize = newSize ;
      }
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGCHKBUFF, rc ) ;
   return rc ;
error :
   goto done ;
}

void msgReleaseBuffer( CHAR *pBuffer,
                       IExecutor *cb )
{
   if ( cb )
   {
      cb->releaseBuff( pBuffer ) ;
   }
   else if ( pBuffer )
   {
      SDB_OSS_FREE( pBuffer ) ;
   }
}

BOOLEAN msgIsInnerOpReply( MsgHeader *pMsg )
{
   if ( pMsg->messageLength < (SINT32)sizeof( MsgOpReply ) )
   {
      return FALSE ;
   }
   MsgOpReply *pReply = ( MsgOpReply* )pMsg ;
   if ( -1 != pReply->contextID  )
   {
      return FALSE ;
   }
   return TRUE ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDUPMSG, "msgBuildUpdateMsg" )
INT32 msgBuildUpdateMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName,
                          SINT32 flag, UINT64 reqID,
                          const BSONObj *selector,
                          const BSONObj *updator,
                          const BSONObj *hint,
                          IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDUPMSG );
   PD_TRACE1 ( SDB_MSGBLDUPMSG, PD_PACK_STRING(CollectionName) );
   const BSONObj emptyObj ;

   if ( !selector )
   {
      selector = &emptyObj ;
   }
   if ( !updator )
   {
      updator = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                selector && updator && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpUpdate *pUpdate = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpUpdate, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( selector->objsize(), 4 ) +
                        ossRoundUpToMultipleX( updator->objsize (), 4 ) +
                        ossRoundUpToMultipleX( hint->objsize (), 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDUPMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pUpdate                       = (MsgOpUpdate*)(*ppBuffer) ;
   pUpdate->version              = 1 ;
   pUpdate->w                    = 0 ;
   pUpdate->flags                = flag ;
   pUpdate->nameLength           = ossStrlen ( CollectionName ) ;
   pUpdate->header.requestID     = reqID ;
   pUpdate->header.opCode        = MSG_BS_UPDATE_REQ ;
   pUpdate->header.messageLength = packetLength ;
   pUpdate->header.routeID.value = 0 ;
   pUpdate->header.TID           = ossGetCurrentThreadID() ;
   ossStrncpy ( pUpdate->name, CollectionName, pUpdate->nameLength ) ;
   pUpdate->name[pUpdate->nameLength] = 0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpUpdate, name) +
                                   pUpdate->nameLength + 1,
                                   4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), selector->objdata(),
                                       selector->objsize());
   offset += ossRoundUpToMultipleX( selector->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), updator->objdata(), updator->objsize());
   offset += ossRoundUpToMultipleX( updator->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(), hint->objsize());
   offset += ossRoundUpToMultipleX( hint->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDUPMSG, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTUP, "msgExtractUpdate" )
INT32 msgExtractUpdate ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppSelector, CHAR **ppUpdator, CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppSelector &&
                ppUpdator && ppHint, "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTUP );
   INT32 offset = 0 ;
   INT32 length = 0 ; // the length of bson object
   MsgOpUpdate *pUpdate = (MsgOpUpdate*)pBuffer ;
   *pflag = pUpdate->flags ;
   *ppCollectionName = pUpdate->name ;
   PD_TRACE1 ( SDB_MSGEXTRACTUP, PD_PACK_STRING(*ppCollectionName) );
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pUpdate->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   offset = ossRoundUpToMultipleX ( offsetof(MsgOpUpdate, name) +
                                    pUpdate->nameLength + 1, 4 ) ;
   if ( ppSelector )
   {
      *ppSelector = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;
   if ( offset + length > pUpdate->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( ppUpdator )
   {
      *ppUpdator  = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;
   if ( offset + length > pUpdate->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( ppHint )
   {
      *ppHint  = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;
   if ( offset + length > pUpdate->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTUP, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDINSERTMSG, "msgBuildInsertMsg" )
INT32 msgBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          const BSONObj *insertor,
                          IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                insertor, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDINSERTMSG );
   PD_TRACE1 ( SDB_MSGBLDINSERTMSG, PD_PACK_STRING(CollectionName) );
   MsgOpInsert *pInsert = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpInsert, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( insertor->objsize(), 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDINSERTMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pInsert                       = (MsgOpInsert*)(*ppBuffer) ;
   pInsert->version              = 1 ;
   pInsert->w                    = 0 ;
   pInsert->flags                = flag ;
   pInsert->nameLength           = ossStrlen ( CollectionName ) ;
   pInsert->header.requestID     = reqID ;
   pInsert->header.opCode        = MSG_BS_INSERT_REQ ;
   pInsert->header.messageLength = packetLength ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.TID           = ossGetCurrentThreadID() ;
   ossStrncpy ( pInsert->name, CollectionName, pInsert->nameLength ) ;
   pInsert->name[pInsert->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpInsert, name) +
                                   pInsert->nameLength + 1,
                                   4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), insertor->objdata(), insertor->objsize());
   offset += ossRoundUpToMultipleX( insertor->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDINSERTMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDINSERTMSG2, "msgBuildInsertMsg" )
INT32 msgBuildInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          void *pFiller, std::vector< CHAR * > &ObjQueue,
                          netIOVec &ioVec,
                          IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName && pFiller,
                "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDINSERTMSG2 );
   PD_TRACE1 ( SDB_MSGBLDINSERTMSG2, PD_PACK_STRING(CollectionName) );
   MsgOpInsert *pInsert = NULL ;
   INT32 headLength = ossRoundUpToMultipleX(offsetof(MsgOpInsert, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 );
   INT32 packetLength = headLength ;
   UINT32 i = 0 ;
   ioVec.clear() ;
   netIOV ioHead ;
   ioHead.iovBase = NULL ; // we will fill the address after malloc;
   ioHead.iovLen = headLength - sizeof( MsgHeader );
   ioVec.push_back( ioHead ) ;
   for ( ; i < ObjQueue.size(); i++ )
   {
      try
      {
         BSONObj boInsertor( ObjQueue[i] ) ;
         netIOV ioObj;
         ioObj.iovBase = ObjQueue[i];
         ioObj.iovLen = boInsertor.objsize();
         ioVec.push_back( ioObj ) ;

         INT32 usedLength = ossRoundUpToMultipleX( boInsertor.objsize(), 4 );
         INT32 fillLength = usedLength - boInsertor.objsize();
         if ( fillLength > 0 )
         {
            netIOV ioFiller;
            ioFiller.iovBase = pFiller;
            ioFiller.iovLen = fillLength;
            ioVec.push_back( ioFiller );
         }
         packetLength += usedLength ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                  "failed to parse the obj:%s", e.what() );
      }
   }
   PD_TRACE1 ( SDB_MSGBLDINSERTMSG2, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, headLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pInsert                       = (MsgOpInsert*)(*ppBuffer) ;
   pInsert->version              = 1 ;
   pInsert->w                    = 0 ;
   pInsert->flags                = flag ;
   pInsert->nameLength           = ossStrlen ( CollectionName ) ;
   pInsert->header.requestID     = reqID ;
   pInsert->header.opCode        = MSG_BS_INSERT_REQ ;
   pInsert->header.messageLength = packetLength ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.TID           = ossGetCurrentThreadID() ;
   ossStrncpy ( pInsert->name, CollectionName, pInsert->nameLength ) ;
   pInsert->name[pInsert->nameLength]=0 ;
   ioVec[0].iovBase = (void *)((CHAR *)pInsert + sizeof( MsgHeader ) ) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDINSERTMSG2, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGAPDINSERTMSG, "msgAppendInsertMsg" )
INT32 msgAppendInsertMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                           const BSONObj *insertor,
                           IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize &&
                insertor, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGAPDINSERTMSG );
   MsgOpInsert *pInsert = (MsgOpInsert*)(*ppBuffer) ;
   SDB_ASSERT ( pInsert->header.messageLength &&
                MSG_BS_INSERT_REQ ==  pInsert->header.opCode &&
                ossIsAligned4 ( pInsert->header.messageLength ),
                "Invalid messageLength" ) ;
   INT32 offset         = pInsert->header.messageLength ;
   INT32 packetLength   = offset +
                        ossRoundUpToMultipleX( insertor->objsize(), 4 ) ;
   PD_TRACE1 ( SDB_MSGAPDINSERTMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pInsert->header.messageLength = packetLength ;
   ossMemcpy ( &((*ppBuffer)[offset]), insertor->objdata(), insertor->objsize());
   offset += ossRoundUpToMultipleX( insertor->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGAPDINSERTMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTINSERT, "msgExtractInsert" )
INT32 msgExtractInsert ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppInsertor, INT32 &count )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppInsertor,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTINSERT );
   INT32 offset = 0 ;
   INT32 size = 0 ;
   CHAR *pCurrent = NULL ;
   MsgOpInsert *pInsert = (MsgOpInsert*)pBuffer ;
   *pflag = pInsert->flags ;
   *ppCollectionName = pInsert->name ;
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pInsert->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   offset = ossRoundUpToMultipleX ( offsetof(MsgOpInsert, name) +
                                    pInsert->nameLength + 1, 4 ) ;
   *ppInsertor = &pBuffer[offset] ;
   pCurrent = &pBuffer[offset] ;
   count = 0 ;
   while ( TRUE )
   {
      if ( ossRoundUpToMultipleX( offset, 4 ) >= pInsert->header.messageLength )
      {
         break ;
      }
      size = ossRoundUpToMultipleX ( *((SINT32*)pCurrent), 4 ) ;
      if ( size < 8 )
      {
         PD_LOG( PDERROR, "Insert msg is invalid, msg length: %d, offset: %d, "
                 "current: %d, count: %d", pInsert->header.messageLength,
                 offset, *((SINT32*)pCurrent), count ) ;
         SDB_ASSERT( FALSE, "Insert msg is invalid" ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ++count ;
      offset += size ;
      pCurrent = &pBuffer[offset] ;
   }
   PD_TRACE2 ( SDB_MSGEXTRACTINSERT, PD_PACK_STRING(*ppCollectionName),
                                     PD_PACK_INT(count) );
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTINSERT, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDQRYMSG, "msgBuildQueryMsg" )
INT32 msgBuildQueryMsg  ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          SINT64 numToSkip, SINT64 numToReturn,
                          const BSONObj *query,
                          const BSONObj *fieldSelector,
                          const BSONObj *orderBy,
                          const BSONObj *hint,
                          IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDQRYMSG ) ;
   PD_TRACE1 ( SDB_MSGBLDQRYMSG, PD_PACK_STRING(CollectionName) ) ;
   const BSONObj emptyObj ;

   if ( !query )
   {
      query = &emptyObj ;
   }
   if ( !fieldSelector )
   {
      fieldSelector = &emptyObj ;
   }
   if ( !orderBy )
   {
      orderBy = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                query && fieldSelector && orderBy && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpQuery, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( query->objsize(), 4 ) +
                        ossRoundUpToMultipleX( fieldSelector->objsize(), 4) +
                        ossRoundUpToMultipleX( orderBy->objsize(), 4 ) +
                        ossRoundUpToMultipleX( hint->objsize(), 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDQRYMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = flag ;
   pQuery->nameLength            = ossStrlen ( CollectionName ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = numToSkip ;
   pQuery->numToReturn           = numToReturn ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossStrncpy ( pQuery->name, CollectionName, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   pQuery->nameLength + 1,
                                   4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), query->objdata(), query->objsize() ) ;
   offset += ossRoundUpToMultipleX( query->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), fieldSelector->objdata(),
               fieldSelector->objsize() ) ;
   offset += ossRoundUpToMultipleX( fieldSelector->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), orderBy->objdata(),
               orderBy->objsize() ) ;
   offset += ossRoundUpToMultipleX( orderBy->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(),
               hint->objsize() ) ;
   offset += ossRoundUpToMultipleX( hint->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDQRYMSG, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTQUERY, "msgExtractQuery" )
INT32 msgExtractQuery  ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         SINT64 *numToSkip, SINT64 *numToReturn,
                         CHAR **ppQuery, CHAR **ppFieldSelector,
                         CHAR **ppOrderBy, CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer, "Invalid input" ) ;

   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTQUERY );
   INT32 offset = 0 ;
   INT32 length = 0 ;
   MsgOpQuery *pQuery = (MsgOpQuery*)pBuffer ;

   if ( pflag )
   {
      *pflag = pQuery->flags ;
   }
   if ( ppCollectionName )
   {
      *ppCollectionName = pQuery->name ;
   }
   if ( numToSkip )
   {
      *numToSkip = pQuery->numToSkip ;
   }
   if ( numToReturn )
   {
      *numToReturn = pQuery->numToReturn ;
   }
   PD_TRACE3 ( SDB_MSGEXTRACTQUERY, PD_PACK_STRING(pQuery->name),
               PD_PACK_LONG(pQuery->numToSkip),
               PD_PACK_LONG(pQuery->numToReturn) );
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( pQuery->name ) ==
                            pQuery->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   offset = ossAlign4 ( (UINT32)(offsetof(MsgOpQuery, name) +
                                 pQuery->nameLength + 1) ) ;
   if ( ppQuery )
   {
      *ppQuery = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   if ( offset + length > pQuery->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossAlign4( (UINT32)length ) ;
   if ( offset < pQuery->header.messageLength )
   {
      if ( ppFieldSelector )
      {
         *ppFieldSelector = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length > pQuery->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossAlign4( (UINT32)length ) ;
   if ( offset < pQuery->header.messageLength )
   {
      if ( ppOrderBy )
      {
         *ppOrderBy = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length > pQuery->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossAlign4( (UINT32)length ) ;
   if ( offset < pQuery->header.messageLength )
   {
      if ( ppHint )
      {
         *ppHint = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length > pQuery->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTQUERY, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDGETMOREMSG, "msgBuildGetMoreMsg" )
INT32 msgBuildGetMoreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                           SINT32 numToReturn,
                           SINT64 contextID, UINT64 reqID,
                           IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize, "Invalid input" ) ;
   INT32 rc               = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDGETMOREMSG );
   PD_TRACE2 ( SDB_MSGBLDGETMOREMSG, PD_PACK_INT(numToReturn),
                                     PD_PACK_LONG(contextID) );
   MsgOpGetMore *pGetMore = NULL ;
   INT32 packetLength = sizeof(MsgOpGetMore);
   PD_TRACE1 ( SDB_MSGBLDGETMOREMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pGetMore                       = (MsgOpGetMore*)(*ppBuffer) ;
   pGetMore->header.requestID     = reqID ;
   pGetMore->header.opCode        = MSG_BS_GETMORE_REQ ;
   pGetMore->numToReturn          = numToReturn ;
   pGetMore->contextID            = contextID ;
   pGetMore->header.messageLength = packetLength ;
   pGetMore->header.routeID.value = 0 ;
   pGetMore->header.TID           = ossGetCurrentThreadID() ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDGETMOREMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTGETMORE, "msgExtractGetMore" )
INT32 msgExtractGetMore  ( CHAR *pBuffer,
                           SINT32 *numToReturn, SINT64 *contextID )
{
   SDB_ASSERT ( pBuffer &&
                numToReturn && contextID ,
                "Invalid input" ) ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTGETMORE );
   MsgOpGetMore *pGetMore = (MsgOpGetMore*)pBuffer ;
   *numToReturn = pGetMore->numToReturn ;
   *contextID = pGetMore->contextID ;
   PD_TRACE_EXIT ( SDB_MSGEXTRACTGETMORE );
   return SDB_OK ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGFILLGETMOREMSG, "msgFillGetMoreMsg" )
void msgFillGetMoreMsg ( MsgOpGetMore &getMoreMsg, const UINT32 tid,
                        const SINT64 contextID, const SINT32 numToReturn,
                        const UINT64 reqID )
{
   PD_TRACE_ENTRY ( SDB_MSGFILLGETMOREMSG );
   getMoreMsg.header.messageLength = sizeof( MsgOpGetMore );
   getMoreMsg.header.opCode = MSG_BS_GETMORE_REQ;
   getMoreMsg.header.TID = tid;
   getMoreMsg.header.routeID.value = 0;
   getMoreMsg.header.requestID = reqID;
   getMoreMsg.contextID = contextID;
   getMoreMsg.numToReturn = numToReturn;
   PD_TRACE_EXIT ( SDB_MSGFILLGETMOREMSG );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDELMSG, "msgBuildDeleteMsg" )
INT32 msgBuildDeleteMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, SINT32 flag, UINT64 reqID,
                          const BSONObj *deletor,
                          const BSONObj *hint,
                          engine::IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDDELMSG );
   PD_TRACE1 ( SDB_MSGBLDDELMSG, PD_PACK_STRING(CollectionName) );
   const BSONObj emptyObj ;
   if ( !deletor )
   {
      deletor = &emptyObj ;
   }
   if ( !hint )
   {
      hint = &emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                deletor && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpDelete *pDelete = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossRoundUpToMultipleX(offsetof(MsgOpDelete, name) +
                                              ossStrlen ( CollectionName ) + 1,
                                              4 ) +
                        ossRoundUpToMultipleX( deletor->objsize(), 4 ) +
                        ossRoundUpToMultipleX( hint->objsize(), 4 ) ;
   PD_TRACE1( SDB_MSGBLDDELMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pDelete                       = (MsgOpDelete*)(*ppBuffer) ;
   pDelete->version              = 1 ;
   pDelete->w                    = 0 ;
   pDelete->flags                = flag ;
   pDelete->nameLength           = ossStrlen ( CollectionName ) ;
   pDelete->header.requestID     = reqID ;
   pDelete->header.opCode        = MSG_BS_DELETE_REQ ;
   pDelete->header.messageLength = packetLength ;
   pDelete->header.routeID.value = 0 ;
   pDelete->header.TID           = ossGetCurrentThreadID() ;
   ossStrncpy ( pDelete->name, CollectionName, pDelete->nameLength ) ;
   pDelete->name[pDelete->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpDelete, name) +
                                   pDelete->nameLength + 1,
                                   4 ) ;

   ossMemcpy ( &((*ppBuffer)[offset]), deletor->objdata(), deletor->objsize() );
   offset += ossRoundUpToMultipleX( deletor->objsize(), 4 ) ;

   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(), hint->objsize() );
   offset += ossRoundUpToMultipleX( hint->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDELMSG, rc );
   return rc ;
error :
   goto done ;

}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTDEL, "msgExtractDelete" )
INT32 msgExtractDelete ( CHAR *pBuffer, INT32 *pflag, CHAR **ppCollectionName,
                         CHAR **ppDeletor, CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppDeletor && ppHint,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTDEL );
   INT32 offset = 0 ;
   INT32 length = 0 ;
   MsgOpDelete *pDelete = (MsgOpDelete*)pBuffer ;
   *pflag = pDelete->flags ;
   *ppCollectionName = pDelete->name ;
   PD_TRACE1 ( SDB_MSGEXTRACTDEL, PD_PACK_STRING(*ppCollectionName) );
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pDelete->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   offset = ossRoundUpToMultipleX ( offsetof(MsgOpDelete, name) +
                                    pDelete->nameLength + 1, 4 ) ;
   if ( ppDeletor )
   {
      *ppDeletor = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;
   if ( offset + length > pDelete->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( offset < pDelete->header.messageLength )
   {
      if ( ppHint )
      {
         *ppHint = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length >
           pDelete->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTDEL, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDKILLCONTXMSG, "msgBuildKillContextsMsg" )
INT32 msgBuildKillContextsMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                               UINT64 reqID,
                               SINT32 numContexts, SINT64 *pContextIDs,
                               engine::IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && pContextIDs, "Invalid input" ) ;
   SDB_ASSERT ( numContexts > 0, "Invalid input" ) ;
   INT32 rc              = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDKILLCONTXMSG );
   MsgOpKillContexts *pKC = NULL ;
   INT32 packetLength = offsetof(MsgOpKillContexts, contextIDs) +
                        sizeof ( SINT64 ) * (numContexts) ;
   PD_TRACE1 ( SDB_MSGBLDKILLCONTXMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pKC                       = (MsgOpKillContexts*)(*ppBuffer) ;
   pKC->header.requestID     = reqID ;
   pKC->header.opCode        = MSG_BS_KILL_CONTEXT_REQ ;
   pKC->header.messageLength = packetLength ;
   pKC->numContexts          = numContexts ;
   pKC->header.routeID.value = 0 ;
   pKC->header.TID           = ossGetCurrentThreadID() ;
   ossMemcpy ( (CHAR*)(&pKC->contextIDs[0]), (CHAR*)pContextIDs,
               sizeof(SINT64)*pKC->numContexts ) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDKILLCONTXMSG, rc );
   return rc ;
error :
   goto done ;

}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTKILLCONTX, "msgExtractKillContexts" )
INT32 msgExtractKillContexts ( CHAR *pBuffer,
                              SINT32 *numContexts, SINT64 **ppContextIDs )
{
   SDB_ASSERT ( pBuffer && numContexts && ppContextIDs
                , "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTKILLCONTX );
   MsgOpKillContexts *pKC = (MsgOpKillContexts*)pBuffer ;
   *numContexts = pKC->numContexts ;
   *ppContextIDs = &pKC->contextIDs[0] ;
   if ( offsetof(MsgOpKillContexts, contextIDs) + pKC->numContexts *
        sizeof ( SINT64 ) != (UINT32)pKC->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTKILLCONTX, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDMSGMSG, "msgBuildMsgMsg" )
INT32 msgBuildMsgMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                       UINT64 reqID, CHAR *pMsgStr,
                       IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && pMsgStr, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDMSGMSG );
   MsgOpMsg *pMsg       = NULL ;
   INT32 msgLen         = ossStrlen ( pMsgStr ) ;
   INT32 packetLength = ossRoundUpToMultipleX (
                          offsetof(MsgOpMsg, msg)+
                          msgLen + 1, 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDMSGMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pMsg                       = (MsgOpMsg*)(*ppBuffer) ;
   pMsg->header.requestID     = reqID ;
   pMsg->header.opCode        = MSG_BS_MSG_REQ ;
   pMsg->header.messageLength = packetLength ;
   pMsg->header.routeID.value = 0 ;
   pMsg->header.TID           = ossGetCurrentThreadID() ;
   ossStrncpy ( pMsg->msg, pMsgStr, msgLen ) ;
   pMsg->msg[msgLen] = 0 ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDMSGMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTMSG, "msgExtractMsg" )
INT32 msgExtractMsg ( CHAR *pBuffer, CHAR **ppMsgStr )
{
   SDB_ASSERT ( pBuffer && ppMsgStr, "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTMSG );
   MsgOpMsg *pMsg = (MsgOpMsg*)pBuffer ;
   *ppMsgStr = &pMsg->msg[0] ;
   if ( offsetof(MsgOpMsg, msg) + ossStrlen(*ppMsgStr) >
        (UINT32)pMsg->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDREPLYMSG, "msgBuildReplyMsg" )
INT32 msgBuildReplyMsg ( CHAR **ppBuffer, INT32 *bufferSize, INT32 opCode,
                         SINT32 flag, SINT64 contextID, SINT32 startFrom,
                         SINT32 numReturned, UINT64 reqID,
                         vector<BSONObj> *objList,
                         IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && objList, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDREPLYMSG );
   MsgOpReply *pReply = NULL ;
   INT32 packetLength = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   for ( UINT32 i = 0; i < objList->size(); i ++ )
   {
      INT32 objSize ;
      objSize = (*objList)[i].objsize() ;
      packetLength = ossRoundUpToMultipleX( packetLength, 4 ) ;
      rc = msgCheckBuffer ( ppBuffer, bufferSize,
                            packetLength + objSize,
                            cb ) ;
      if ( rc )
      {
         PD_LOG ( PDERROR, "Failed to check buffer" ) ;
         goto error ;
      }
      ossMemcpy ( &((*ppBuffer)[packetLength]), (*objList)[i].objdata(),
                                                objSize ) ;
      packetLength += objSize ;
   }
   PD_TRACE1 ( SDB_MSGBLDREPLYMSG, PD_PACK_INT(packetLength) );
   pReply                       = (MsgOpReply*)(*ppBuffer) ;
   pReply->flags                = flag ;
   pReply->contextID            = contextID ;
   pReply->startFrom            = startFrom ;
   pReply->numReturned          = numReturned ;
   pReply->header.requestID     = reqID ;
   pReply->header.opCode        = MAKE_REPLY_TYPE(opCode);
   pReply->header.messageLength = packetLength ;
   pReply->header.routeID.value = 0 ;
   pReply->header.TID           = ossGetCurrentThreadID() ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDREPLYMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDREPLYMSG2, "msgBuildReplyMsg" )
INT32 msgBuildReplyMsg ( CHAR **ppBuffer, INT32 *bufferSize, INT32 opCode,
                         SINT32 flag, SINT64 contextID, SINT32 startFrom,
                         SINT32 numReturned, UINT64 reqID,
                         const BSONObj *bsonobj,
                         IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize, "Invalid input" ) ;
   INT32 rc           = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDREPLYMSG2 );
   INT32 offset       = 0 ;
   MsgOpReply *pReply = NULL ;
   INT32 packetLength = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
   if ( numReturned != 0 )
   {
      packetLength += ossRoundUpToMultipleX ( bsonobj->objsize(), 4 ) ;
   }
   PD_TRACE1 ( SDB_MSGBLDREPLYMSG2, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer, rc: %d", rc ) ;
      goto error ;
   }
   pReply                       = (MsgOpReply*)(*ppBuffer) ;
   pReply->flags                = flag ;
   pReply->contextID            = contextID ;
   pReply->startFrom            = startFrom ;
   pReply->numReturned          = numReturned ;
   pReply->header.requestID     = reqID ;
   pReply->header.opCode        = MAKE_REPLY_TYPE(opCode);
   pReply->header.messageLength = packetLength ;
   pReply->header.routeID.value = 0 ;
   pReply->header.TID           = ossGetCurrentThreadID() ;
   if ( numReturned != 0 )
   {
      offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
      ossMemcpy ( &((*ppBuffer)[offset]), bsonobj->objdata(),
                                          bsonobj->objsize() ) ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDREPLYMSG2, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTREPLY, "msgExtractReply" )
INT32 msgExtractReply ( CHAR *pBuffer, SINT32 *flag, SINT64 *contextID,
                        SINT32 *startFrom, SINT32 *numReturned,
                        vector<BSONObj> &objList )
{
   SDB_ASSERT ( pBuffer && flag && contextID && startFrom &&
                numReturned , "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTREPLY );
   INT32 offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
   MsgOpReply *pReply = (MsgOpReply*)pBuffer ;
   *flag = pReply->flags ;
   *contextID = pReply->contextID ;
   *startFrom = pReply->startFrom ;
   *numReturned = pReply->numReturned ;

   try
   {
      for ( SINT32 i = 0; i < *numReturned; i++ )
      {
         if ( offset > pReply->header.messageLength )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         BSONObj obj(&pBuffer[offset]) ;
         SDB_ASSERT( obj.objsize() >= 5, "obj size must grater or equal 5" ) ;
         MSG_CHECK_BSON_LENGTH( obj.objsize() ) ;
         objList.push_back(obj) ;
         offset += ossRoundUpToMultipleX ( obj.objsize(), 4 ) ;
      }
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Extract reply msg occur exception: %s", e.what() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTREPLY, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDISCONNMSG, "msgBuildDisconnectMsg" )
INT32 msgBuildDisconnectMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                              UINT64 reqID,
                              IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize, "Invalid input" ) ;
   INT32 rc                     = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDDISCONNMSG );
   MsgOpDisconnect *pDisconnect = NULL ;
   INT32 packetLength = ossRoundUpToMultipleX ( sizeof ( MsgOpDisconnect ),4) ;
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pDisconnect                       = (MsgOpDisconnect*)(*ppBuffer) ;
   pDisconnect->header.requestID     = reqID ;
   pDisconnect->header.opCode        = MSG_BS_DISCONNECT ;
   pDisconnect->header.messageLength = packetLength ;
   pDisconnect->header.routeID.value = 0 ;
   pDisconnect->header.TID           = ossGetCurrentThreadID() ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDISCONNMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDREPLYMSGHD, "msgBuildReplyMsgHeader" )
void msgBuildReplyMsgHeader ( MsgOpReply &replyHeader, SINT32 packetLength,
                              INT32 opCode, SINT32 flag, SINT64 contextID,
                              SINT32 startFrom, SINT32 numReturned,
                              MsgRouteID &routeID, UINT64 reqID )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDREPLYMSGHD );
   replyHeader.flags                = flag ;
   replyHeader.contextID            = contextID ;
   replyHeader.startFrom            = startFrom ;
   replyHeader.numReturned          = numReturned ;
   replyHeader.header.routeID       = routeID ;
   replyHeader.header.requestID     = reqID ;
   replyHeader.header.opCode        = MAKE_REPLY_TYPE(opCode);
   replyHeader.header.messageLength = packetLength ;
   replyHeader.header.TID           = ossGetCurrentThreadID() ;
   PD_TRACE_EXIT ( SDB_MSGBLDREPLYMSGHD );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDISCONNMSG2, "msgBuildDisconnectMsg" )
void msgBuildDisconnectMsg ( MsgOpDisconnect &disconnectHeader,
                             MsgRouteID &routeID, UINT64 reqID )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDDISCONNMSG2 );
   disconnectHeader.header.requestID     = reqID ;
   disconnectHeader.header.routeID       = routeID ;
   disconnectHeader.header.opCode        = MSG_BS_DISCONNECT ;
   disconnectHeader.header.messageLength = sizeof(MsgOpDisconnect) ;
   disconnectHeader.header.TID           = ossGetCurrentThreadID() ;
   PD_TRACE_EXIT ( SDB_MSGBLDDISCONNMSG2 );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDQRYCATREQMSG, "msgBuildQueryCatalogReqMsg" )
INT32 msgBuildQueryCatalogReqMsg ( CHAR **ppBuffer, INT32 *pBufferSize,
                                   SINT32 flag, UINT64 reqID, SINT64 numToSkip,
                                   SINT64 numToReturn, UINT32 TID,
                                   const BSONObj *query,
                                   const BSONObj *fieldSelector,
                                   const BSONObj *orderBy,
                                   const BSONObj *hint,
                                   IExecutor *cb )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGBLDQRYCATREQMSG ) ;

   rc = msgBuildQueryMsg ( ppBuffer, pBufferSize, "", flag, reqID,
                           numToSkip, numToReturn, query, fieldSelector,
                           orderBy, hint, cb ) ;
   if ( SDB_OK == rc )
   {
      MsgHeader *pHead = (MsgHeader *)(*ppBuffer) ;
      pHead->opCode = MSG_CAT_QUERY_CATALOG_REQ ;
      pHead->routeID.value = 0 ;
      pHead->TID = TID ;
   }
   PD_TRACE_EXITRC ( SDB_MSGBLDQRYCATREQMSG, rc ) ;
   return rc ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDQRYSPCREQMSG, "msgBuildQuerySpaceReqMsg" )
INT32 msgBuildQuerySpaceReqMsg ( CHAR **ppBuffer, INT32 *pBufferSize,
                                 SINT32 flag, UINT64 reqID, SINT64 numToSkip,
                                 SINT64 numToReturn, UINT32 TID,
                                 const BSONObj *query,
                                 const BSONObj *fieldSelector,
                                 const BSONObj *orderBy,
                                 const BSONObj *hint,
                                 IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDQRYSPCREQMSG );
   INT32 rc = SDB_OK;
   rc = msgBuildQueryMsg ( ppBuffer, pBufferSize, "", flag, reqID,
                           numToSkip, numToReturn, query, fieldSelector,
                           orderBy, hint, cb ) ;
   if ( SDB_OK == rc )
   {
      MsgHeader *pHead = (MsgHeader *)(*ppBuffer) ;
      pHead->opCode = MSG_CAT_QUERY_SPACEINFO_REQ ;
      pHead->routeID.value = 0 ;
      pHead->TID = TID ;
   }
   PD_TRACE_EXITRC ( SDB_MSGBLDQRYSPCREQMSG, rc ) ;
   return rc ;
}

INT32 msgExtractQueryCatalogMsg ( CHAR *pBuffer, INT32 *pFlag, SINT64 *pNumToSkip,
                              SINT64 *pNumToReturn, CHAR **ppQuery,
                              CHAR **ppFieldSelector, CHAR **ppOrderBy, CHAR **ppHint )
{
   CHAR *pCmdName = NULL;
   return  msgExtractQuery( pBuffer, pFlag, &pCmdName, pNumToSkip, pNumToReturn,
                           ppQuery, ppFieldSelector, ppOrderBy, ppHint );
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_EXTRACTRC, "extractRC" )
INT32 extractRC ( BSONObj &obj )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_EXTRACTRC );
   BSONElement ele = obj.getField ( OP_ERRNOFIELD ) ;
   if ( ele.eoo() || ele.type() != NumberInt )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = ele.numberInt() ;
done :
   PD_TRACE_EXITRC ( SDB_EXTRACTRC, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION (SDB_MSGBLDCMREQ, "msgBuildCMRequest" )
INT32 msgBuildCMRequest ( CHAR **ppBuffer, INT32 *pBufferSize,
                          SINT32 remoCode,
                          const BSONObj *arg1,
                          const BSONObj *arg2,
                          const BSONObj *arg3,
                          const BSONObj *arg4,
                          IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDCMREQ );
   PD_TRACE1 ( SDB_MSGBLDCMREQ, PD_PACK_INT(remoCode) );
   const BSONObj emptyObj ;
   if ( !arg1 )
   {
      arg1 = &emptyObj ;
   }
   if ( !arg2 )
   {
      arg2 = &emptyObj ;
   }
   if ( !arg3 )
   {
      arg3 = &emptyObj ;
   }
   if ( !arg4 )
   {
      arg4 = &emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && pBufferSize, "Invalid input" ) ;
   INT32 rc                 = SDB_OK ;
   MsgCMRequest *pCMRequest = NULL ;
   INT32 offset             = ossRoundUpToMultipleX (
                                 offsetof ( MsgCMRequest, arguments ), 4 ) ;
   INT32 packetLength       = offset +
                              ossRoundUpToMultipleX ( arg1->objsize(), 4 ) +
                              ossRoundUpToMultipleX ( arg2->objsize(), 4 ) +
                              ossRoundUpToMultipleX ( arg3->objsize(), 4 ) +
                              ossRoundUpToMultipleX ( arg4->objsize(), 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDCMREQ, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pCMRequest = (MsgCMRequest*) (*ppBuffer) ;
   pCMRequest->header.messageLength = packetLength ;
   pCMRequest->header.requestID     = 0 ;
   pCMRequest->header.opCode        = MSG_CM_REMOTE ;
   pCMRequest->header.routeID.value = 0 ;
   pCMRequest->header.TID           = ossGetCurrentThreadID() ;
   pCMRequest->remoCode             = remoCode ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg1->objdata(), arg1->objsize() ) ;
   offset += ossRoundUpToMultipleX ( arg1->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg2->objdata(), arg2->objsize() ) ;
   offset += ossRoundUpToMultipleX ( arg2->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg3->objdata(), arg3->objsize() ) ;
   offset += ossRoundUpToMultipleX ( arg3->objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg4->objdata(), arg4->objsize() ) ;
   offset += ossRoundUpToMultipleX ( arg4->objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGBLDCMREQ, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTCMREQ, "msgExtractCMRequest" )
INT32 msgExtractCMRequest ( CHAR *pBuffer, SINT32 *remoCode,
                            CHAR **arg1, CHAR **arg2,
                            CHAR **arg3, CHAR **arg4 )
{
   SDB_ASSERT ( pBuffer && remoCode && arg1 && arg2 && arg3 && arg4,
                "Invalid input" ) ;
   INT32 rc           = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTCMREQ );
   INT32 length       = 0 ;
   INT32 offset       = ossRoundUpToMultipleX (
                        offsetof ( MsgCMRequest, arguments ), 4 ) ;
   MsgCMRequest *pCMRequest = (MsgCMRequest*) pBuffer ;
   *remoCode = pCMRequest->remoCode ;

   if ( arg1 )
   {
      *arg1 = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;
   if ( offset + length > pCMRequest->header.messageLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( offset < pCMRequest->header.messageLength )
   {
      if ( arg2 )
      {
         *arg2 = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length > pCMRequest->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( offset < pCMRequest->header.messageLength )
   {
      if ( arg3 )
      {
         *arg3 = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      MSG_CHECK_BSON_LENGTH( length ) ;
      if ( offset + length > pCMRequest->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   offset += ossRoundUpToMultipleX( length, 4 ) ;
   if ( offset < pCMRequest->header.messageLength )
   {
      if ( arg4 )
      {
         *arg4 = &pBuffer[offset] ;
      }
      length = *((SINT32*)(&pBuffer[offset])) ;
      if ( offset + length > pCMRequest->header.messageLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTCMREQ, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDROPCLMSG, "msgBuildDropCLMsg" )
INT32 msgBuildDropCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName, UINT64 reqID,
                          IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDDROPCLMSG );
   const bson::BSONObj emptyObj ;
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName,
                "Invalid input" ) ;
   PD_TRACE1 ( SDB_MSGBLDDROPCLMSG, PD_PACK_STRING(CollectionName) );
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   INT32 offset         = 0 ;
   bson::BSONObj boQuery;
   INT32 packetLength = 0;
   try
   {
      bson::BSONObjBuilder bobQuery;
      bobQuery.append( FIELD_NAME_NAME, CollectionName );
      boQuery = bobQuery.obj() ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build drop collection message failed, occurred unexpected error:%s",
               e.what() );
      goto error;
   }
   packetLength = ossRoundUpToMultipleX(offsetof(MsgOpQuery, name) +
                        ossStrlen ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ) + 1, 4 ) +
                  ossRoundUpToMultipleX( boQuery.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;

   PD_TRACE1 ( SDB_MSGBLDDROPCLMSG, PD_PACK_INT(packetLength) );

   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR,
              "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR,
              "Failed to check buffer" ) ;
      goto error ;
   }
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = 0 ;
   pQuery->nameLength            = ossStrlen( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = 0 ;
   pQuery->numToReturn           = 0 ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossStrncpy ( pQuery->name, CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   pQuery->nameLength + 1,
                                   4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), boQuery.objdata(), boQuery.objsize() ) ;
   offset += ossRoundUpToMultipleX( boQuery.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDROPCLMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDLINKCLMSG, "msgBuildLinkCLMsg" )
INT32 msgBuildLinkCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName,
                          const CHAR *subCollectionName,
                          const BSONObj *lowBound, const BSONObj *upBound,
                          UINT64 reqID,
                          IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY ( SDB_MSGBLDLINKCLMSG ) ;

   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName && subCollectionName,
                "Invalid input" ) ;

   PD_TRACE2 ( SDB_MSGBLDLINKCLMSG, PD_PACK_STRING(CollectionName),
                                    PD_PACK_STRING(subCollectionName) ) ;

   MsgOpQuery *pQuery = NULL ;
   INT32 offset = 0 ;
   bson::BSONObj boQuery ;
   const bson::BSONObj emptyObj ;
   INT32 packetLength = 0 ;

   if ( !lowBound )
   {
      lowBound = &emptyObj ;
   }
   if ( !upBound )
   {
      upBound = &emptyObj ;
   }

   try
   {
      bson::BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_NAME, CollectionName ) ;
      bobQuery.append( FIELD_NAME_SUBCLNAME, subCollectionName ) ;
      bobQuery.append( FIELD_NAME_LOWBOUND, *lowBound ) ;
      bobQuery.append( FIELD_NAME_UPBOUND, *upBound ) ;
      boQuery = bobQuery.obj() ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build link collection message failed, "
               "occurred unexpected error:%s",
               e.what() );
      goto error;
   }
   packetLength = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                        ossStrlen( CMD_ADMIN_PREFIX CMD_NAME_LINK_CL ) + 1, 4 ) +
                  ossRoundUpToMultipleX( boQuery.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;

   PD_TRACE1 ( SDB_MSGBLDLINKCLMSG, PD_PACK_INT(packetLength) );

   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR,
              "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = 0 ;
   pQuery->nameLength            = ossStrlen( CMD_ADMIN_PREFIX CMD_NAME_LINK_CL ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = 0 ;
   pQuery->numToReturn           = 0 ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossStrncpy ( pQuery->name, CMD_ADMIN_PREFIX CMD_NAME_LINK_CL, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   pQuery->nameLength + 1,
                                   4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), boQuery.objdata(), boQuery.objsize() ) ;
   offset += ossRoundUpToMultipleX( boQuery.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDLINKCLMSG, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDUNLINKCLMSG, "msgBuildUnlinkCLMsg" )
INT32 msgBuildUnlinkCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                            const CHAR *CollectionName,
                            const CHAR *subCollectionName,
                            UINT64 reqID,
                            IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY ( SDB_MSGBLDUNLINKCLMSG );

   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName && subCollectionName,
                "Invalid input" ) ;

   PD_TRACE2 ( SDB_MSGBLDUNLINKCLMSG, PD_PACK_STRING(CollectionName),
                                      PD_PACK_STRING(subCollectionName) ) ;

   MsgOpQuery *pQuery = NULL ;
   INT32 offset = 0 ;
   bson::BSONObj boQuery ;
   const bson::BSONObj emptyObj ;
   INT32 packetLength = 0 ;

   try
   {
      bson::BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_NAME, CollectionName ) ;
      bobQuery.append( FIELD_NAME_SUBCLNAME, subCollectionName ) ;
      boQuery = bobQuery.obj() ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build unlink collection message failed, occurred unexpected error:%s",
               e.what() );
      goto error;
   }
   packetLength = ossRoundUpToMultipleX(offsetof(MsgOpQuery, name) +
                        ossStrlen ( CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL ) + 1, 4 ) +
                  ossRoundUpToMultipleX( boQuery.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                  ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;

   PD_TRACE1 ( SDB_MSGBLDUNLINKCLMSG, PD_PACK_INT(packetLength) );

   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR,
              "Failed to check buffer" ) ;
      goto error ;
   }
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = 0 ;
   pQuery->nameLength            = ossStrlen( CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = 0 ;
   pQuery->numToReturn           = 0 ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossStrncpy ( pQuery->name, CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   pQuery->nameLength + 1,
                                   4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), boQuery.objdata(), boQuery.objsize() ) ;
   offset += ossRoundUpToMultipleX( boQuery.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy( &((*ppBuffer)[offset]), emptyObj.objdata(), emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDUNLINKCLMSG, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDROPINXMSG, "msgBuildDropIndexMsg" )
INT32 msgBuildDropIndexMsg  ( CHAR **ppBuffer, INT32 *bufferSize,
                              const CHAR *CollectionName,
                              const CHAR *IndexName,
                              UINT64 reqID,
                              IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDDROPINXMSG );
   const bson::BSONObj emptyObj ;
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName && IndexName,
                "Invalid input" ) ;
   PD_TRACE2 ( SDB_MSGBLDDROPINXMSG, PD_PACK_STRING(CollectionName),
                                     PD_PACK_STRING(IndexName) );
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   INT32 offset         = 0 ;
   bson::BSONObj boQuery;
   INT32 packetLength = 0;
   try
   {
      bson::BSONObjBuilder bobIndex;
      bobIndex.append( IXM_FIELD_NAME_NAME, IndexName );
      bson::BSONObj boIndex = bobIndex.obj();

      bson::BSONObjBuilder bobQuery;
      bobQuery.append( FIELD_NAME_COLLECTION, CollectionName );
      bobQuery.append( FIELD_NAME_INDEX, boIndex );
      boQuery = bobQuery.obj() ;
   }
   catch ( std::exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build dropindex message failed, occured unexpected error:%s",
               e.what() );
      goto error;
   }
   packetLength = ossRoundUpToMultipleX(offsetof(MsgOpQuery, name) +
                                 ossStrlen ( CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ) + 1,
                                 4 ) +
                        ossRoundUpToMultipleX( boQuery.objsize(), 4 ) +
                        ossRoundUpToMultipleX( emptyObj.objsize(), 4) +
                        ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) +
                        ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   PD_TRACE1 ( SDB_MSGBLDDROPINXMSG, PD_PACK_INT(packetLength) );
   if ( packetLength < 0 )
   {
      PD_LOG ( PDERROR, "Packet size overflow" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = msgCheckBuffer ( ppBuffer, bufferSize, packetLength, cb ) ;
   if ( rc )
   {
      PD_LOG ( PDERROR, "Failed to check buffer" ) ;
      goto error ;
   }
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = 0 ;
   pQuery->nameLength            = ossStrlen ( CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = 0 ;
   pQuery->numToReturn           = 0 ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossStrncpy ( pQuery->name, CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpQuery, name) +
                                   pQuery->nameLength + 1,
                                   4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), boQuery.objdata(), boQuery.objsize() ) ;
   offset += ossRoundUpToMultipleX( boQuery.objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), emptyObj.objdata(),
               emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), emptyObj.objdata(),
               emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), emptyObj.objdata(),
               emptyObj.objsize() ) ;
   offset += ossRoundUpToMultipleX( emptyObj.objsize(), 4 ) ;
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDROPINXMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTSQL, "msgExtractSql" )
INT32 msgExtractSql( CHAR *pBuffer, CHAR **sql )
{
   SDB_ASSERT( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSQL );
   MsgOpSql *msg = ( MsgOpSql * )pBuffer ;
   SDB_ASSERT( msg->header.messageLength > (INT32)sizeof(MsgHeader),
               "impossible" ) ;
   *sql = pBuffer + sizeof( MsgHeader ) ;
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTSQL, rc );
   return rc ;
}

INT32 msgBuildTransCommitPreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                  IExecutor *cb )
{
   SDB_ASSERT( ppBuffer, "invalid input" ) ;
   INT32 rc = SDB_OK;
   MsgOpTransCommitPre *pMsg = NULL;
   INT32 packetLength
            = ossRoundUpToMultipleX( sizeof( MsgOpTransCommitPre ), 4 );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransCommitPre *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.opCode = MSG_BS_TRANS_COMMITPRE_REQ;
   pMsg->header.routeID.value = 0;

done:
   return rc ;
error:
   goto done ;
}

INT32 msgBuildTransCommitMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                               IExecutor *cb )
{
   SDB_ASSERT( ppBuffer, "invalid input" ) ;
   INT32 rc = SDB_OK;
   MsgOpTransCommit *pMsg = NULL;
   INT32 packetLength
            = ossRoundUpToMultipleX( sizeof( MsgOpTransCommit ), 4 );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransCommit *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.opCode = MSG_BS_TRANS_COMMIT_REQ;
   pMsg->header.routeID.value = 0;

done:
   return rc;
error:
   goto done;
}

INT32 msgBuildTransRollbackMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                 IExecutor *cb )
{
   SDB_ASSERT( ppBuffer, "invalid input" ) ;
   INT32 rc = SDB_OK;
   MsgOpTransRollback *pMsg = NULL;
   INT32 packetLength
            = ossRoundUpToMultipleX( sizeof( MsgOpTransRollback ), 4 );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransRollback *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.opCode = MSG_BS_TRANS_ROLLBACK_REQ;
   pMsg->header.routeID.value = 0;

done:
   return rc;
error:
   goto done;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBUILDSYSINFOREQUEST, "msgBuildSysInfoRequest" )
INT32 msgBuildSysInfoRequest ( CHAR **ppBuffer, INT32 *pBufferSize,
                               IExecutor *cb )
{
   SDB_ASSERT( NULL != ppBuffer &&
               NULL != pBufferSize, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   MsgSysInfoRequest *request = NULL ;
   PD_TRACE_ENTRY ( SDB_MSGBUILDSYSINFOREQUEST ) ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize,
                         sizeof(MsgSysInfoRequest), cb ) ;
   PD_RC_CHECK ( rc, PDERROR, "Failed to check buffer" ) ;
   request                                   = (MsgSysInfoRequest*)(*ppBuffer) ;
   request->header.specialSysInfoLen         = MSG_SYSTEM_INFO_LEN ;
   request->header.eyeCatcher                = MSG_SYSTEM_INFO_EYECATCHER ;
   request->header.realMessageLength         = sizeof(MsgSysInfoRequest) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBUILDSYSINFOREQUEST, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTSYSINFOREQUEST, "msgExtractSysInfoRequest" )
INT32 msgExtractSysInfoRequest ( CHAR *pBuffer, BOOLEAN &endianConvert )
{
   SDB_ASSERT ( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSYSINFOREQUEST ) ;
   MsgSysInfoRequest *request = (MsgSysInfoRequest*)pBuffer ;
   SDB_ASSERT ( (UINT32)request->header.specialSysInfoLen ==
                MSG_SYSTEM_INFO_LEN,
                "Invalid len, expected -1" ) ;
   if ( request->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER )
   {
      endianConvert = FALSE ;
   }
   else if ( request->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER_REVERT )
   {
      endianConvert = TRUE ;
   }
   else
   {
      PD_RC_CHECK ( SDB_INVALIDARG, PDERROR,
                    "Invalid sys info request eyecatcher" ) ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTSYSINFOREQUEST, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBUILDSYSINFOREPLY, "msgBuildSysInfoReply" )
INT32 msgBuildSysInfoReply ( CHAR **ppBuffer, INT32 *pBufferSize,
                             IExecutor *cb )
{
   SDB_ASSERT ( NULL != ppBuffer &&
                NULL != pBufferSize, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   MsgSysInfoReply *reply = NULL ;
   PD_TRACE_ENTRY ( SDB_MSGBUILDSYSINFOREPLY ) ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, sizeof(MsgSysInfoReply), cb ) ;
   PD_RC_CHECK ( rc, PDERROR, "Failed to check buffer, rc = %d", rc ) ;
   reply                                     = (MsgSysInfoReply*)(*ppBuffer) ;
   reply->header.specialSysInfoLen           = MSG_SYSTEM_INFO_LEN ;
   reply->header.eyeCatcher                  = MSG_SYSTEM_INFO_EYECATCHER ;
   reply->header.realMessageLength           = sizeof(MsgSysInfoReply) ;
   reply->osType                             = OSS_OSTYPE ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBUILDSYSINFOREPLY, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTSYSINFOREPLY, "msgExtractSysInfoReply" )
INT32 msgExtractSysInfoReply ( CHAR *pBuffer, BOOLEAN &endianConvert,
                               INT32 *osType )
{
   SDB_ASSERT ( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSYSINFOREPLY ) ;
   MsgSysInfoReply *reply = (MsgSysInfoReply*)pBuffer ;
   SDB_ASSERT ( (UINT32)reply->header.specialSysInfoLen ==
                MSG_SYSTEM_INFO_LEN,
                "Invalid len, expected -1" ) ;

   if ( reply->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER )
   {
      endianConvert = FALSE ;
   }
   else if ( reply->header.eyeCatcher == MSG_SYSTEM_INFO_EYECATCHER_REVERT )
   {
      endianConvert = TRUE ;
   }
   else
   {
      PD_RC_CHECK ( SDB_INVALIDARG, PDERROR,
                    "Invalid sys info request eyecatcher" ) ;
   }
   if ( osType )
   {
      ossEndianConvertIf4(reply->osType, *osType, endianConvert ) ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTSYSINFOREPLY, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTAGGRREQ, "msgExtractAggrRequest" )
INT32 msgExtractAggrRequest ( CHAR *pBuffer, CHAR **ppCollectionName,
                              CHAR **ppObjs, INT32 &count,
                              INT32 *pFlags )
{
   INT32 rc = SDB_OK;
   INT32 offset = 0;
   INT32 msgLen = 0;
   INT32 num = 0;
   CHAR *pCur = NULL;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTAGGRREQ ) ;
   SDB_ASSERT( pBuffer && ppCollectionName && ppObjs, "Invalid input!" ) ;
   MsgOpAggregate *pAggr = (MsgOpAggregate *)pBuffer ;
   *ppCollectionName = pAggr->name ;
   if ( pFlags )
   {
      *pFlags = pAggr->flags ;
   }
   msgLen = pAggr->header.messageLength ;
   offset = ossRoundUpToMultipleX( offsetof(MsgOpAggregate, name) +
                                   pAggr->nameLength + 1, 4 );
   if ( ppObjs )
   {
      *ppObjs = &pBuffer[ offset ];
   }
   while( offset + (INT32)(sizeof(SINT32 *)) < msgLen )
   {
      pCur = &pBuffer[ offset ];
      INT32 size = ossRoundUpToMultipleX( *((SINT32 *)pCur), 4 );
      MSG_CHECK_BSON_LENGTH( size ) ;
      ++num;
      offset += size;
   }
   count = num ;
done:
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTAGGRREQ, rc ) ;
   return rc;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTLOBREQ, "msgExtractLobRequest" )
INT32 msgExtractLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                            bson::BSONObj &meta, const MsgLobTuple **tuples,
                            UINT32 *tuplesSize )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTLOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer, "can not be null" ) ;
   SDB_ASSERT( NULL != header, "can not be null" ) ;
   const MsgOpLob *msgHeader = ( const MsgOpLob * )pBuffer ;
   UINT32 msgLen = ( UINT32 )msgHeader->header.messageLength ;
   const CHAR *body = NULL ;
   UINT32 bsonLen = 0 ;

   if ( msgLen < sizeof( MsgOpLob ) )
   {
      PD_LOG( PDERROR, "invalid msg len:%d", msgLen ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   body = pBuffer + sizeof( MsgOpLob ) ;
   *header = msgHeader ;

   if ( 0 < msgHeader->bsonLen )
   {
      bsonLen = ossRoundUpToMultipleX( msgHeader->bsonLen, 4 ) ;
      if ( msgLen < ( sizeof( MsgOpLob ) + bsonLen ) )
      {
         PD_LOG( PDERROR, "invalid msg len:%d", msgLen ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         meta = BSONObj( body ) ;
         body += ossRoundUpToMultipleX( msgHeader->bsonLen, 4 ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }

   if ( NULL != tuples && NULL != tuplesSize &&
        sizeof( MsgOpLob ) + bsonLen < msgLen )
   {
      *tuples = ( MsgLobTuple * )body ;
      *tuplesSize = msgLen - sizeof( MsgOpLob ) - bsonLen ;
   }
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTLOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

INT32 msgExtractTuples( const MsgLobTuple **begin, UINT32 *tuplesSize,
                        const MsgLobTuple **tuple, BOOLEAN *got )
{
   INT32 rc = SDB_OK ;

   if ( NULL == *begin || 0 == *tuplesSize )
   {
      goto done ;
   }

   if ( sizeof( MsgLobTuple ) < *tuplesSize )
   {
      *tuple = *begin ;
      *tuplesSize -= sizeof( MsgLobTuple ) ;
      *begin += 1 ;
      *got = TRUE ;
   }
   else if ( sizeof( MsgLobTuple ) == *tuplesSize )
   {
      *tuple = *begin ;
      *tuplesSize -= sizeof( MsgLobTuple ) ;
      *begin = NULL ;
      *got = TRUE ;
   }
   else
   {
      PD_LOG( PDERROR, "invalid tuple size:%d", *tuplesSize ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 msgExtractTuplesAndData( const MsgLobTuple **begin, UINT32 *tuplesSize,
                               const MsgLobTuple **tuple, const CHAR **data,
                               BOOLEAN *got )
{
   INT32 rc = SDB_OK ;
   if ( NULL == *begin || 0 == *tuplesSize )
   {
      *got = FALSE ;
      goto done ;
   }

   if ( sizeof( MsgLobTuple ) <= *tuplesSize )
   {
      const MsgLobTuple *t = *begin ;
      UINT32 dataLen = t->columns.len ;
      if ( sizeof( MsgLobTuple ) + dataLen < *tuplesSize )
      {
         *tuple = *begin ;
         *tuplesSize -= sizeof( MsgLobTuple ) + dataLen ;
         *data = ( const CHAR * )( *begin ) + sizeof( MsgLobTuple ) ;
         *begin = ( const MsgLobTuple * )( *data + dataLen ) ;
         *got = TRUE ;
      }
      else if ( sizeof( MsgLobTuple ) + dataLen == *tuplesSize )
      {
         *tuple = *begin ;
         *tuplesSize -= sizeof( MsgLobTuple ) + dataLen ;
         *data = ( const CHAR * )( *begin ) + sizeof( MsgLobTuple ) ;
         *begin = NULL ;
         *got = TRUE ;
      }
      else
      {
         PD_LOG( PDERROR, "invalid tuple size:%d", *tuplesSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }
   }
   else
   {
      PD_LOG( PDERROR, "invalid tuple size:%d", *tuplesSize ) ;
      rc = SDB_SYS ;
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTOPENLOBREQ, "msgExtractOpenLobRequest" )
INT32 msgExtractOpenLobRequest( const CHAR *pBuffer,
                                const MsgOpLob **header,
                                BSONObj &lob )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTOPENLOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer, "can not be null" ) ;
   SDB_ASSERT( NULL != header, "can not be null" ) ;
   rc = msgExtractLobRequest( pBuffer, header, lob, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTOPENLOBREQ, rc ) ;
   return rc ;
error:
   *header = NULL ;
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTWRITELOBREQ, "msgExtractWriteLobRequest" )
INT32 msgExtractWriteLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                 UINT32 *len, SINT64 *offset, const CHAR **data )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTWRITELOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer && NULL != header &&
               NULL != len && NULL != offset &&
               NULL != data, "cat not be null" ) ;
   const MsgLobTuple *tuple = NULL ;
   const MsgLobTuple *tuples = NULL ;
   UINT32 size = 0 ;
   BOOLEAN got = FALSE ;
   BSONObj lob ;

   rc = msgExtractLobRequest( pBuffer, header, lob, &tuples, &size ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

   rc = msgExtractTuplesAndData( &tuples, &size,
                                 &tuple, data, &got ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract write msg:%d", rc ) ;
      goto error ;
   }

   if ( !got )
   {
      PD_LOG( PDERROR, "failed to extract write msg"
              ", we got nothing" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   *offset = tuple->columns.offset ;
   *len = tuple->columns.len ;
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTWRITELOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}


// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTREADLOBREQ, "msgExtractReadLobRequest" )
INT32 msgExtractReadLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                UINT32 *len, SINT64 *offset )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTREADLOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer && NULL != header &&
               NULL != len && NULL != offset, "cat not be null" ) ;

   const MsgLobTuple *tuple = NULL ;
   const MsgLobTuple *tuples = NULL ;
   UINT32 size = 0 ;
   BOOLEAN got = FALSE ;
   BSONObj lob ;

   rc = msgExtractLobRequest( pBuffer, header, lob, &tuples, &size ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

   rc = msgExtractTuples( &tuples, &size,
                              &tuple, &got ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract read msg:%d", rc ) ;
      goto error ;
   }

   if ( !got )
   {
      PD_LOG( PDERROR, "failed to extract read msg"
              ", we got nothing" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

   *offset = tuple->columns.offset ;
   *len = tuple->columns.len ;
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTREADLOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTLOCKLOBREQ, "msgExtractLockLobRequest" )
INT32 msgExtractLockLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                INT64 *offset, INT64 *len )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTLOCKLOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer && NULL != header &&
               NULL != len && NULL != offset, "cat not be null" ) ;

   BSONObj lob ;
   BSONElement ele ;

   rc = msgExtractLobRequest( pBuffer, header, lob, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

   ele = lob.getField( FIELD_NAME_LOB_OFFSET ) ;
   if ( NumberLong != ele.type() )
   {
      PD_LOG( PDERROR, "invalid offset type:%d", ele.type() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *offset = ele.Long() ;

   ele = lob.getField( FIELD_NAME_LOB_LENGTH ) ;
   if ( NumberLong != ele.type() )
   {
      PD_LOG( PDERROR, "invalid length type:%d", ele.type() ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *len = ele.Long() ;

done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTLOCKLOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTCLOSELOBREQ, "msgExtractCloseLobRequest" )
INT32 msgExtractCloseLobRequest( const CHAR *pBuffer, const MsgOpLob **header )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTCLOSELOBREQ ) ;
   SDB_ASSERT( NULL != pBuffer && NULL != header, "can not be null" ) ;
   BSONObj obj ;
   rc = msgExtractLobRequest( pBuffer, header, obj, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTCLOSELOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTREMOVELOBREQ, "msgExtractRemoveLobRequest" )
INT32 msgExtractRemoveLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                  BSONObj &obj )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTREMOVELOBREQ ) ;
   rc = msgExtractLobRequest( pBuffer, header, obj, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTREMOVELOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTTRUNCATELOBREQ, "msgExtractTruncateLobRequest" )
INT32 msgExtractTruncateLobRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                    BSONObj &obj )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTTRUNCATELOBREQ ) ;
   rc = msgExtractLobRequest( pBuffer, header, obj, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTTRUNCATELOBREQ, rc ) ;
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTREREADRESULT, "msgExtraceReadResult" )
INT32 msgExtractReadResult( const MsgOpReply *header,
                            const MsgLobTuple **begin,
                            UINT32 *tupleSz )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTREREADRESULT ) ;
   SDB_ASSERT( NULL != header, "can not be null" ) ;
   if ( ( (UINT32)header->header.messageLength ) <
        sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *begin = ( const MsgLobTuple * )
            (( const CHAR * )header + sizeof( MsgOpReply ) ) ;
   *tupleSz = (UINT32)header->header.messageLength -
              sizeof( MsgOpReply ) ;
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTREREADRESULT, rc ) ;
   return rc ;
error:
   goto done ;
}

