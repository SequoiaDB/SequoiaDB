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
#include "../bson/bsonobj.h"
#include "../bson/lib/md5.hpp"
#include <stddef.h>
#include "ossVer.hpp"

using namespace engine ;
using namespace bson ;
using namespace std ;

#define MSG_BSON_MIN_LEN      (8)

#define MSG_CHECK_BSON_LENGTH( x )                                  \
   do {                                                             \
      if ( ossAlign4( x ) < MSG_BSON_MIN_LEN )                                     \
      {                                                             \
         PD_LOG( PDERROR, "Invalid bson length %d", x ) ;           \
         SDB_ASSERT( FALSE, "Msg is invalid" ) ;                    \
         rc = SDB_INVALIDARG ;                                      \
         goto error ;                                               \
      }                                                             \
   } while ( FALSE )

const static BSONObj __emptyObj ;

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
      INT32 newSize = ossAlign4 ( packetLength ) ;
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
            // realloc does NOT free original memory if it fails, so we have to
            // assign pointer to original
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
   /// context id must be -1
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

   if ( !selector )
   {
      selector = &__emptyObj ;
   }
   if ( !updator )
   {
      updator = &__emptyObj ;
   }
   if ( !hint )
   {
      hint = &__emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                selector && updator && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpUpdate *pUpdate = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossAlign4( offsetof(MsgOpUpdate, name) +
                                   ossStrlen ( CollectionName ) + 1 ) +
                        ossAlign4( selector->objsize() ) +
                        ossAlign4( updator->objsize () ) +
                        ossAlign4( hint->objsize () ) ;
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
   // now the buffer is large enough
   pUpdate                       = (MsgOpUpdate*)(*ppBuffer) ;
   pUpdate->version              = 1 ;
   pUpdate->w                    = 0 ;
   pUpdate->flags                = flag ;
   // nameLength does NOT include '\0'
   pUpdate->nameLength           = ossStrlen ( CollectionName ) ;
   pUpdate->header.requestID     = reqID ;
   pUpdate->header.opCode        = MSG_BS_UPDATE_REQ ;
   pUpdate->header.messageLength = packetLength ;
   pUpdate->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pUpdate->header.version       = SDB_PROTOCOL_VER_2 ;
   pUpdate->header.flags         = 0 ;
   pUpdate->header.routeID.value = 0 ;
   pUpdate->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pUpdate->header.globalID), 0, sizeof(pUpdate->header.globalID) ) ;
   ossMemset( pUpdate->header.reserve, 0, sizeof(pUpdate->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pUpdate->name, CollectionName, pUpdate->nameLength ) ;
   pUpdate->name[pUpdate->nameLength] = 0 ;
   // get the offset of the first bson obj
   offset = ossAlign4( offsetof(MsgOpUpdate, name) + pUpdate->nameLength + 1 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), selector->objdata(),
                                       selector->objsize());
   // get the offset of the second bson obj
   offset += ossAlign4( selector->objsize() ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), updator->objdata(), updator->objsize());
   offset += ossAlign4( updator->objsize() ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(), hint->objsize());
   offset += ossAlign4( hint->objsize() ) ;
   // sanity test
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

INT32 msgExtractTransCommit ( const CHAR *pBuffer, const CHAR **ppHint )
{
   INT32 rc = SDB_OK ;
   INT32 offset = 0 ;
   INT32 length = 0 ;
   MsgOpTransCommit *pCommit = (MsgOpTransCommit*)pBuffer ;

   //old driver use MsgOpTransBegin as messageLength and old driver does not have hint
   if ( pCommit->header.messageLength != sizeof (MsgOpTransBegin) )
   {
      offset = ossRoundUpToMultipleX( sizeof( MsgOpTransCommit ), 4 );

      if ( offset  < pCommit->header.messageLength && ppHint )
      {
         *ppHint  = &pBuffer[offset] ;
         length = *((SINT32*)(&pBuffer[offset])) ;
         MSG_CHECK_BSON_LENGTH( length ) ;
         // the result may not exactly match because messageLength is 4 bytes aligned
         if ( offset + length > pCommit->header.messageLength )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
   }
done :
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTUP, "msgExtractUpdate" )
INT32 msgExtractUpdate ( const CHAR *pBuffer, INT32 *pflag,
                         const CHAR **ppCollectionName,
                         const CHAR **ppSelector,
                         const CHAR **ppUpdator,
                         const CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppSelector &&
                ppUpdator && ppHint, "Invalid input" ) ;

   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTUP );

   INT32 offset = 0 ;
   INT32 length = 0 ; // the length of bson object
   const MsgOpUpdate *pUpdate = (const MsgOpUpdate*)pBuffer ;

   const static INT32 _minSize = ossAlign4(offsetof(MsgOpUpdate, name)+1 ) ;

   /// check length
   if ( pUpdate->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *pflag = pUpdate->flags ;
   *ppCollectionName = pUpdate->name ;

   PD_TRACE1 ( SDB_MSGEXTRACTUP, PD_PACK_STRING(*ppCollectionName) ) ;

   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pUpdate->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   // get the offset for the first BSONObj
   offset = ossAlign4 ( offsetof(MsgOpUpdate, name) + pUpdate->nameLength + 1 ) ;
   /// check length
   if ( pUpdate->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppSelector )
   {
      *ppSelector = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of first BSONObj
   offset += ossAlign4( length ) ;
   /// check length
   if ( pUpdate->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppUpdator )
   {
      *ppUpdator  = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of second BSONObj
   offset += ossAlign4( length ) ;
   // check length
   if ( pUpdate->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppHint )
   {
      *ppHint  = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // the result may not exactly match because messageLength is 4 bytes aligned
   if ( pUpdate->header.messageLength < offset + length )
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
   INT32 packetLength = ossAlign4( offsetof(MsgOpInsert, name) +
                                   ossStrlen ( CollectionName ) + 1 ) +
                        ossAlign4( insertor->objsize() ) ;
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
   // now the buffer is large enough
   pInsert                       = (MsgOpInsert*)(*ppBuffer) ;
   pInsert->version              = 1 ;
   pInsert->w                    = 0 ;
   pInsert->flags                = flag ;
   // nameLength does NOT include '\0'
   pInsert->nameLength           = ossStrlen ( CollectionName ) ;
   pInsert->header.requestID     = reqID ;
   pInsert->header.opCode        = MSG_BS_INSERT_REQ ;
   pInsert->header.messageLength = packetLength ;
   pInsert->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pInsert->header.version       = SDB_PROTOCOL_VER_2 ;
   pInsert->header.flags         = 0 ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pInsert->header.globalID), 0, sizeof(pInsert->header.globalID) ) ;
   ossMemset( pInsert->header.reserve, 0, sizeof(pInsert->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pInsert->name, CollectionName, pInsert->nameLength ) ;
   pInsert->name[pInsert->nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossAlign4( offsetof(MsgOpInsert, name) + pInsert->nameLength + 1 ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), insertor->objdata(), insertor->objsize());
   offset += ossAlign4( insertor->objsize() ) ;
   // sanity test
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
                          std::vector< CHAR * > &ObjQueue,
                          netIOVec &ioVec,
                          IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName,
                "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   const static INT32 _fullVal = 0 ;
   PD_TRACE_ENTRY ( SDB_MSGBLDINSERTMSG2 ) ;
   PD_TRACE1 ( SDB_MSGBLDINSERTMSG2, PD_PACK_STRING(CollectionName) );

   MsgOpInsert *pInsert = NULL ;
   INT32 headLength = ossAlign4( offsetof(MsgOpInsert, name) +
                                 ossStrlen ( CollectionName ) + 1 );
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

         INT32 usedLength = ossAlign4( boInsertor.objsize() );
         INT32 fillLength = usedLength - boInsertor.objsize();
         if ( fillLength > 0 )
         {
            netIOV ioFiller;
            ioFiller.iovBase = (const void*)&_fullVal ;
            ioFiller.iovLen = fillLength ;
            ioVec.push_back( ioFiller ) ;
         }
         packetLength += usedLength ;
      }
      catch ( std::exception &e )
      {
         PD_CHECK( FALSE, SDB_INVALIDARG, error, PDERROR,
                   "Failed to parse the obj:%s", e.what() );
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
   // now the buffer is large enough
   pInsert                       = (MsgOpInsert*)(*ppBuffer) ;
   pInsert->version              = 1 ;
   pInsert->w                    = 0 ;
   pInsert->flags                = flag ;
   // nameLength does NOT include '\0'
   pInsert->nameLength           = ossStrlen ( CollectionName ) ;
   pInsert->header.requestID     = reqID ;
   pInsert->header.opCode        = MSG_BS_INSERT_REQ ;
   pInsert->header.messageLength = packetLength ;
   pInsert->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pInsert->header.version       = SDB_PROTOCOL_VER_2 ;
   pInsert->header.flags         = 0 ;
   pInsert->header.routeID.value = 0 ;
   pInsert->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pInsert->header.globalID), 0, sizeof(pInsert->header.globalID) ) ;
   ossMemset( pInsert->header.reserve, 0, sizeof(pInsert->header.reserve) ) ;
   // copy collection name
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
   // make sure the it's a valid insert request
   SDB_ASSERT ( pInsert->header.messageLength &&
                MSG_BS_INSERT_REQ ==  pInsert->header.opCode &&
                ossIsAligned4 ( pInsert->header.messageLength ),
                "Invalid messageLength" ) ;
   INT32 offset         = pInsert->header.messageLength ;
   INT32 packetLength   = offset + ossAlign4( insertor->objsize() ) ;
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
   // now the buffer is large enough
   pInsert = (MsgOpInsert*)(*ppBuffer) ;
   pInsert->header.messageLength = packetLength ;
   ossMemcpy ( &((*ppBuffer)[offset]), insertor->objdata(), insertor->objsize());
   offset += ossAlign4( insertor->objsize() ) ;
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
INT32 msgExtractInsert ( const CHAR *pBuffer, INT32 *pflag,
                         const CHAR **ppCollectionName,
                         const CHAR **ppInsertor,
                         INT32 &count,
                         const CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppInsertor,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTINSERT ) ;

   const static INT32 _minSize = ossAlign4( offsetof(MsgOpInsert, name) + 1 ) ;

   INT32 offset = 0 ;
   INT32 size = 0 ;
   const CHAR *pCurrent = NULL ;
   BOOLEAN hasHint = FALSE ;
   const MsgOpInsert *pInsert = (const MsgOpInsert*)pBuffer ;
   INT32 objSize = 0 ;

   /// check length
   if ( pInsert->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *pflag = pInsert->flags ;
   *ppCollectionName = pInsert->name ;
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pInsert->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;

   hasHint = OSS_BIT_TEST( *pflag, FLG_INSERT_HASHINT ) ? TRUE : FALSE ;
   if ( !hasHint && NULL != ppHint )
   {
      *ppHint = NULL ;
   }

   // get the offset for the first BSONObj
   offset = ossAlign4( offsetof(MsgOpInsert, name) + pInsert->nameLength + 1 ) ;
   if ( pInsert->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *ppInsertor = &pBuffer[offset] ;
   pCurrent = &pBuffer[offset] ;
   count = 0 ;
   while ( TRUE )
   {
      objSize = *((SINT32*)pCurrent) ;
      size = ossAlign4( objSize ) ;
      if ( size < MSG_BSON_MIN_LEN )
      {
         // If there is a hint in the insertion message, it's ALWAYS at the end
         // of the message, and 4 bytes(all set to 0) are placed before the
         // hint. Both the beginning of these 4 padding bytes and the hint are
         // 4 bytes aligned.
         // For old versions which do not support hint in insertion, error will
         // be returned in the else branch.
         if ( hasHint && 0 == size &&
              ( ( pInsert->header.messageLength - offset ) >=
                MSG_BSON_MIN_LEN + MSG_HINT_MARK_LEN ) )
         {
            offset += MSG_HINT_MARK_LEN ;
            pCurrent += MSG_HINT_MARK_LEN ;
            size = ossAlign4( *((SINT32*)pCurrent ) ) ;
            if ( offset + size < pInsert->header.messageLength )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Insert message length is not as expected. "
                       "Length in msg header: %d, count length: %d, rc: %d",
                       pInsert->header.messageLength, offset + size, rc ) ;
               SDB_ASSERT( FALSE, "Insert msg is invalid" ) ;
               goto error ;
            }
            else
            {
               if ( NULL != ppHint )
               {
                  *ppHint = &pBuffer[offset] ;
               }
               goto done ;
            }
         }
         else
         {
            PD_LOG( PDERROR, "Insert msg is invalid, msg length: %d, offset: %d, "
                    "current: %d, count: %d", pInsert->header.messageLength,
                    offset, *((SINT32*)pCurrent), count ) ;
            SDB_ASSERT( FALSE, "Insert msg is invalid" ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      ++count ;
      offset += size ;

      if ( '\0' != *( pCurrent + objSize - 1 ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( pInsert->header.messageLength <= offset )
      {
         break ;
      }
      pCurrent = &pBuffer[offset] ;
   }
   if ( (INT32)ossAlign4( pInsert->header.messageLength ) < offset )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
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

   if ( !query )
   {
      query = &__emptyObj ;
   }
   if ( !fieldSelector )
   {
      fieldSelector = &__emptyObj ;
   }
   if ( !orderBy )
   {
      orderBy = &__emptyObj ;
   }
   if ( !hint )
   {
      hint = &__emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                query && fieldSelector && orderBy && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpQuery *pQuery   = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossAlign4( offsetof(MsgOpQuery, name) +
                                   ossStrlen ( CollectionName ) + 1 ) +
                        ossAlign4( query->objsize() ) +
                        ossAlign4( fieldSelector->objsize() ) +
                        ossAlign4( orderBy->objsize() ) +
                        ossAlign4( hint->objsize() ) ;
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

   // now the buffer is large enough
   pQuery                        = (MsgOpQuery*)(*ppBuffer) ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = flag ;
   // nameLength does NOT include '\0'
   pQuery->nameLength            = ossStrlen ( CollectionName ) ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = numToSkip ;
   pQuery->numToReturn           = numToReturn ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.eye            = MSG_COMM_EYE_DEFAULT ;
   pQuery->header.version        = SDB_PROTOCOL_VER_2 ;
   pQuery->header.flags          = FLAG_RESULT_DETAIL | FLAG_PROCESS_DETAIL ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossMemset( &(pQuery->header.globalID), 0, sizeof(pQuery->header.globalID) ) ;
   ossMemset( pQuery->header.reserve, 0, sizeof(pQuery->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pQuery->name, CollectionName, pQuery->nameLength ) ;
   pQuery->name[pQuery->nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossAlign4( offsetof(MsgOpQuery, name) + pQuery->nameLength + 1 ) ;
   // write query condition
   ossMemcpy ( &((*ppBuffer)[offset]), query->objdata(), query->objsize() ) ;
   offset += ossAlign4( query->objsize() ) ;
   // write field select
   ossMemcpy ( &((*ppBuffer)[offset]), fieldSelector->objdata(),
               fieldSelector->objsize() ) ;
   offset += ossAlign4( fieldSelector->objsize() ) ;
   // write order by clause
   ossMemcpy ( &((*ppBuffer)[offset]), orderBy->objdata(),
               orderBy->objsize() ) ;
   offset += ossAlign4( orderBy->objsize() ) ;
   // write optimizer hint
   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(),
               hint->objsize() ) ;
   offset += ossAlign4( hint->objsize() ) ;
   // sanity test
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
INT32 msgExtractQuery  ( const CHAR *pBuffer, INT32 *pflag,
                         const CHAR **ppCollectionName,
                         SINT64 *numToSkip, SINT64 *numToReturn,
                         const CHAR **ppQuery,
                         const CHAR **ppFieldSelector,
                         const CHAR **ppOrderBy,
                         const CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer, "Invalid input" ) ;

   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTQUERY );
   const static INT32 _minSize = ossAlign4( offsetof(MsgOpQuery, name)+1 ) ;
   INT32 offset = 0 ;
   INT32 length = 0 ;
   const MsgOpQuery *pQuery = (const MsgOpQuery*)pBuffer ;

   if ( pQuery->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

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
   // get the offset for the first BSONObj
   offset = ossAlign4 ( (UINT32)(offsetof(MsgOpQuery, name) +
                                 pQuery->nameLength + 1) ) ;
   if ( pQuery->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppQuery )
   {
      *ppQuery = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of first BSONObj
   offset += ossAlign4( (UINT32)length ) ;
   if ( pQuery->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppFieldSelector )
   {
      *ppFieldSelector = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of second BSONObj
   offset += ossAlign4( (UINT32)length ) ;
   if ( pQuery->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppOrderBy )
   {
      *ppOrderBy = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of third BSONObj
   offset += ossAlign4( (UINT32)length ) ;
   if ( pQuery->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppHint )
   {
      *ppHint = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // the result should exactly match messageLength
   if ( pQuery->header.messageLength < offset + length )
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
   // now the buffer is large enough
   pGetMore                       = (MsgOpGetMore*)(*ppBuffer) ;
   // nameLength does NOT include '\0'
   pGetMore->header.requestID     = reqID ;
   pGetMore->header.opCode        = MSG_BS_GETMORE_REQ ;
   pGetMore->numToReturn          = numToReturn ;
   pGetMore->contextID            = contextID ;
   pGetMore->header.messageLength = packetLength ;
   pGetMore->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pGetMore->header.version       = SDB_PROTOCOL_VER_2 ;
   pGetMore->header.flags         = 0 ;
   pGetMore->header.routeID.value = 0 ;
   pGetMore->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pGetMore->header.globalID), 0, sizeof(pGetMore->header.globalID) ) ;
   ossMemset( pGetMore->header.reserve, 0, sizeof(pGetMore->header.reserve) ) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDGETMOREMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTGETMORE, "msgExtractGetMore" )
INT32 msgExtractGetMore  ( const CHAR *pBuffer,
                           SINT32 *numToReturn,
                           SINT64 *contextID )
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
   getMoreMsg.header.eye = MSG_COMM_EYE_DEFAULT ;
   getMoreMsg.header.version = SDB_PROTOCOL_VER_2 ;
   getMoreMsg.header.flags = 0 ;
   getMoreMsg.header.opCode = MSG_BS_GETMORE_REQ;
   getMoreMsg.header.TID = tid;
   getMoreMsg.header.routeID.value = 0;
   getMoreMsg.header.requestID = reqID;
   ossMemset( &(getMoreMsg.header.globalID), 0, sizeof(getMoreMsg.header.globalID) ) ;
   ossMemset( getMoreMsg.header.reserve, 0, sizeof(getMoreMsg.header.reserve) ) ;
   getMoreMsg.contextID = contextID;
   getMoreMsg.numToReturn = numToReturn;
   PD_TRACE_EXIT ( SDB_MSGFILLGETMOREMSG );
}

INT32 msgExtractAdvanceMsg( const CHAR *pBuffer, INT64 *contextID,
                            const CHAR **arg,
                            const CHAR **ppBackData,
                            INT32 *pBackDataSize )
{
   SDB_ASSERT ( pBuffer, "Invalid input" ) ;

   INT32 rc = SDB_OK ;
   INT32 offset = 0 ;
   INT32 length = 0 ;
   const MsgOpAdvance *pAdvance = (const MsgOpAdvance*)pBuffer ;

   const static INT32 _minSize = ossAlign4( sizeof( MsgOpAdvance ) ) + 5 ;

   /// check message
   if ( pAdvance->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( contextID )
   {
      *contextID = pAdvance->contextID ;
   }

   // get the offset for the first BSONObj
   offset = ossAlign4 ( sizeof(MsgOpAdvance) ) ;
   if ( arg )
   {
      *arg = &pBuffer[offset] ;
   }

   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   /// parse back data
   if ( pAdvance->backDataSize > 0 )
   {
      offset += ossAlign4( length ) ;

      if ( ppBackData )
      {
         *ppBackData = ( const CHAR* )&pBuffer[offset] ;
      }
      if ( pBackDataSize )
      {
         *pBackDataSize = pAdvance->backDataSize ;
      }

      offset += pAdvance->backDataSize ;
   }

   if ( pAdvance->header.messageLength < offset )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 msgBuildAdvanceMsg( CHAR **ppBuffer, INT32 *bufferSize,
                          SINT64 contextID, UINT64 reqID,
                          const BSONObj *arg,
                          const CHAR *pBackData,
                          INT32 backDataSize,
                          IExecutor *cb )
{
   if ( !arg )
   {
      arg = &__emptyObj ;
   }

   SDB_ASSERT ( ppBuffer && bufferSize && arg, "Invalid input" ) ;

   INT32 rc = SDB_OK ;
   MsgOpAdvance *pAdvance  = NULL ;
   INT32 offset            = 0 ;
   INT32 packetLength      = ossAlign4( sizeof( MsgOpAdvance ) ) +
                             ossAlign4( arg->objsize() ) +
                             ossAlign4( backDataSize ) ;

   if ( backDataSize < 0 )
   {
      PD_LOG( PDERROR, "Back data size is invalid" ) ;
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( packetLength < 0 )
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

   // now the buffer is large enough
   pAdvance                      = (MsgOpAdvance*)(*ppBuffer) ;
   pAdvance->contextID           = contextID ;
   pAdvance->backDataSize        = backDataSize ;
   ossMemset( (void*)(pAdvance->padding), 0, sizeof( pAdvance->padding ) ) ;

   pAdvance->header.requestID    = reqID ;
   pAdvance->header.opCode       = MSG_BS_ADVANCE_REQ ;
   pAdvance->header.messageLength= packetLength ;
   pAdvance->header.routeID.value= 0 ;
   pAdvance->header.TID          = ossGetCurrentThreadID() ;
   ossMemset( &(pAdvance->header.globalID), 0, sizeof(pAdvance->header.globalID) ) ;

   // get the offset of the first bson obj
   offset = ossAlign4( sizeof( MsgOpAdvance ) ) ;

   ossMemcpy ( &((*ppBuffer)[offset]), arg->objdata(), arg->objsize() );
   offset += ossAlign4( arg->objsize() ) ;

   // copy the back data
   if ( backDataSize > 0 )
   {
      ossMemcpy( &((*ppBuffer)[offset]), pBackData, backDataSize ) ;
      offset += ossAlign4( backDataSize ) ;
   }

   // sanity test
   if ( offset != packetLength )
   {
      PD_LOG ( PDERROR, "Invalid packet length" ) ;
      rc = SDB_SYS ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
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

   if ( !deletor )
   {
      deletor = &__emptyObj ;
   }
   if ( !hint )
   {
      hint = &__emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName &&
                deletor && hint, "Invalid input" ) ;
   INT32 rc             = SDB_OK ;
   MsgOpDelete *pDelete = NULL ;
   INT32 offset         = 0 ;
   INT32 packetLength = ossAlign4(offsetof(MsgOpDelete, name) +
                                  ossStrlen ( CollectionName ) + 1 ) +
                        ossAlign4( deletor->objsize()) +
                        ossAlign4( hint->objsize()) ;
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
   // now the buffer is large enough
   pDelete                       = (MsgOpDelete*)(*ppBuffer) ;
   pDelete->version              = 1 ;
   pDelete->w                    = 0 ;
   pDelete->flags                = flag ;
   // nameLength does NOT include '\0'
   pDelete->nameLength           = ossStrlen ( CollectionName ) ;
   pDelete->header.requestID     = reqID ;
   pDelete->header.opCode        = MSG_BS_DELETE_REQ ;
   pDelete->header.messageLength = packetLength ;
   pDelete->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pDelete->header.version       = SDB_PROTOCOL_VER_2 ;
   pDelete->header.flags         = 0 ;
   pDelete->header.routeID.value = 0 ;
   pDelete->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pDelete->header.globalID), 0, sizeof(pDelete->header.globalID) ) ;
   ossMemset( pDelete->header.reserve, 0, sizeof(pDelete->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pDelete->name, CollectionName, pDelete->nameLength ) ;
   pDelete->name[pDelete->nameLength]=0 ;
   // get the offset of the first bson obj
   offset = ossAlign4( offsetof(MsgOpDelete, name) + pDelete->nameLength + 1 ) ;

   ossMemcpy ( &((*ppBuffer)[offset]), deletor->objdata(), deletor->objsize() );
   offset += ossAlign4( deletor->objsize() ) ;

   ossMemcpy ( &((*ppBuffer)[offset]), hint->objdata(), hint->objsize() );
   offset += ossAlign4( hint->objsize() ) ;
   // sanity test
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
INT32 msgExtractDelete ( const CHAR *pBuffer, INT32 *pflag,
                         const CHAR **ppCollectionName,
                         const CHAR **ppDeletor,
                         const CHAR **ppHint )
{
   SDB_ASSERT ( pBuffer && pflag && ppCollectionName && ppDeletor && ppHint,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTDEL ) ;

   const static INT32 _minSize = ossAlign4( offsetof(MsgOpDelete, name)+1 ) ;
   INT32 offset = 0 ;
   INT32 length = 0 ;
   const MsgOpDelete *pDelete = (const MsgOpDelete*)pBuffer ;

   if ( pDelete->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *pflag = pDelete->flags ;
   *ppCollectionName = pDelete->name ;
   PD_TRACE1 ( SDB_MSGEXTRACTDEL, PD_PACK_STRING(*ppCollectionName) );
   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pDelete->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;

   // get the offset for the first BSONObj
   offset = ossAlign4 ( offsetof(MsgOpDelete, name) +
                        pDelete->nameLength + 1 ) ;
   if ( pDelete->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppDeletor )
   {
      *ppDeletor = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // add the size of first BSONObj
   offset += ossAlign4( length ) ;
   if ( pDelete->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppHint )
   {
      *ppHint = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // the result should exactly match messageLength
   if ( pDelete->header.messageLength < offset + length )
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
   // aligned by 8 since contextIDs are 64 bits
   // so we don't need to manually align it, it must be 8 bytes aligned already
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
   // now the buffer is large enough
   pKC                       = (MsgOpKillContexts*)(*ppBuffer) ;
   pKC->header.requestID     = reqID ;
   pKC->header.opCode        = MSG_BS_KILL_CONTEXT_REQ ;
   pKC->header.messageLength = packetLength ;
   pKC->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pKC->header.version       = SDB_PROTOCOL_VER_2 ;
   pKC->header.flags         = 0 ;
   pKC->numContexts          = numContexts ;
   pKC->header.routeID.value = 0 ;
   pKC->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pKC->header.globalID), 0, sizeof(pKC->header.globalID) ) ;
   ossMemset( pKC->header.reserve, 0, sizeof(pKC->header.reserve) ) ;
   // copy collection name
   ossMemcpy ( (CHAR*)(&pKC->contextIDs[0]), (CHAR*)pContextIDs,
               sizeof(SINT64)*pKC->numContexts ) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDKILLCONTXMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTKILLCONTX, "msgExtractKillContexts" )
INT32 msgExtractKillContexts ( const CHAR *pBuffer,
                               SINT32 *numContexts,
                               const SINT64 **ppContextIDs )
{
   SDB_ASSERT ( pBuffer && numContexts && ppContextIDs, "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTKILLCONTX );

   const static INT32 _minSize = offsetof(MsgOpKillContexts, contextIDs) ;

   const MsgOpKillContexts *pKC = (const MsgOpKillContexts*)pBuffer ;

   if ( pKC->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *numContexts = pKC->numContexts ;
   *ppContextIDs = &pKC->contextIDs[0] ;

   // we check exact match situation here, since the input should always
   // aligned with 8
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
   INT32 packetLength = ossAlign4 ( offsetof(MsgOpMsg, msg)+ msgLen + 1 ) ;
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
   // now the buffer is large enough
   pMsg                       = (MsgOpMsg*)(*ppBuffer) ;
   pMsg->header.requestID     = reqID ;
   pMsg->header.opCode        = MSG_BS_MSG_REQ ;
   pMsg->header.messageLength = packetLength ;
   pMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pMsg->header.version       = SDB_PROTOCOL_VER_2 ;
   pMsg->header.flags         = 0 ;
   pMsg->header.routeID.value = 0 ;
   pMsg->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pMsg->header.globalID), 0, sizeof(pMsg->header.globalID) ) ;
   ossMemset( pMsg->header.reserve, 0, sizeof(pMsg->header.reserve) ) ;
   // copy collection name
   ossStrncpy ( pMsg->msg, pMsgStr, msgLen ) ;
   pMsg->msg[msgLen] = 0 ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDMSGMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTMSG, "msgExtractMsg" )
INT32 msgExtractMsg ( const CHAR *pBuffer, const CHAR **ppMsgStr )
{
   SDB_ASSERT ( pBuffer && ppMsgStr, "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTMSG ) ;

   const static INT32 _minSize = ossAlign4( offsetof(MsgOpMsg, msg) + 1 ) ;
   const MsgOpMsg *pMsg = (const MsgOpMsg*)pBuffer ;

   if ( pMsg->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *ppMsgStr = &pMsg->msg[0] ;
   if ( (UINT32)pMsg->header.messageLength <
        offsetof(MsgOpMsg, msg) + ossStrlen(*ppMsgStr) + 1 )
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
   INT32 packetLength = ossAlign4 ( sizeof ( MsgOpReply ) ) ;
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
      packetLength = ossAlign4( packetLength ) ;
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
   pReply->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pReply->header.version       = SDB_PROTOCOL_VER_2 ;
   pReply->header.flags         = 0 ;
   pReply->header.routeID.value = 0 ;
   pReply->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pReply->header.globalID), 0, sizeof(pReply->header.globalID) ) ;
   ossMemset( pReply->header.reserve, 0, sizeof(pReply->header.reserve) ) ;
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
   INT32 packetLength = ossAlign4 ( sizeof ( MsgOpReply ) ) ;
   if ( numReturned != 0 )
   {
      packetLength += ossAlign4 ( bsonobj->objsize() ) ;
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
   pReply->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pReply->header.version       = SDB_PROTOCOL_VER_2 ;
   pReply->header.flags         = 0 ;
   pReply->header.routeID.value = 0 ;
   pReply->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pReply->header.globalID), 0, sizeof(pReply->header.globalID) ) ;
   ossMemset( pReply->header.reserve, 0, sizeof(pReply->header.reserve) ) ;
   if ( numReturned != 0 )
   {
      offset = ossAlign4 ( sizeof ( MsgOpReply ) ) ;
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
INT32 msgExtractReply ( const CHAR *pBuffer, SINT32 *flag, SINT64 *contextID,
                        SINT32 *startFrom, SINT32 *numReturned,
                        vector<BSONObj> &objList )
{
   SDB_ASSERT ( pBuffer , "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTREPLY ) ;

   INT32 offset = ossAlign4 ( sizeof ( MsgOpReply ) ) ;
   const MsgOpReply *pReply = (const MsgOpReply*)pBuffer ;

   if ( pReply->header.messageLength < offset )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( flag )
   {
      *flag = pReply->flags ;
   }
   if ( contextID )
   {
      *contextID = pReply->contextID ;
   }
   if ( startFrom )
   {
      *startFrom = pReply->startFrom ;
   }
   if ( numReturned )
   {
      *numReturned = pReply->numReturned ;
   }

   try
   {
      for ( SINT32 i = 0 ; i < pReply->numReturned ; i++ )
      {
         if ( pReply->header.messageLength < offset + 5 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         BSONObj obj(&pBuffer[offset]) ;
         SDB_ASSERT( obj.objsize() >= 5, "obj size must grater or equal 5" ) ;
         MSG_CHECK_BSON_LENGTH( obj.objsize() ) ;

         objList.push_back(obj) ;
         offset += ossAlign4 ( obj.objsize() ) ;
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
   INT32 packetLength = ossAlign4 ( sizeof ( MsgOpDisconnect ) ) ;
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
   pDisconnect->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pDisconnect->header.version       = SDB_PROTOCOL_VER_2 ;
   pDisconnect->header.flags         = 0 ;
   pDisconnect->header.routeID.value = 0 ;
   pDisconnect->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pDisconnect->header.globalID), 0,
              sizeof(pDisconnect->header.globalID) ) ;
   ossMemset( pDisconnect->header.reserve, 0,
              sizeof(pDisconnect->header.reserve) ) ;
done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDISCONNMSG, rc );
   return rc ;
error :
   goto done ;
}

// create reply header ONLY, note packet length is the header + data
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
   replyHeader.header.eye           = MSG_COMM_EYE_DEFAULT ;
   replyHeader.header.version       = SDB_PROTOCOL_VER_2 ;
   replyHeader.header.flags         = 0 ;
   replyHeader.header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(replyHeader.header.globalID), 0,
              sizeof(replyHeader.header.globalID) ) ;
   ossMemset( replyHeader.header.reserve, 0,
              sizeof(replyHeader.header.reserve) ) ;
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
   disconnectHeader.header.eye           = MSG_COMM_EYE_DEFAULT ;
   disconnectHeader.header.version       = SDB_PROTOCOL_VER_2 ;
   disconnectHeader.header.flags         = 0 ;
   disconnectHeader.header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(disconnectHeader.header.globalID), 0,
              sizeof(disconnectHeader.header.globalID) ) ;
   ossMemset( disconnectHeader.header.reserve, 0,
              sizeof(disconnectHeader.header.reserve) ) ;
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

// cluster manager
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

   if ( !arg1 )
   {
      arg1 = &__emptyObj ;
   }
   if ( !arg2 )
   {
      arg2 = &__emptyObj ;
   }
   if ( !arg3 )
   {
      arg3 = &__emptyObj ;
   }
   if ( !arg4 )
   {
      arg4 = &__emptyObj ;
   }
   SDB_ASSERT ( ppBuffer && pBufferSize, "Invalid input" ) ;
   INT32 rc                 = SDB_OK ;
   MsgCMRequest *pCMRequest = NULL ;
   INT32 offset             = ossAlign4 (
                                 offsetof ( MsgCMRequest, arguments ) ) ;
   INT32 packetLength       = offset +
                              ossAlign4 ( arg1->objsize() ) +
                              ossAlign4 ( arg2->objsize() ) +
                              ossAlign4 ( arg3->objsize() ) +
                              ossAlign4 ( arg4->objsize() ) ;
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
   pCMRequest->header.eye           = MSG_COMM_EYE_DEFAULT ;
   pCMRequest->header.version       = SDB_PROTOCOL_VER_2 ;
   pCMRequest->header.flags         = 0 ;
   pCMRequest->header.requestID     = 0 ;
   pCMRequest->header.opCode        = MSG_CM_REMOTE ;
   pCMRequest->header.routeID.value = 0 ;
   pCMRequest->header.TID           = ossGetCurrentThreadID() ;
   ossMemset( &(pCMRequest->header.globalID), 0,
              sizeof(pCMRequest->header.globalID) ) ;
   ossMemset( pCMRequest->header.reserve, 0,
              sizeof(pCMRequest->header.reserve) ) ;
   pCMRequest->remoCode             = remoCode ;
   // write arguments
   ossMemcpy ( &((*ppBuffer)[offset]), arg1->objdata(), arg1->objsize() ) ;
   offset += ossAlign4 ( arg1->objsize() ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg2->objdata(), arg2->objsize() ) ;
   offset += ossAlign4 ( arg2->objsize() ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg3->objdata(), arg3->objsize() ) ;
   offset += ossAlign4 ( arg3->objsize() ) ;
   ossMemcpy ( &((*ppBuffer)[offset]), arg4->objdata(), arg4->objsize() ) ;
   offset += ossAlign4 ( arg4->objsize() ) ;
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
INT32 msgExtractCMRequest ( const CHAR *pBuffer, SINT32 *remoCode,
                            const CHAR **arg1, const CHAR **arg2,
                            const CHAR **arg3, const CHAR **arg4 )
{
   SDB_ASSERT ( pBuffer && remoCode && arg1 && arg2 && arg3 && arg4,
                "Invalid input" ) ;
   INT32 rc           = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTCMREQ );

   INT32 length       = 0 ;
   INT32 offset       = ossAlign4 ( offsetof ( MsgCMRequest, arguments ) ) ;
   const MsgCMRequest *pCMRequest = (const MsgCMRequest*) pBuffer ;

   if ( pCMRequest->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *remoCode = pCMRequest->remoCode ;

   // extract the first BSONObj
   if ( arg1 )
   {
      *arg1 = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // extract the second BSONObj
   offset += ossAlign4( length ) ;
   if ( pCMRequest->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( arg2 )
   {
      *arg2 = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // extract the third BSONObj
   offset += ossAlign4( length ) ;
   if ( pCMRequest->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( arg3 )
   {
      *arg3 = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   MSG_CHECK_BSON_LENGTH( length ) ;

   // extract the fourth BSONObj
   offset += ossAlign4( length ) ;
   if ( pCMRequest->header.messageLength < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( arg4 )
   {
      *arg4 = &pBuffer[offset] ;
   }
   length = *((SINT32*)(&pBuffer[offset])) ;
   if ( pCMRequest->header.messageLength < offset + length )
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

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDQUERYCMDMSG, "msgBuildQueryCMDMsg" )
INT32 msgBuildQueryCMDMsg ( CHAR ** ppBuffer,
                            INT32 * pBufferSize,
                            const CHAR * commandName,
                            const BSONObj & boQuery,
                            const BSONObj & boSelect,
                            const BSONObj & boSort,
                            const BSONObj & boHint,
                            UINT64 reqID,
                            IExecutor * cb,
                            SINT64 numToReturn )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY( SDB_MSGBLDQUERYCMDMSG ) ;

   INT32 bufferSize = 0 ;
   CHAR * pBuffer = NULL ;
   MsgOpQuery * pQuery = NULL ;

   INT32 offset = 0 ;
   INT32 commandNameLength = ossStrlen ( commandName ) ;
   INT32 packetLength  = ossAlign4( offsetof(MsgOpQuery, name) +
                                    commandNameLength + 1 ) +
                         ossAlign4( boQuery.objsize() ) +
                         ossAlign4( boSelect.objsize() ) +
                         ossAlign4( boSort.objsize() ) +
                         ossAlign4( boHint.objsize() ) ;

   PD_TRACE1( SDB_MSGBLDQUERYCMDMSG, PD_PACK_INT( packetLength ) ) ;

   PD_CHECK( packetLength >= 0, SDB_INVALIDARG, error, PDERROR,
             "Packet size overflow" ) ;

   rc = msgCheckBuffer( &pBuffer, &bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to check buffer, rc: %d", rc ) ;

   // now the buffer is large enough
   pQuery                        = (MsgOpQuery *)pBuffer ;
   pQuery->version               = 1 ;
   pQuery->w                     = 0 ;
   pQuery->flags                 = 0 ;
   // nameLength does NOT include '\0'
   pQuery->nameLength            = commandNameLength ;
   pQuery->header.requestID      = reqID ;
   pQuery->header.opCode         = MSG_BS_QUERY_REQ ;
   pQuery->numToSkip             = 0 ;
   pQuery->numToReturn           = numToReturn ;
   pQuery->header.messageLength  = packetLength ;
   pQuery->header.eye            = MSG_COMM_EYE_DEFAULT ;
   pQuery->header.version        = SDB_PROTOCOL_VER_2 ;
   pQuery->header.flags          = 0 ;
   pQuery->header.routeID.value  = 0 ;
   pQuery->header.TID            = ossGetCurrentThreadID() ;
   ossMemset( &(pQuery->header.globalID), 0, sizeof(pQuery->header.globalID) ) ;
   ossMemset( pQuery->header.reserve, 0, sizeof(pQuery->header.reserve) ) ;

   // copy collection name
   ossStrncpy ( pQuery->name, commandName, commandNameLength ) ;
   pQuery->name[ commandNameLength ] = '\0' ;

   // get the offset of the first bson obj
   offset = ossAlign4( offsetof( MsgOpQuery, name ) + pQuery->nameLength + 1 ) ;
   // write query condition
   ossMemcpy( pBuffer + offset, boQuery.objdata(), boQuery.objsize() ) ;
   offset += ossAlign4( boQuery.objsize() ) ;

   // write field select
   ossMemcpy( pBuffer + offset, boSelect.objdata(), boSelect.objsize() ) ;
   offset += ossAlign4( boSelect.objsize() ) ;

   // write order by clause
   ossMemcpy( pBuffer + offset, boSort.objdata(), boSort.objsize() ) ;
   offset += ossAlign4( boSort.objsize() ) ;

   // write optimizer hint
   ossMemcpy( pBuffer + offset, boHint.objdata(), boHint.objsize() ) ;
   offset += ossAlign4( boHint.objsize() ) ;

   // sanity test
   PD_CHECK( offset == packetLength, SDB_SYS, error, PDERROR,
             "Invalid packet length" ) ;

   (*ppBuffer) = pBuffer ;
   (*pBufferSize) = bufferSize ;

done :
   PD_TRACE_EXITRC( SDB_MSGBLDQUERYCMDMSG, rc );
   return rc ;

error :
   msgReleaseBuffer( pBuffer, cb ) ;
   goto done ;
}

INT32 msgBuildCreateCSMsg( CHAR **ppBuffer, INT32 *bufferSize,
                           const CHAR *CollectionSpaceName,
                           const BSONObj &options, UINT64 reqID,
                           engine::IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionSpaceName,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   try
   {
      const BSONObj emptyObj ;
      BSONObjBuilder builder ;
      BSONObj query ;
      builder.append( FIELD_NAME_NAME, CollectionSpaceName ) ;
      builder.appendElements( options ) ;
      query = builder.obj() ;
      rc = msgBuildQueryCMDMsg(
                            ppBuffer, bufferSize,
                            CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE,
                            query, emptyObj, emptyObj, emptyObj, reqID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate create cs request, "
                   "cs: %s,rc: %d", CollectionSpaceName, rc ) ;
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_OOM ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 msgBuildTestCSMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         const CHAR *CollectionSpaceName,
                         UINT64 reqID, engine::IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionSpaceName,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   try
   {
      const BSONObj emptyObj ;
      BSONObj query = BSON( FIELD_NAME_NAME << CollectionSpaceName ) ;
      rc = msgBuildQueryCMDMsg(
                            ppBuffer, bufferSize,
                            CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTIONSPACE,
                            query, emptyObj, emptyObj, emptyObj, reqID, cb,
                            -1 ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate test cs request, "
                   "cs: %s,rc: %d", CollectionSpaceName, rc ) ;
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_OOM ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

INT32 msgBuildDropCSMsg( CHAR **ppBuffer, INT32 *bufferSize,
                         const CHAR *CollectionSpaceName,
                         BOOLEAN skipRecycleBin,
                         BOOLEAN ignoreLock,
                         UINT64 reqID,
                         IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionSpaceName,
                "Invalid input" ) ;
   const BSONObj emptyObj ;
   INT32 rc = SDB_OK ;
   BSONObj boQuery, boHint ;
   try
   {
      BSONObjBuilder bobQuery;
      bobQuery.append( FIELD_NAME_NAME, CollectionSpaceName ) ;
      bobQuery.appendBool( FIELD_NAME_SKIPRECYCLEBIN, skipRecycleBin ) ;
      boQuery = bobQuery.obj() ;

      BSONObjBuilder bobHint ;
      bobHint.appendBool( FIELD_NAME_IGNORE_LOCK, ignoreLock ) ;
      boHint = bobHint.obj() ;
   }
   catch( exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG( PDERROR, "occurred unexpected error:%s", e.what() );
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE,
                             boQuery, emptyObj, emptyObj, boHint,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 msgBuildCreateCLMsg( CHAR **ppBuffer, INT32 *bufferSize,
                           const CHAR *CollectionName,
                           const BSONObj &options, UINT64 reqID,
                           engine::IExecutor *cb )
{
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName,
                "Invalid input" ) ;
   INT32 rc = SDB_OK ;
   try
   {
      const BSONObj emptyObj ;
      BSONObjBuilder builder ;
      BSONObj query ;
      builder.append( FIELD_NAME_NAME, CollectionName ) ;
      builder.appendElements( options ) ;
      query = builder.obj() ;
      rc = msgBuildQueryCMDMsg(
                            ppBuffer, bufferSize,
                            CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                            query, emptyObj, emptyObj, emptyObj, reqID, cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to generate create cl request, "
                   "cl: %s,rc: %d", CollectionName, rc ) ;
   }
   catch( std::exception &e )
   {
      PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
      rc = SDB_OOM ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDROPCLMSG, "msgBuildDropCLMsg" )
INT32 msgBuildDropCLMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                          const CHAR *CollectionName,
                          BOOLEAN skipRecycleBin,
                          BOOLEAN ignoreLock,
                          UINT64 reqID,
                          IExecutor *cb )
{
   PD_TRACE_ENTRY ( SDB_MSGBLDDROPCLMSG );

   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName,
                "Invalid input" ) ;
   PD_TRACE1 ( SDB_MSGBLDDROPCLMSG, PD_PACK_STRING(CollectionName) );
   INT32 rc             = SDB_OK ;
   BSONObj boQuery, boHint ;
   try
   {
      bson::BSONObjBuilder bobQuery;
      bobQuery.append( FIELD_NAME_NAME, CollectionName ) ;
      bobQuery.appendBool( FIELD_NAME_SKIPRECYCLEBIN, skipRecycleBin ) ;
      boQuery = bobQuery.obj() ;

      BSONObjBuilder bobHint ;
      bobHint.appendBool( FIELD_NAME_IGNORE_LOCK, ignoreLock ) ;
      boHint = bobHint.obj() ;
   }
   catch ( exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build drop collection message failed, occurred unexpected error:%s",
               e.what() );
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION,
                             boQuery, __emptyObj, __emptyObj, boHint,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDROPCLMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDTRUNCCLMSG, "msgBuildTruncateCLMsg" )
INT32 msgBuildTruncateCLMsg( CHAR **ppBuffer,
                             INT32 *bufferSize,
                             const CHAR *CollectionName,
                             BOOLEAN skipRecycleBin,
                             BOOLEAN ignoreLock,
                             UINT64 reqID,
                             IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY( SDB_MSGBLDTRUNCCLMSG ) ;

   SDB_ASSERT( ppBuffer && bufferSize && CollectionName, "Invalid input" ) ;
   PD_TRACE1( SDB_MSGBLDTRUNCCLMSG, PD_PACK_STRING( CollectionName ) ) ;

   BSONObj boQuery, boHint ;
   const BSONObj emptyObj ;

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_COLLECTION, CollectionName ) ;
      bobQuery.appendBool( FIELD_NAME_SKIPRECYCLEBIN, skipRecycleBin ) ;
      boQuery = bobQuery.obj() ;

      BSONObjBuilder bobHint ;
      bobHint.appendBool( FIELD_NAME_IGNORE_LOCK, ignoreLock ) ;
      boHint = bobHint.obj() ;
   }
   catch ( exception &e )
   {
      PD_LOG ( PDERROR, "Failed to build truncate collection message, "
               "occur exception %s", e.what() ) ;
      rc = ossException2RC( &e ) ;
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE,
                             boQuery, emptyObj, emptyObj, boHint,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_MSGBLDTRUNCCLMSG, rc ) ;
   return rc ;

error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDALTERCLMSG, "msgBuildAlterCLMsg" )
INT32 msgBuildAlterCLMsg( CHAR **ppBuffer,
                          INT32 *bufferSize,
                          const CHAR *collectionName,
                          const BSONObj &options,
                          UINT64 reqID,
                          engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY( SDB_MSGBLDALTERCLMSG ) ;

   SDB_ASSERT( ppBuffer && bufferSize && collectionName, "Invalid input" ) ;
   PD_TRACE1( SDB_MSGBLDALTERCLMSG, PD_PACK_STRING( collectionName ) ) ;

   BSONObj boQuery ;
   const BSONObj emptyObj ;

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append ( FIELD_NAME_NAME, collectionName ) ;
      bobQuery.append ( FIELD_NAME_OPTIONS, options ) ;
      boQuery = bobQuery.obj() ;
   }
   catch ( exception &e )
   {
      PD_LOG ( PDERROR, "Failed to build alter collection message, "
               "occur exception %s", e.what() ) ;
      rc = ossException2RC( &e ) ;
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION,
                             boQuery, emptyObj, emptyObj, emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_MSGBLDALTERCLMSG, rc ) ;
   return rc ;

error:
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

   BSONObj boQuery ;

   if ( !lowBound )
   {
      lowBound = &__emptyObj ;
   }
   if ( !upBound )
   {
      upBound = &__emptyObj ;
   }

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_NAME, CollectionName ) ;
      bobQuery.append( FIELD_NAME_SUBCLNAME, subCollectionName ) ;
      bobQuery.append( FIELD_NAME_LOWBOUND, *lowBound ) ;
      bobQuery.append( FIELD_NAME_UPBOUND, *upBound ) ;
      boQuery = bobQuery.obj() ;
   }
   catch ( exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build link collection message failed, "
               "occurred unexpected error:%s",
               e.what() );
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_LINK_CL,
                             boQuery, __emptyObj, __emptyObj, __emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

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

   BSONObj boQuery ;

   try
   {
      BSONObjBuilder bobQuery ;
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

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL,
                             boQuery, __emptyObj, __emptyObj, __emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done :
   PD_TRACE_EXITRC ( SDB_MSGBLDUNLINKCLMSG, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDCRTINXMSG, "msgBuildCreateIndexMsg" )
INT32 msgBuildCreateIndexMsg( CHAR **ppBuffer,
                              INT32 *bufferSize,
                              const CHAR *collectionName,
                              const BSONObj &options,
                              UINT64 reqID,
                              engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY( SDB_MSGBLDCRTINXMSG ) ;

   BSONObj boQuery ;

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_NAME, collectionName ) ;
      bobQuery.append( FIELD_NAME_INDEX, options ) ;
      boQuery = bobQuery.obj() ;
   }
   catch ( std::exception &e )
   {
      PD_LOG ( PDERROR, "Failed to build alter collection message, "
               "occur exception %s", e.what() ) ;
      rc = ossException2RC( &e ) ;
      goto error ;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX,
                             boQuery, __emptyObj, __emptyObj, __emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_MSGBLDCRTINXMSG, rc ) ;
   return rc ;

error:
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
   SDB_ASSERT ( ppBuffer && bufferSize && CollectionName && IndexName,
                "Invalid input" ) ;
   PD_TRACE2 ( SDB_MSGBLDDROPINXMSG, PD_PACK_STRING(CollectionName),
                                     PD_PACK_STRING(IndexName) );
   INT32 rc = SDB_OK ;
   BSONObj boQuery ;

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_COLLECTION, CollectionName ) ;
      BSONObjBuilder bobIndex( bobQuery.subobjStart( FIELD_NAME_INDEX ) ) ;
      bobIndex.append( IXM_FIELD_NAME_NAME, IndexName ) ;
      bobIndex.done() ;
      boQuery = bobQuery.obj() ;
   }
   catch ( exception &e )
   {
      rc = SDB_INVALIDARG;
      PD_LOG ( PDERROR,
               "build dropindex message failed, occured unexpected error:%s",
               e.what() );
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                             boQuery, __emptyObj, __emptyObj, __emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done :
   PD_TRACE_EXITRC ( SDB_MSGBLDDROPINXMSG, rc );
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBLDDROPRECYBINITEMMSG, "msgBuildDropRecyBinItemMsg" )
INT32 msgBuildDropRecyBinItemMsg( CHAR **ppBuffer,
                                  INT32 *bufferSize,
                                  const CHAR *recycleName,
                                  BOOLEAN isRecursive,
                                  BOOLEAN isEnforced,
                                  BOOLEAN ignoreLock,
                                  UINT64 reqID,
                                  engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   PD_TRACE_ENTRY( SDB_MSGBLDDROPRECYBINITEMMSG ) ;

   BSONObj boQuery ;
   BSONObj boHint ;

   try
   {
      BSONObjBuilder bobQuery ;
      bobQuery.append( FIELD_NAME_RECYCLE_NAME, recycleName ) ;
      bobQuery.appendBool( FIELD_NAME_ENFORCED1, isEnforced ) ;
      bobQuery.appendBool( FIELD_NAME_RECURSIVE, isRecursive ) ;
      boQuery = bobQuery.obj() ;

      BSONObjBuilder bobHint ;
      bobHint.appendBool( FIELD_NAME_IGNORE_LOCK, ignoreLock ) ;
      boHint = bobHint.obj() ;
   }
   catch ( exception &e )
   {
      PD_LOG( PDERROR, "Failed to build drop recycle bin item message, "
              "occur exception %s", e.what() ) ;
      rc = ossException2RC( &e ) ;
      goto error;
   }

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_DROP_RECYCLEBIN_ITEM,
                             boQuery, __emptyObj, __emptyObj, boHint,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build query command, rc: %d", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_MSGBLDDROPRECYBINITEMMSG, rc ) ;
   return rc ;

error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTSQL, "msgExtractSql" )
INT32 msgExtractSql( const CHAR *pBuffer, const CHAR **sql )
{
   SDB_ASSERT( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSQL );
   const MsgOpSql *msg = ( const MsgOpSql * )pBuffer ;

   if ( (UINT32)msg->header.messageLength < sizeof( MsgHeader ) + 1 )
   {
      rc = SDB_INVALIDARG ;
   }
   else
   {
      *sql = pBuffer + sizeof( MsgHeader ) ;
      if ( (UINT32)msg->header.messageLength <
           sizeof( MsgHeader ) + ossStrlen( *sql ) + 1 )
      {
         rc = SDB_INVALIDARG ;
      }
   }

   PD_TRACE_EXITRC ( SDB_MSGEXTRACTSQL, rc );
   return rc ;
}

static INT32 msgBuildSequenceMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  INT32 sequenceOpCode,
                                  UINT64 reqID, const BSONObj& options,
                                  engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   rc = msgBuildQueryMsg( ppBuffer, bufferSize, "", 0, reqID, 0, -1,
                          &options, NULL, NULL, NULL, cb ) ;
   if ( SDB_OK == rc )
   {
      MsgHeader* msg = (MsgHeader*)(*ppBuffer) ;
      msg->opCode = sequenceOpCode ;
   }
   else
   {
      PD_LOG( PDERROR, "Failed to build sequence msg, opCode=%d, rc=%d",
              sequenceOpCode, rc ) ;
   }

   return rc ;
}

INT32 msgBuildSequenceAcquireMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  UINT64 reqID, const BSONObj& options,
                                  engine::IExecutor *cb )
{
   return msgBuildSequenceMsg( ppBuffer, bufferSize, MSG_GTS_SEQUENCE_ACQUIRE_REQ,
                               reqID, options, cb ) ;
}

INT32 msgBuildSequenceInvalidateCacheMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                          const BSONObj &boQuery, UINT64 reqID,
                                          engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   SDB_ASSERT ( ppBuffer && bufferSize,
                "Invalid input" ) ;

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_SEQUENCE_CACHE,
                             boQuery, __emptyObj, __emptyObj, __emptyObj,
                             reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Failed to build invalidate sequence cache message, rc: %d", rc ) ;

done :
   return rc ;
error :
   goto done ;
}

INT32 msgExtractSequenceRequestMsg( const CHAR *pBuffer, BSONObj& options )
{
   INT32 rc = SDB_OK ;
   const CHAR* query = NULL ;

   rc = msgExtractQuery( pBuffer, NULL, NULL, NULL, NULL,
                         &query, NULL, NULL, NULL ) ;
   if ( SDB_OK == rc )
   {
      try
      {
         options = BSONObj( query ) ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "unexpected err happened:%s", e.what() ) ;
         rc = SDB_SYS ;
      }
   }
   else
   {
      PD_LOG( PDERROR, "Failed to extract sequence msg, rc=%d", rc ) ;
   }

   return rc ;
}

INT32 msgExtractSequenceAcquireReply( const CHAR *pBuffer, BSONObj& options )
{
   INT32 rc = SDB_OK ;
   const MsgOpReply *pReply = (const MsgOpReply*)pBuffer ;
   INT32 offset = ossAlign4 ( sizeof ( MsgOpReply ) ) ;
   INT32 numReturned = 0 ;
   BSONObj obj ;

   // check length
   if ( pReply->header.messageLength < offset )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   numReturned = pReply->numReturned ;
   if ( numReturned > 1 )
   {
      rc = SDB_INVALIDARG ;
      PD_LOG( PDERROR,
              "More than 1 objects returned for sequence acquire request" ) ;
      goto error ;
   }

   if ( pReply->header.messageLength >= offset + 5 )
   {
      try
      {
         obj = BSONObj( pBuffer + offset ) ;
         SDB_ASSERT( obj.objsize() >= 5, "obj size must grater or equal 5" ) ;
         MSG_CHECK_BSON_LENGTH( obj.objsize() ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Unexpected exception happened when extracting "
                 "acquire sequence reply: %s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   if ( obj.isEmpty() )
   {
      if ( SDB_OK == pReply->flags )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "No object returned for sequence acquire request" ) ;
         goto error ;
      }
   }
   else
   {
      options = obj ;
   }

done:
   return rc ;
error:
   goto done ;
}

INT32 msgBuildTransCommitPreMsg ( CHAR **ppBuffer, INT32 *bufferSize,
                                  IExecutor *cb )
{
   SDB_ASSERT( ppBuffer, "invalid input" ) ;
   INT32 rc = SDB_OK;
   MsgOpTransCommitPre *pMsg = NULL;
   INT32 packetLength = ossAlign4( sizeof( MsgOpTransCommitPre ) );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransCommitPre *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.eye = MSG_COMM_EYE_DEFAULT ;
   pMsg->header.version = SDB_PROTOCOL_VER_2 ;
   pMsg->header.flags = 0 ;
   pMsg->header.opCode = MSG_BS_TRANS_COMMITPRE_REQ;
   pMsg->header.routeID.value = 0;
   ossMemset( &(pMsg->header.globalID), 0, sizeof(pMsg->header.globalID) ) ;
   ossMemset( pMsg->header.reserve, 0, sizeof(pMsg->header.reserve) ) ;

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
   INT32 packetLength = ossAlign4( sizeof( MsgOpTransCommit ) );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransCommit *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.eye = MSG_COMM_EYE_DEFAULT ;
   pMsg->header.version = SDB_PROTOCOL_VER_2 ;
   pMsg->header.flags = 0 ;
   pMsg->header.opCode = MSG_BS_TRANS_COMMIT_REQ;
   pMsg->header.routeID.value = 0;
   ossMemset( &(pMsg->header.globalID), 0, sizeof(pMsg->header.globalID) ) ;
   ossMemset( pMsg->header.reserve, 0, sizeof(pMsg->header.reserve) ) ;

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
   INT32 packetLength = ossAlign4( sizeof( MsgOpTransRollback ) );
   PD_CHECK( (packetLength > 0), SDB_INVALIDARG, error, PDERROR,
            "Packet size overflow" );
   rc = msgCheckBuffer( ppBuffer, bufferSize, packetLength, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "failed to check buffer" );
   pMsg = (MsgOpTransRollback *)(*ppBuffer);
   pMsg->header.messageLength = packetLength;
   pMsg->header.eye = MSG_COMM_EYE_DEFAULT ;
   pMsg->header.version = SDB_PROTOCOL_VER_2 ;
   pMsg->header.flags = 0 ;
   pMsg->header.opCode = MSG_BS_TRANS_ROLLBACK_REQ;
   pMsg->header.routeID.value = 0;
   ossMemset( &(pMsg->header.globalID), 0, sizeof(pMsg->header.globalID) ) ;
   ossMemset( pMsg->header.reserve, 0, sizeof(pMsg->header.reserve) ) ;

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
INT32 msgExtractSysInfoRequest ( const CHAR *pBuffer, BOOLEAN &endianConvert )
{
   SDB_ASSERT ( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSYSINFOREQUEST ) ;
   const MsgSysInfoRequest *request = (const MsgSysInfoRequest*)pBuffer ;
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
                             UINT64 dbStartTime,
                             IExecutor *cb )
{
   SDB_ASSERT ( NULL != ppBuffer &&
                NULL != pBufferSize, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   INT32 version = 0 ;
   INT32 subVersion = 0 ;
   INT32 fixVersion = 0 ;
   md5::md5digest digest ;
   MsgSysInfoReply *reply = NULL ;
   PD_TRACE_ENTRY ( SDB_MSGBUILDSYSINFOREPLY ) ;
   rc = msgCheckBuffer ( ppBuffer, pBufferSize, sizeof(MsgSysInfoReply), cb ) ;
   PD_RC_CHECK ( rc, PDERROR, "Failed to check buffer, rc = %d", rc ) ;
   reply                                     = (MsgSysInfoReply*)(*ppBuffer) ;
   reply->header.specialSysInfoLen           = MSG_SYSTEM_INFO_LEN ;
   reply->header.eyeCatcher                  = MSG_SYSTEM_INFO_EYECATCHER ;
   reply->header.realMessageLength           = sizeof(MsgSysInfoReply) ;
   reply->osType                             = OSS_OSTYPE ;
   reply->dbStartTime                        = dbStartTime ;
   ossGetVersion( &version, &subVersion, &fixVersion, NULL, NULL, NULL ) ;
   reply->version                            = version ;
   reply->subVersion                         = subVersion ;
   reply->fixVersion                         = fixVersion ;
   ossMemset( reply->pad, 0, sizeof( reply->pad ) ) ;

   md5::md5( (const void *)reply,
             sizeof(MsgSysInfoReply) - sizeof(reply->fingerprint),
             digest ) ;
   ossMemcpy( reply->fingerprint, digest, sizeof(reply->fingerprint) ) ;

done :
   PD_TRACE_EXITRC ( SDB_MSGBUILDSYSINFOREPLY, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTSYSINFOREPLY, "msgExtractSysInfoReply" )
INT32 msgExtractSysInfoReply ( const CHAR *pBuffer, BOOLEAN &endianConvert,
                               INT32 *osType, SDB_PROTOCOL_VERSION *protocolVer )
{
   SDB_ASSERT ( NULL != pBuffer, "invalid pBuffer" ) ;
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTSYSINFOREPLY ) ;
   const MsgSysInfoReply *reply = (const MsgSysInfoReply*)pBuffer ;
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

   if ( protocolVer )
   {
      md5::md5digest digest ;
      md5::md5( (const void *)reply,
                sizeof(MsgSysInfoReply) - sizeof(reply->fingerprint),
                digest ) ;
      *protocolVer =
            ( 0 == ossStrncmp( reply->fingerprint,
                               (const char *)digest,
                               sizeof(reply->fingerprint) ) ) ?
            SDB_PROTOCOL_VER_2 : SDB_PROTOCOL_VER_1 ;
   }
done :
   PD_TRACE_EXITRC ( SDB_MSGEXTRACTSYSINFOREPLY, rc ) ;
   return rc ;
error :
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTAGGRREQ, "msgExtractAggrRequest" )
INT32 msgExtractAggrRequest ( const CHAR *pBuffer, const CHAR **ppCollectionName,
                              const CHAR **ppObjs, INT32 &count,
                              INT32 *pFlags )
{
   INT32 rc = SDB_OK;
   INT32 offset = 0;
   INT32 msgLen = 0;
   INT32 num = 0;
   const CHAR *pCur = NULL;
   PD_TRACE_ENTRY ( SDB_MSGEXTRACTAGGRREQ ) ;
   SDB_ASSERT( pBuffer && ppCollectionName && ppObjs, "Invalid input!" ) ;

   const static INT32 _minSize = ossAlign4( offsetof(MsgOpAggregate, name)+1 ) ;
   const MsgOpAggregate *pAggr = (const MsgOpAggregate *)pBuffer ;

   if ( pAggr->header.messageLength < _minSize )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *ppCollectionName = pAggr->name ;
   if ( pFlags )
   {
      *pFlags = pAggr->flags ;
   }
   msgLen = pAggr->header.messageLength ;

   SDB_VALIDATE_GOTOERROR ( (SINT32)ossStrlen ( *ppCollectionName ) ==
                            pAggr->nameLength, SDB_INVALIDARG,
                            "Invalid name length" ) ;
   offset = ossAlign4( offsetof(MsgOpAggregate, name) + pAggr->nameLength + 1 ) ;

   /// at least 1 object
   if ( msgLen < offset + 5 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ppObjs )
   {
      *ppObjs = &pBuffer[ offset ];
   }

   while( TRUE )
   {
      pCur = &pBuffer[ offset ] ;
      INT32 size = *((SINT32 *)pCur) ;
      MSG_CHECK_BSON_LENGTH( size ) ;

      ++num ;
      offset += ossAlign4( size ) ;

      if ( msgLen <= offset )
      {
         break ;
      }
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
      bsonLen = ossAlign4( msgHeader->bsonLen ) ;
      if ( msgLen < ( sizeof( MsgOpLob ) + bsonLen ) )
      {
         PD_LOG( PDERROR, "invalid msg len:%d", msgLen ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      try
      {
         meta = BSONObj( body ) ;
         if ( msgHeader->bsonLen < (UINT32)meta.objsize() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         body += ossAlign4( msgHeader->bsonLen ) ;
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
      //UINT32 dataLen = ossAlign4( t->columns.len ) ;
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

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTGETLOBRTDETAILREQ, "msgExtractGetLobRTDetailRequest" )
INT32 msgExtractGetLobRTDetailRequest( const CHAR *pBuffer,
                                       const MsgOpLob **header )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTGETLOBRTDETAILREQ ) ;
   SDB_ASSERT( NULL != pBuffer && NULL != header, "cat not be null" ) ;

   BSONObj lob ;

   rc = msgExtractLobRequest( pBuffer, header, lob, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "failed to extract lob msg:%d", rc ) ;
      goto error ;
   }

done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTGETLOBRTDETAILREQ, rc ) ;
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

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTCREATELOBIDREQ, "msgExtractCreateLobIDRequest" )
INT32 msgExtractCreateLobIDRequest( const CHAR *pBuffer, const MsgOpLob **header,
                                    BSONObj &obj )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGEXTRACTCREATELOBIDREQ ) ;
   rc = msgExtractLobRequest( pBuffer, header, obj, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      PD_LOG( PDERROR, "Failed to extract lob msg:%d", rc ) ;
      goto error ;
   }
done:
   PD_TRACE_EXITRC( SDB_MSGEXTRACTCREATELOBIDREQ, rc ) ;
   return rc ;
error:
   goto done ;
}


// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGEXTRACTREREADRESULT, "msgExtractReadResult" )
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

INT32 msgBuildInvalidateCacheMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                  const BSONObj &boQuery, UINT64 reqID,
                                  IExecutor *cb )
{
   INT32 rc = SDB_OK ;

   BSONObj dummyObj ;

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
                             CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE,
                             boQuery, dummyObj, dummyObj,
                             dummyObj, reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Build invalidate cache message failed[%d]", rc ) ;

done:
   return rc ;
error:
   goto done ;
}

// PD_TRACE_DECLARE_FUNCTION ( SDB_MSGBUILDDATASOURCEINVALIDATECACHEMSG, "msgBuildDataSourceInvalidateCacheMsg" )
INT32 msgBuildDataSourceInvalidateCacheMsg( CHAR **ppBuffer, INT32 *bufferSize,
                                            const BSONObj& boQuery,
                                            UINT64 reqID,
                                            engine::IExecutor *cb )
{
   INT32 rc = SDB_OK ;
   PD_TRACE_ENTRY( SDB_MSGBUILDDATASOURCEINVALIDATECACHEMSG )  ;
   BSONObj dummyObj ;

   rc = msgBuildQueryCMDMsg( ppBuffer, bufferSize,
         CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_DATASOURCE_CACHE,
         boQuery, dummyObj, dummyObj, dummyObj, reqID, cb ) ;
   PD_RC_CHECK( rc, PDERROR, "Build invalidate data source cache message "
                "failed[%d]", rc ) ;

done:
   PD_TRACE_EXITRC( SDB_MSGBUILDDATASOURCEINVALIDATECACHEMSG, rc ) ;
   return rc ;
error:
   goto done ;
}
