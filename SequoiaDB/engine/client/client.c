/*******************************************************************************
   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include "client_internal.h"
#include "bson/bson.h"
#include "ossUtil.h"
#include "ossMem.h"
#include "msg.h"
#include "msgDef.h"
#include "network.h"
#include "common.h"
#include "pmdOptions.h"
#include "msgCatalogDef.h"
#include "../bson/lib/md5.h"
#include "fmpDef.h"
#include "utilCipher.h"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

SDB_EXTERN_C_START
BOOLEAN g_disablePassEncode = FALSE ;
SDB_EXTERN_C_END

#define BSON_MIN_SIZE 5
#define CLIENT_UNUSED( p )         \
   do { \
      p = p ; \
   } while ( 0 )

#define HANDLE_CHECK( handle, interhandle, handletype ) \
do                                                      \
{                                                       \
   if ( SDB_INVALID_HANDLE == handle )                  \
   {                                                    \
      rc = SDB_INVALIDARG ;                             \
      goto error ;                                      \
   }                                                    \
   if ( !interhandle ||                                 \
        handletype != interhandle->_handleType )        \
   {                                                    \
      rc = SDB_CLT_INVALID_HANDLE ;                     \
      goto error ;                                      \
   }                                                    \
}while( FALSE )

#define CHECK_RET_MSGHEADER( pSendBuf, pRecvBuf, connHandle )               \
do                                                                          \
{                                                                           \
   sdbConnectionStruct *db = (sdbConnectionStruct*)connHandle ;             \
   HANDLE_CHECK( connHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;             \
   rc = clientCheckRetMsgHeader( pSendBuf, pRecvBuf, db->_endianConvert ) ; \
   if ( SDB_OK != rc )                                                      \
   {                                                                        \
      if ( SDB_UNEXPECTED_RESULT == rc )                                    \
      {                                                                     \
         sdbDisconnect( connHandle ) ;                                      \
      }                                                                     \
      goto error ;                                                          \
   }                                                                        \
}while( FALSE )

#define ALLOC_HANDLE( handle, type )                      \
do                                                        \
{                                                         \
   handle = ( type* ) SDB_OSS_MALLOC ( sizeof( type ) ) ; \
   if ( !handle )                                         \
   {                                                      \
      rc = SDB_OOM ;                                      \
      goto error ;                                        \
   }                                                      \
   ossMemset ( handle, 0, sizeof( type ) ) ;              \
}while( FALSE )

#define INIT_CURSOR( cursor, conn, handle, contextID )      \
do                                                          \
{                                                           \
   cursor->_handleType = SDB_HANDLE_TYPE_CURSOR ;           \
   cursor->_connection=                                     \
               ( sdbConnectionHandle )conn ;                \
   cursor->_sock            = handle->_sock ;               \
   cursor->_contextID       = contextID ;                   \
   cursor->_offset          = -1 ;                          \
   cursor->_endianConvert   = handle->_endianConvert ;      \
}while( FALSE )

#define BSON_INIT( bsonobj )              \
do                                        \
{                                         \
   bson_init( &bsonobj );                 \
   bsoninit = TRUE ;                      \
}while ( FALSE )

#define BSON_INIT2( bsonobj, flag )       \
do                                        \
{                                         \
   bson_init( &bsonobj );                 \
   flag = TRUE ;                          \
} while ( FALSE )

#define BSON_APPEND_NULL( bsonobj, key )                 \
do                                                       \
{                                                        \
   rc = bson_append_null( &bsonobj, key ) ;              \
   if ( rc )                                             \
   {                                                     \
      rc = SDB_DRIVER_BSON_ERROR ;                       \
      goto error ;                                       \
   }                                                     \
}while ( FALSE )

#define BSON_APPEND( bsonobj, key, val, type )       \
do                                                   \
{                                                    \
   rc = bson_append_##type( &bsonobj, key, val ) ;   \
   if ( rc )                                         \
   {                                                 \
      rc = SDB_DRIVER_BSON_ERROR ;                   \
      goto error ;                                   \
   }                                                 \
}while( FALSE )


#define BSON_FINISH( bson )        \
do                                 \
{                                  \
   rc = bson_finish ( &bson ) ;    \
   if ( rc )                       \
   {                               \
      rc = SDB_DRIVER_BSON_ERROR ; \
      goto error ;                 \
   }                               \
}while ( FALSE )

#define BSON_DESTROY( bson )       \
do                                 \
{                                  \
   if ( bsoninit )                 \
   {                               \
      bson_destroy( &bson ) ;      \
   }                               \
}while( FALSE )

#define BSON_DESTROY2( bson, flag )       \
do                                        \
{                                         \
   if ( flag )                            \
   {                                      \
      bson_destroy( &bson ) ;             \
   }                                      \
}while( FALSE )

static INT32 _mergeBson( bson* to, bson* from )
{
   INT32 rc = SDB_OK ;
   bson_iterator iter ;

   if ( NULL == to || NULL == from )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   bson_iterator_init( &iter, from ) ;
   while ( bson_iterator_more( &iter ) )
   {
      bson_iterator_next( &iter ) ;
      BSON_APPEND( *to, NULL, &iter, element ) ;
   }

done:
   return rc ;
error:
   goto done ;
}

#define SET_INVALID_HANDLE( handle ) \
if ( handle )                        \
{                                    \
   *handle = SDB_INVALID_HANDLE ;    \
}

#define LOB_INIT( lob, conn, handle )\
        do\
        {\
           lob->_handleType = SDB_HANDLE_TYPE_LOB ;\
           lob->_connection = ( sdbConnectionHandle )conn ;\
           lob->_sock = handle->_sock ;\
           lob->_contextID = -1 ;\
           lob->_offset = -1 ; \
           lob->_endianConvert = handle->_endianConvert ;\
           lob->_cachedOffset = -1 ;\
           lob->_currentOffset = 0 ;\
           lob->_dataCache = NULL ;\
           lob->_cachedSize = 0 ;\
           lob->_lobSize = 0 ;\
           lob->_pageSize = 0 ;\
           lob->_createTime = 0 ;\
           lob->_modificationTime = 0 ;\
        } while ( FALSE )

#define LOB_ALIGNED_LEN 524288
#define CLIENT_SQL_MAX_LEN 127
#define MAX_USERNAME_LENGTH 256
#define MAX_PASSWORD_LENGTH 256

static BOOLEAN _sdbIsSrand = FALSE ;
static ERROR_ON_REPLY_FUNC _sdbErrorOnReplyCallback = NULL ;

static BOOLEAN isLobReadOnlyMode( INT32 mode )
{
   return ( mode == SDB_LOB_READ || mode == SDB_LOB_SHAREREAD ) ;
}

static BOOLEAN isLobReadWriteMode( INT32 mode )
{
   return ( mode == ( SDB_LOB_SHAREREAD | SDB_LOB_WRITE ) ) ;
}

static BOOLEAN hasLobWriteMode( INT32 mode )
{
   return ( mode == SDB_LOB_WRITE || isLobReadWriteMode( mode ) ) ;
}

static sdbConnectionStruct* _sdbGetConnHandleStruct( sdbConnectionHandle handle )
{
   sdbConnectionStruct *pStruct = (sdbConnectionStruct *)handle ;
   if ( !pStruct )
   {
      return NULL ;
   }
   switch( pStruct->_handleType )
   {
      case SDB_HANDLE_TYPE_CONNECTION:
         break ;
      case SDB_HANDLE_TYPE_COLLECTION:
         pStruct = (sdbConnectionStruct*)(((sdbCollectionStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_CS:
         pStruct = (sdbConnectionStruct*)(((sdbCSStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_CURSOR:
         pStruct = (sdbConnectionStruct*)(((sdbCursorStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_REPLICAGROUP:
         pStruct = (sdbConnectionStruct*)(((sdbRGStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_REPLICANODE:
         pStruct = (sdbConnectionStruct*)(((sdbRNStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_DOMAIN:
         pStruct = (sdbConnectionStruct*)(((sdbDomainStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_LOB:
         pStruct = (sdbConnectionStruct*)(((sdbLobStruct*)handle)->_connection) ;
         break ;
      case SDB_HANDLE_TYPE_DC:
         pStruct = (sdbConnectionStruct*)(((sdbDCStruct*)handle)->_connection) ;
         break ;
      default:
         pStruct = NULL ;
   }
   return pStruct ;
}

#define UNSET_ERROR_BUFF( handle, ptr, size )                                \
   do {                                                                      \
      const CHAR *pBuf                 = NULL ;                              \
      INT32 bufSize                    = 0 ;                                 \
      sdbConnectionStruct *pConnStruct = _sdbGetConnHandleStruct( handle ) ; \
      if ( !pConnStruct ) return ;                                           \
      pBuf    = pConnStruct->_pErrObjBuf ;                                   \
      bufSize = pConnStruct->_errObjBufSize ;                                \
      if ( pBuf &&                                                           \
           (const CHAR*)ptr <= pBuf &&                                       \
           pBuf + bufSize <= (const CHAR*)ptr + size )                       \
      {                                                                      \
         pConnStruct->_pErrObjBuf = NULL ;                                   \
         pConnStruct->_errObjBufSize = 0 ;                                   \
      }                                                                      \
   } while( 0 )

// Local function define
void  _sdbDisconnect_inner ( sdbConnectionHandle handle ) ;

#if defined (_LINUX) || defined (_AIX)
static UINT32 _sdbRandSeed = 0 ;
#endif
static void _sdbSrand ()
{
   if ( !_sdbIsSrand )
   {
#if defined (_WINDOWS)
      srand ( (UINT32) time ( NULL ) ) ;
#elif defined (_LINUX) || defined (_AIX)
      _sdbRandSeed = time ( NULL ) ;
#endif
      _sdbIsSrand = TRUE ;
   }
}

static UINT32 _sdbRand ()
{
   UINT32 randVal = 0 ;
   if ( !_sdbIsSrand )
   {
      _sdbSrand () ;
   }
#if defined (_WINDOWS)
   rand_s ( &randVal ) ;
#elif defined (_LINUX) || defined (_AIX)
   randVal = rand_r ( &_sdbRandSeed ) ;
#endif
   return randVal ;
}

/***********************************************************
 *** note: internal function's parmeters don't check     ***
 ***       because interface function already check it   ***
 ***********************************************************/
#define SDB_CLIENT_DFT_NETWORK_TIMEOUT -1
static INT32 _setRGName ( sdbReplicaGroupHandle handle,
                          const CHAR *pGroupName )
{
   INT32 rc       = SDB_OK ;
   sdbRGStruct *r = (sdbRGStruct*)handle ;

   HANDLE_CHECK( handle, r, SDB_HANDLE_TYPE_REPLICAGROUP );
   if ( ossStrlen ( pGroupName ) > CLIENT_RG_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemset ( r->_replicaGroupName, 0, sizeof(r->_replicaGroupName) ) ;
   ossStrncpy( r->_replicaGroupName, pGroupName, CLIENT_RG_NAMESZ ) ;
   r->_isCatalog = ( 0 == ossStrcmp ( pGroupName, CAT_CATALOG_GROUPNAME ) ) ;
done :
   return rc ;
error :
   goto done ;
}

static INT32 _setCSName ( sdbCSHandle cHandle,
                          const CHAR *pCollectionSpaceName )
{
   INT32 rc       = SDB_OK ;
   sdbCSStruct *s = (sdbCSStruct*)cHandle ;

   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_CS );
   if ( ossStrlen( pCollectionSpaceName) > CLIENT_CS_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemset ( s->_CSName, 0, sizeof(s->_CSName) ) ;
   ossStrncpy( s->_CSName, pCollectionSpaceName, CLIENT_CS_NAMESZ ) ;
done :
   return rc ;
error :
   goto done ;
}

#define CL_FULLNAME_LEN (CLIENT_CS_NAMESZ + CLIENT_COLLECTION_NAMESZ +1)
static INT32 _setCollectionName ( sdbCollectionHandle cHandle,
                                  const CHAR *pCollectionFullName )
{
   INT32 rc                 = SDB_OK ;
   CHAR *pDot               = NULL ;
   CHAR *pDot1              = NULL ;
   CHAR collectionFullName [ CL_FULLNAME_LEN  + 1 ] = {0} ;
   sdbCollectionStruct *s   = (sdbCollectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_COLLECTION );
   ossMemset ( s->_CSName, 0, sizeof ( s->_CSName ) );
   ossMemset ( s->_collectionName, 0, sizeof ( s->_collectionName ) ) ;
   ossMemset ( s->_collectionFullName, 0, sizeof ( s->_collectionFullName ) ) ;
   if ( ossStrlen ( pCollectionFullName ) > CL_FULLNAME_LEN )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy ( collectionFullName, pCollectionFullName, CL_FULLNAME_LEN ) ;
   pDot = (CHAR*)ossStrchr ( (CHAR*)collectionFullName, '.' ) ;
   pDot1 = (CHAR*)ossStrrchr ( (CHAR*)collectionFullName, '.' ) ;
   if ( !pDot || ( pDot != pDot1 ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *pDot++ = 0 ;

   if ( ossStrlen ( collectionFullName ) <= CLIENT_CS_NAMESZ &&
        ossStrlen ( pDot ) <= CLIENT_COLLECTION_NAMESZ )
   {
      ossStrncpy( s->_CSName, collectionFullName, CLIENT_CS_NAMESZ ) ;
      ossStrncpy( s->_collectionName, pDot, CLIENT_COLLECTION_NAMESZ ) ;
      ossStrncpy( s->_collectionFullName, pCollectionFullName,
                  CL_FULLNAME_LEN ) ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

static INT32 _sdbBuildAlterObject ( const CHAR * taskName,
                                    const bson * arguments,
                                    BOOLEAN allowNullArgs,
                                    bson * object )
{
   INT32 rc = SDB_OK ;

   bson_iterator itr ;

   if ( NULL == taskName || NULL == object )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_start_object( object, FIELD_NAME_ALTER ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_string( object, FIELD_NAME_NAME, taskName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   if ( NULL == arguments )
   {
      if ( allowNullArgs )
      {
         rc = bson_append_null( object, FIELD_NAME_ARGS ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = bson_append_start_object( object, FIELD_NAME_ARGS ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      bson_iterator_init ( &itr, arguments ) ;
      while ( BSON_EOO != bson_iterator_next ( &itr ) )
      {
         rc = bson_append_element( object, NULL, &itr ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      rc = bson_append_finish_object( object ) ;
      if ( SDB_OK != rc )
      {
          rc = SDB_DRIVER_BSON_ERROR ;
          goto error ;
      }
   }
   rc = bson_append_finish_object( object ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_finish( object ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

done :
   return rc ;

error :
   goto done ;
}

static INT32 _reallocBuffer ( CHAR **ppBuffer, INT32 *buffersize,
                              INT32 newSize )
{
   INT32 rc              = SDB_OK ;
   CHAR *pOriginalBuffer = NULL ;

   if ( *buffersize < newSize )
   {
      pOriginalBuffer = *ppBuffer ;
      *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer, sizeof(CHAR)*newSize );
      if ( !*ppBuffer )
      {
         *ppBuffer = pOriginalBuffer ;
         rc = SDB_OOM ;
         goto error ;
      }
      *buffersize = newSize ;
   }

   // SEQUOIADBMAINSTREAM-1916
   if ( NULL == *ppBuffer )
   {
      rc = SDB_OOM ;
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

void _msgConvertorReset( sdbConnectionStruct *connection )
{
   connection->_msgConvertor->_hasData = FALSE ;
}

INT32 _msgConvertorEnsureBuff( sdbConnectionStruct *connection, UINT32 size )
{
   INT32 rc = SDB_OK ;

   if ( size > connection->_msgConvertor->_buffSize )
   {
      CHAR *newBuff = (CHAR *)SDB_OSS_REALLOC( connection->_msgConvertor->_buff, size ) ;
      if ( !newBuff )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      connection->_msgConvertor->_buff = newBuff ;
      connection->_msgConvertor->_buffSize = size ;
    }

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgConvertorDowngradeRequest( sdbConnectionStruct *connection, MsgHeader *header )
{
   INT32 rc = SDB_OK ;
   MsgHeaderV1 newHeader ;

   clientMsgHeaderDowngrade( header, &newHeader ) ;
   rc = _msgConvertorEnsureBuff( connection, newHeader.messageLength ) ;
   if ( rc )
   {
      goto error ;
   }

   ossMemcpy( connection->_msgConvertor->_buff, &newHeader, sizeof(MsgHeaderV1) ) ;
   ossMemcpy( connection->_msgConvertor->_buff + sizeof(MsgHeaderV1), (CHAR *)header
              + sizeof(MsgHeader), header->messageLength - sizeof(MsgHeader) ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgConvertorUpgradeReply( sdbConnectionStruct *connection, MsgOpReplyV1 *reply )
{
   INT32 rc = SDB_OK ;
   MsgOpReply newReply ;

   clientMsgReplyHeaderUpgrade( reply, &newReply ) ;
   rc = _msgConvertorEnsureBuff( connection, newReply.header.messageLength ) ;
   if ( rc )
   {
      goto error ;
   }

   ossMemcpy( connection->_msgConvertor->_buff, &newReply, sizeof(MsgOpReply) ) ;
   ossMemcpy( connection->_msgConvertor->_buff + sizeof(MsgOpReply),
              (CHAR *)reply + sizeof(MsgOpReplyV1),
               reply->header.messageLength - sizeof(MsgOpReplyV1) ) ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgConvertorPush( sdbConnectionStruct *connection, const CHAR *data)
{
   INT32 rc = SDB_OK ;
   MsgHeader *msg = (MsgHeader *)data ;

   if ( MSG_COMM_EYE_DEFAULT == ( msg->eye ) )
   {
      rc = _msgConvertorDowngradeRequest( connection, (MsgHeader *)data ) ;
   }
   else
   {
      rc = _msgConvertorUpgradeReply( connection, (MsgOpReplyV1 *)data ) ;
   }
   if ( rc )
   {
      goto error ;
   }
   connection->_msgConvertor->_hasData = TRUE ;

done:
   return rc ;
error:
   goto done ;
}

INT32 _msgConvertorOutput( sdbConnectionStruct *connection, CHAR **data, UINT32 *len)
{
   *len = 0 ;
   *data = NULL ;
   if ( connection->_msgConvertor->_hasData )
   {
      *len = *(INT32 *)connection->_msgConvertor->_buff ;
      *data = connection->_msgConvertor->_buff ;
   }
   return SDB_OK ;
}

static INT32 _send1 ( sdbConnectionHandle cHandle, Socket* sock,
                      const CHAR *pMsg, INT32 len )
{
   INT32 rc = SDB_OK ;
   INT32 sentSize = 0 ;
   INT32 totalSentSize = 0 ;

   if ( NULL == sock )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   while( len > totalSentSize )
   {
      rc = clientSend ( sock, pMsg + totalSentSize,
                        len - totalSentSize, &sentSize,
                        SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
      totalSentSize += sentSize ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
done:
   return rc ;
error :
   goto done ;
}

static INT32 _send ( sdbConnectionHandle cHandle, Socket* sock,
                     const MsgHeader *msg, BOOLEAN endianConvert )
{
   INT32 rc  = SDB_OK ;
   INT32 len = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;
   CHAR *pbuffer = (CHAR*)msg ;

   if ( NULL == sock )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( connection->_msgConvertor )
   {
      UINT32 tmpLen = 0 ;
      _msgConvertorReset( connection ) ;
      rc = _msgConvertorPush( connection, (const CHAR *)pbuffer ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _msgConvertorOutput( connection, &pbuffer, &tmpLen ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( tmpLen != *(UINT32 *)pbuffer )
      {
          rc = SDB_SYS ;
          goto error ;
      }
   }

   ossEndianConvertIf4 ( *(SINT32*)pbuffer, len, endianConvert ) ;
   rc = _send1 ( cHandle, sock, (const CHAR*)pbuffer, len ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error :
   goto done ;
}

static INT32 _recv ( sdbConnectionHandle cHandle, Socket* sock,
                     MsgHeader **msg, INT32 *size,
                     BOOLEAN endianConvert )
{
   INT32 rc        = SDB_OK ;
   INT32 len       = 0 ;
   INT32 realLen   = 0 ;
   INT32 receivedLen = 0 ;
   INT32 totalReceivedLen = 0 ;
   CHAR **ppBuffer = (CHAR**)msg ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;
   INT32 minReplySize = ( SDB_PROTOCOL_VER_1 == connection->_peerProtocolVersion ) ?
                        sizeof(MsgOpReplyV1) : sizeof(MsgOpReply) ;

   if ( NULL == sock )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   while ( TRUE )
   {
      // get length first
      rc = clientRecv ( sock, ((CHAR*)&len) + totalReceivedLen,
                        sizeof(len) - totalReceivedLen, &receivedLen,
                        SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
      totalReceivedLen += receivedLen ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         goto error ;
      }

#if defined( _LINUX ) || defined (_AIX)
      #if defined (_AIX)
         #define TCP_QUICKACK TCP_NODELAYACK
      #endif
      // quick ack
      {
         INT32 i = 1 ;
         setsockopt( clientGetRawSocket ( sock ),
                     IPPROTO_TCP, TCP_QUICKACK,
                     (void*)&i, sizeof(i) ) ;
      }
#endif
      break ;
   }
   ossEndianConvertIf4 ( len, realLen, endianConvert ) ;
   if ( realLen < minReplySize )
   {
       rc = SDB_NET_BROKEN_MSG ;
       goto error ;
   }

   rc = _reallocBuffer ( ppBuffer, size, realLen+1 ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // use the original len before convert
   *(SINT32*)(*ppBuffer) = len ;
   receivedLen = 0 ;
   totalReceivedLen = 0 ;
   while ( TRUE )
   {
      rc = clientRecv ( sock, &(*ppBuffer)[sizeof(realLen) + totalReceivedLen],
                        realLen - sizeof(realLen) - totalReceivedLen,
                        &receivedLen,
                        SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
      totalReceivedLen += receivedLen ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      break ;
   }

   if ( connection->_msgConvertor )
   {
      UINT32 tmpLen = 0 ;
      CHAR *tmpPtr = NULL ;
      _msgConvertorReset( connection ) ;
      rc = _msgConvertorPush( connection, *ppBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _msgConvertorOutput( connection, &tmpPtr, &tmpLen ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( tmpLen != *(UINT32 *)tmpPtr )
      {
          rc = SDB_SYS ;
          goto error ;
      }
      if ( tmpLen > (UINT32)realLen )
      {
         rc = _reallocBuffer( ppBuffer, size, tmpLen ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      ossMemcpy( *ppBuffer, tmpPtr, tmpLen ) ;
    }

done :
   return rc ;
error :
   goto done ;
}

static INT32 _sendAndRecv ( sdbConnectionHandle cHandle, Socket* sock,
                            const MsgHeader *sendMsg,
                            MsgHeader **recvMsg, INT32 *size,
                            BOOLEAN needRecv,
                            BOOLEAN endianConvert )
{
   INT32 rc = SDB_OK ;
   BOOLEAN hasLock = FALSE ;
   BOOLEAN isNeedDisconnect = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION );

   // check arguments
   if ( NULL == sock )
   {
      rc = SDB_NOT_CONNECTED ;
      goto error ;
   }
   if ( NULL == sendMsg )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // lock before send msg
   ossMutexLock( &connection->_sockMutex ) ;
   hasLock = TRUE ;

   // send msg
   rc = _send( cHandle, sock, sendMsg, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      // SEQUOIADBMAINSTREAM-1916 may be have send half of the message
      isNeedDisconnect = TRUE ;
      goto error ;
   }

   // recv msg
   if ( TRUE == needRecv )
   {
      rc = _recv( cHandle, sock, recvMsg, size, endianConvert ) ;
      if ( SDB_OK != rc )
      {
         // repsone may be still in the inputstream
         isNeedDisconnect = TRUE ;
         goto error ;
      }
   }

done:
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return rc ;

error:
   if ( SDB_NETWORK_CLOSE == rc ||
        SDB_NETWORK == rc ||
        isNeedDisconnect )
   {
      _sdbDisconnect_inner( cHandle ) ;
   }

   goto done ;
}
static INT32 _getLastResultObj( sdbConnectionHandle cHandle, bson *result)
{
   INT32 rc                         = SDB_OK ;
   sdbConnectionStruct *pConnStruct = ( sdbConnectionStruct* )cHandle ;
   bson localobj ;

   bson_init( &localobj ) ;
   if ( NULL == result )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // check handle
   HANDLE_CHECK( cHandle, pConnStruct, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( pConnStruct->_pResultBuf && pConnStruct->_resultBufsize >= BSON_MIN_SIZE &&
       *( INT32* )pConnStruct->_pResultBuf >= BSON_MIN_SIZE )
   {
      rc = bson_init_finished_data( &localobj, pConnStruct->_pResultBuf ) ;
      if ( rc )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }
      rc = bson_copy( result, &localobj ) ;
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_DMS_EOC ;
      goto error ;
   }
done :
   bson_destroy( &localobj ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _extractErrorObj( const CHAR *pErrorBuf,
                               INT32 *pFlag,
                               const CHAR **ppErr,
                               const CHAR **ppDetail )
{
   INT32 rc = SDB_OK ;
   bson localobj ;
   BOOLEAN bsoninit = FALSE ;
   bson_iterator it ;

   BSON_INIT( localobj ) ;

   if ( !pErrorBuf )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_init_finished_data( &localobj, pErrorBuf ) ;
   if ( rc )
   {
      rc = SDB_CORRUPTED_RECORD ;
      goto error ;
   }

   if ( pFlag && BSON_INT == bson_find( &it, &localobj, OP_ERRNOFIELD ) )
   {
      *pFlag = bson_iterator_int( &it ) ;
   }
   if ( ppErr && BSON_STRING == bson_find( &it, &localobj,
                                           OP_ERRDESP_FIELD ) )
   {
      *ppErr = bson_iterator_string( &it ) ;
   }
   if ( ppDetail && BSON_STRING == bson_find( &it, &localobj,
                                              OP_ERR_DETAIL ) )
   {
      *ppDetail = bson_iterator_string( &it ) ;
   }

done:
   BSON_DESTROY( localobj ) ;
   return rc ;
error:
   goto done ;
}

static INT32 _extract ( sdbConnectionHandle cHandle,
                        MsgHeader *msg, INT32 size,
                        SINT64 *contextID,
                        BOOLEAN endianConvert )
{
   INT32 rc          = SDB_OK ;
   INT32 replyFlag   = -1 ;
   INT32 numReturned = -1 ;
   INT32 startFrom   = -1 ;
   sdbConnectionStruct *pConnStruct = (sdbConnectionStruct*)cHandle ;
   CHAR *pBuffer     = (CHAR*)msg ;

   // check handle
   HANDLE_CHECK( cHandle, pConnStruct, SDB_HANDLE_TYPE_CONNECTION );

   rc = clientExtractReply ( pBuffer, &replyFlag, contextID,
                             &startFrom, &numReturned, endianConvert ) ;

   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = replyFlag ;
   pConnStruct->_pErrObjBuf = NULL ;
   pConnStruct->_errObjBufSize = 0 ;
   pConnStruct->_pResultBuf = NULL ;
   pConnStruct->_resultBufsize = 0 ;

   if ( SDB_OK != replyFlag && SDB_DMS_EOC != replyFlag )
   {
      INT32 dataOff     = 0 ;
      INT32 dataSize    = 0 ;
      const CHAR *pErr  = NULL ;
      const CHAR *pDetail = NULL ;

      dataOff = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
      dataSize = msg->messageLength - dataOff ;
      /// save error info
      if ( dataSize > 0 )
      {
         pConnStruct->_pErrObjBuf = ( const CHAR* )msg + dataOff ;
         pConnStruct->_errObjBufSize = dataSize ;
         if ( _sdbErrorOnReplyCallback &&
              SDB_OK == _extractErrorObj( pConnStruct->_pErrObjBuf,
                                          NULL, &pErr, &pDetail ) )
         {
            (*_sdbErrorOnReplyCallback)( pConnStruct->_pErrObjBuf,
                                         (UINT32)(pConnStruct->_errObjBufSize),
                                         replyFlag, pErr, pDetail ) ;
         }
      }
   }
   /*
      Temp solution. Insert result return the LastGenerateId
   */
   else if ( SDB_OK == replyFlag && 1 == numReturned &&
             ( MSG_BS_INSERT_RES      == msg->opCode ||
               MSG_BS_UPDATE_RES      == msg->opCode ||
               MSG_BS_DELETE_RES      == msg->opCode ||
               MSG_BS_SQL_RES         == msg->opCode ||
               MSG_AUTH_VERIFY1_RES   == msg->opCode   ) )
   {
      INT32 dataOff          =0 ;
      INT32 dataSize         =0 ;
      MsgOpReply *pTmpReply  = ( MsgOpReply* )( pBuffer ) ;
      dataOff  = ossRoundDownToMultipleX( sizeof( MsgOpReply ), 4 ) ;
      dataSize = pTmpReply->header.messageLength - dataOff ;
      /// save result info
      if ( dataSize > 0 )
      {
         pConnStruct->_pResultBuf    = ( pBuffer ) + dataOff ;
         pConnStruct->_resultBufsize = dataSize ;
         /// should truncate the message size
         pTmpReply->header.messageLength = sizeof( MsgOpReply ) ;
         pTmpReply->numReturned = 0 ;
      }
   }
done :
   return rc ;
error :
   goto done ;
}

static INT32 _extractEval ( MsgHeader *msg, INT32 size,
                            SINT64 *contextID, SDB_SPD_RES_TYPE *type,
                            BOOLEAN *result, bson *errmsg,
                            BOOLEAN endianConvert )
{
   INT32 rc          = SDB_OK ;
   INT32 tmpRc       = SDB_OK ;
   INT32 replyFlag   = -1 ;
   INT32 numReturned = -1 ;
   INT32 startFrom   = -1 ;
   CHAR *pBuffer   = (CHAR*)msg ;
   MsgOpReply *replyHeader = NULL ;
   bson_iterator rType ;
   bson localObj ;
   bson runInfo ;
   bson_init( &localObj ) ;
   bson_init( &runInfo ) ;

   replyHeader = (MsgOpReply *)(pBuffer) ;

   rc = clientExtractReply ( pBuffer, &replyFlag, contextID,
                             &startFrom, &numReturned, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( SDB_OK != replyFlag )
   {
      *result = FALSE ;
      rc = replyFlag ;
      if ( errmsg && sizeof(MsgOpReply) != replyHeader->header.messageLength )
      {
         tmpRc = bson_init_finished_data( &localObj, pBuffer + sizeof(MsgOpReply) ) ;
         if ( SDB_OK != tmpRc )
         {
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         // copy to output result
         tmpRc = bson_copy ( errmsg, &localObj ) ;
         if ( SDB_OK != tmpRc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      goto error ;
   }
   else if ( 1 == numReturned &&
             (INT32)(sizeof(MsgOpReply)) < replyHeader->header.messageLength )
   {
      bson_init_finished_data( &runInfo, pBuffer + sizeof(MsgOpReply) ) ;
      if ( BSON_INT != bson_find( &rType, &runInfo, FIELD_NAME_RTYPE ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      *type = ( SDB_SPD_RES_TYPE )( bson_iterator_int( &rType ) ) ;
      *result = TRUE ;
   }
   else
   {
      *result = FALSE ;
      rc = SDB_SYS ;
      goto error ;
   }

done :
   bson_destroy( &localObj ) ;
   bson_destroy( &runInfo ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _readNextBuffer ( sdbCursorHandle cursor )
{
   INT32 rc          = SDB_OK ;
   SINT64 lcontextID = 0 ;
   sdbCursorStruct *pCursor = ( sdbCursorStruct* )cursor ;

   HANDLE_CHECK( cursor, pCursor, SDB_HANDLE_TYPE_CURSOR ) ;

   // if contextid is not invalid
   if ( -1 == pCursor->_contextID )
   {
      rc = SDB_DMS_EOC ;
      goto error ;
   }

   rc = clientBuildGetMoreMsg ( &pCursor->_pSendBuffer,
                                &pCursor->_sendBufferSize, -1,
                                pCursor->_contextID,
                                0, pCursor->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( pCursor->_connection, pCursor->_sock,
                      (MsgHeader*)pCursor->_pSendBuffer,
                      (MsgHeader**)&pCursor->_pReceiveBuffer,
                      &pCursor->_receiveBufferSize,
                      TRUE, pCursor->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( pCursor->_connection,
                  (MsgHeader*)pCursor->_pReceiveBuffer,
                  pCursor->_receiveBufferSize,
                  &lcontextID,
                  pCursor->_endianConvert ) ;
   if ( SDB_OK == rc )
   {
      // check return opcode
      CHECK_RET_MSGHEADER( pCursor->_pSendBuffer, pCursor->_pReceiveBuffer,
                           pCursor->_connection ) ;
   }
   if ( SDB_DMS_EOC == rc )
   {
      // check return opcode
      CHECK_RET_MSGHEADER( pCursor->_pSendBuffer, pCursor->_pReceiveBuffer,
                           pCursor->_connection ) ;
      rc = SDB_DMS_EOC ;
   }
   if ( SDB_OK != rc )
   {
      MsgOpReply *pReply = (MsgOpReply*)pCursor->_pReceiveBuffer ;
      if ( !pReply )
      {
         goto error ;
      }
      if ( SDB_OK != pReply->flags )
      {
         lcontextID = -1 ;
      }
      goto error ;
   }
   else if ( -1 == lcontextID )
   {
      // early closed
      pCursor->_contextID = -1 ;
   }

done :
   return rc ;
error :
   if ( SDB_DMS_EOC != rc )
   {
      sdbCloseCursor( cursor ) ;
   }
   goto done ;
}

static INT32 _runCommand ( sdbConnectionHandle cHandle, Socket* sock,
                           CHAR **ppSendBuffer, INT32 *sendBufferSize,
                           CHAR **ppReceiveBuffer, INT32 *receiveBufferSize,
                           BOOLEAN endianConvert, const CHAR *pString,
                           bson *arg1, bson *arg2, bson *arg3, bson *arg4 )
{
   INT32 rc         = SDB_OK ;
   SINT64 contextID = 0 ;

   rc = clientBuildQueryMsg ( ppSendBuffer,
                              sendBufferSize,
                              pString, 0, 0, -1, -1,
                              arg1, arg2, arg3, arg4, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, sock, (MsgHeader*)(*ppSendBuffer),
                      (MsgHeader**)ppReceiveBuffer, receiveBufferSize,
                      TRUE, endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle, (MsgHeader *)*ppReceiveBuffer, *receiveBufferSize,
                  &contextID, endianConvert) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check wether the return message is what we want or not
   CHECK_RET_MSGHEADER( *ppSendBuffer, *ppReceiveBuffer, cHandle ) ;

done :
   return rc ;
error :
   goto done ;
}

static INT32 requestSysInfo ( sdbConnectionStruct *connection )
{
   INT32 rc               = SDB_OK ;
   INT32 receivedLen      = 0 ;
   INT32 totalReceivedLen = 0 ;
   MsgSysInfoReply reply ;

   connection->_endianConvert = FALSE;
   connection->_peerProtocolVersion = SDB_PROTOCOL_VER_INVALID ;
   connection->_authVersion = 0 ;
   rc = clientBuildSysInfoRequest ( (CHAR**)&connection->_pSendBuffer,
                                    &connection->_sendBufferSize ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _send1 ( (sdbConnectionHandle)connection, connection->_sock,
                 connection->_pSendBuffer, sizeof( MsgSysInfoRequest ) ) ;
   if ( SDB_OK != rc )
   {
      // send failed
      // no need to send disconnect, close socket directly
      _sdbDisconnect_inner( (sdbConnectionHandle)connection ) ;
      goto error ;
   }

   while ( TRUE )
   {
      rc = clientRecv ( connection->_sock, ((CHAR*)&reply) + totalReceivedLen,
                        sizeof(MsgSysInfoReply) - totalReceivedLen,
                        &receivedLen,
                        SDB_CLIENT_DFT_NETWORK_TIMEOUT ) ;
      totalReceivedLen += receivedLen ;
      if ( SDB_TIMEOUT == rc )
      {
         continue ;
      }
      if ( SDB_OK != rc )
      {
         _sdbDisconnect_inner( (sdbConnectionHandle)connection ) ;
         goto error ;
      }
      break ;
   }

   rc = clientExtractSysInfoReply ( (CHAR*)&reply,
                                    &(connection->_endianConvert),
                                    NULL, &(connection->_authVersion),
                                    &(connection->_peerProtocolVersion),
                                    NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

static INT32 _addHandle ( Node **ptr, ossValuePtr handle )
{
   INT32 rc = SDB_OK ;
   Node *p  = NULL ;

   p = (Node*)SDB_OSS_MALLOC( sizeof(Node) ) ;
   if ( !p )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset ( p, 0, sizeof(Node) ) ;
   p->data = handle ;
   p->next = NULL ;

   // if it's the 1st time to add handle
   if ( !(*ptr) )
      *ptr = p ;
   // add handle to the node header
   else
   {
      p->next = *ptr ;
      *ptr = p ;
   }
done :
   return rc ;
error :
   goto done ;
}

static INT32 _removeHandle ( Node **ptr, ossValuePtr handle,
                             Node **ptrRemoved )
{
   Node *pcurrent  = NULL ;
   Node *pprevious = NULL ;

   pcurrent = *ptr ;
   while( pcurrent )
   {
      if ( handle == pcurrent->data )
      {
         break;
      }

      pprevious = pcurrent ;
      pcurrent = pcurrent->next ;
   }

   if ( !pcurrent )
   {
      goto done ;
   }
   // test wether the first node is the one we interested
   if ( !pprevious )
   {
      *ptr = pcurrent->next ;
   }
   else
   {
      pprevious->next = pcurrent->next ;
   }

   if ( !ptrRemoved )
   {
      SDB_OSS_FREE ( pcurrent ) ;
   }
   else
   {
      *ptrRemoved = pcurrent ;
   }

done :
   return SDB_OK ;
}

static INT32 _regSocket( ossValuePtr cHandle, Socket** pSock )
{
   INT32 rc        = SDB_OK ;
   BOOLEAN hasLock = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;

   // pass invalid socket
   if ( NULL == *pSock )
   {
      goto done ;
   }

   ossMutexLock( &connection->_sockMutex ) ;
   hasLock = TRUE ;

   // if client has disconnected, stop registing
   if ( NULL == connection->_sock )
   {
      goto done ;
   }

   rc = _addHandle ( &connection->_sockets, (ossValuePtr)pSock ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _unregSocket( ossValuePtr cHandle, Socket** pSock )
{
   INT32 rc                        = SDB_OK ;
   BOOLEAN hasLock                 = FALSE ;
   Node *ptrRemoved                = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;

   if ( NULL == *pSock )
   {
      goto done ;
   }

   ossMutexLock( &connection->_sockMutex ) ;
   hasLock = TRUE ;
   // if client has disconnected, stop unregisting
   if ( NULL == connection->_sock )
   {
      goto done ;
   }

   rc = _removeHandle ( &connection->_sockets, (ossValuePtr)pSock,
                        &ptrRemoved ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( ptrRemoved )
   {
      *(Socket**)ptrRemoved->data = NULL ;
      SDB_OSS_FREE( ptrRemoved ) ;
   }

done :
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _regCursor ( ossValuePtr cHandle, sdbCursorHandle cursorHandle )
{
   INT32 rc        = SDB_OK ;
   BOOLEAN hasLock = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;

   _regSocket( cHandle, &((sdbCursorStruct*)cursorHandle)->_sock ) ;

   ossMutexLock( &connection->_sockMutex ) ;
   hasLock = TRUE ;

   // if client has disconnected, stop registing
   if ( NULL == connection->_sock )
   {
      goto done ;
   }
   // add to connectionStruct
   rc = _addHandle ( &connection->_cursors, cursorHandle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _unregCursor ( ossValuePtr cHandle, sdbCursorHandle cursorHandle )
{
   INT32 rc                        = SDB_OK ;
   BOOLEAN hasLock                 = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;

   _unregSocket( cHandle, &((sdbCursorStruct*)cursorHandle)->_sock ) ;

   ossMutexLock( &connection->_sockMutex ) ;
   hasLock = TRUE ;
   // if client has disconnected, stop unregisting
   if ( NULL == connection->_sock )
   {
      goto done ;
   }

   rc = _removeHandle ( &connection->_cursors, cursorHandle, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _buildEmptyCursor( sdbConnectionHandle cHandle,
                                sdbCursorHandle *handle )
{
   INT32 rc                = SDB_OK ;
   sdbCursorStruct *cursor = NULL ;
   sdbConnectionStruct *db = (sdbConnectionStruct *)cHandle ;

   // check
   HANDLE_CHECK( cHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;

   // build empty cursor
   ALLOC_HANDLE( cursor, sdbCursorStruct ) ;
   INIT_CURSOR( cursor, cHandle, db, -1 ) ;

   // register cursor in connection handle
   rc = _regCursor ( cHandle, (sdbCursorHandle)cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // return
   *handle = (sdbCursorHandle)cursor ;
done:
   return rc ;
error:
   if ( NULL != cursor )
   {
      sdbReleaseCursor( (sdbCursorHandle)cursor ) ;
   }
   goto done ;
}


static INT32 _getRetInfo ( sdbConnectionHandle cHandle,
                           CHAR **ppBuffer, INT32 *size,
                           SINT64 contextID,
                           sdbCursorHandle *pCursor )
{
   INT32 rc                = SDB_OK ;
   sdbCursorHandle cursor  = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *db = (sdbConnectionStruct *)cHandle ;

   // check
   HANDLE_CHECK( cHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pCursor )
   {
      goto done ;
   }
   // when pCursor != NULL, we mush return a cursor
   rc = _buildEmptyCursor( cHandle, &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   // set contextID got from engine
   ((sdbCursorStruct *)cursor)->_contextID = contextID ;
   // set the receive buffer into cursor
   if ( ((UINT32)((MsgHeader*)*ppBuffer)->messageLength) >
           ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
   {
      ((sdbCursorStruct *)cursor)->_pReceiveBuffer = *ppBuffer ;
      *ppBuffer = NULL ;
      ((sdbCursorStruct *)cursor)->_receiveBufferSize = *size ;
      *size = 0 ;
   }
   // return cursor
   *pCursor = cursor ;

done:
   return rc ;
error:
   goto done ;
}

static INT32 _runCommand2 ( sdbConnectionHandle cHandle,
                            CHAR **ppSendBuffer,
                            INT32 *sendBufferSize,
                            CHAR **ppReceiveBuffer,
                            INT32 *receiveBufferSize,
                            const CHAR *pString,
                            SINT32 flag,
                            UINT64 reqID,
                            SINT64 numToSkip,
                            SINT64 numToReturn,
                            const bson *arg1,
                            const bson *arg2,
                            const bson *arg3,
                            const bson *arg4,
                            sdbCursorHandle *handle
                            )
{
   INT32 rc                = SDB_OK ;
   SINT64 contextID        = 0 ;
   sdbConnectionStruct *db = (sdbConnectionStruct *)cHandle ;

   // check
   HANDLE_CHECK( cHandle, db, SDB_HANDLE_TYPE_CONNECTION ) ;

   // when need return cursor, add these flags for optimization
   if ( handle )
   {
      flag |= FLG_QUERY_WITH_RETURNDATA ;
      flag |= FLG_QUERY_CLOSE_EOF_CTX ;
   }

   // build message
   rc = clientBuildQueryMsg ( ppSendBuffer,
                              sendBufferSize,
                              pString, flag, reqID, numToSkip, numToReturn,
                              arg1, arg2, arg3, arg4, db->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, db->_sock, (MsgHeader*)(*ppSendBuffer),
                      (MsgHeader**)ppReceiveBuffer, receiveBufferSize,
                      TRUE, db->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle, (MsgHeader *)*ppReceiveBuffer, *receiveBufferSize,
                  &contextID, db->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check whether the return message is what we want or not
   CHECK_RET_MSGHEADER( *ppSendBuffer, *ppReceiveBuffer, cHandle ) ;

   // try to get retObj
   rc = _getRetInfo( cHandle, ppReceiveBuffer, receiveBufferSize,
                     contextID, handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

static void _resetBsonToEmpty( bson *b )
{
   if ( NULL != b )
   {
      bson_destroy( b ) ;
      bson_init( b ) ;
      bson_finish( b ) ;
   }
}

#define TRACE_FIELD_SEP ','
static INT32 sdbTraceStrtok ( bson *obj, CHAR *pLine )
{
   INT32 rc     = SDB_OK ;
   INT32 len    = 0 ;
   INT32 pos    = 0 ;
   CHAR *pStart = NULL ;
   CHAR pos_buf[128] = { 0 } ;
   CHAR  *pBuff = NULL ;
   CHAR  *pTemp = NULL ;

   if ( !pLine )
   {
      goto done ;
   }
   len = ossStrlen( pLine ) ;
   pBuff = (CHAR*)SDB_OSS_MALLOC( len + 1 ) ;
   if ( NULL == pBuff )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossStrncpy( pBuff, pLine, len + 1 ) ;

   pStart = ossStrtok( pBuff, ", ", &pTemp ) ;
   while ( pStart != NULL )
   {
      ossSnprintf ( pos_buf, sizeof(pos_buf), "%d", pos++ ) ;
      BSON_APPEND( *obj, pos_buf, pStart, string ) ;
      pBuff = NULL ;
      pStart = ossStrtok( pBuff, ", ", &pTemp ) ;
   }

done :
   if (pBuff)
   {
      SDB_OSS_FREE( pBuff ) ;
      pBuff = NULL ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 __sdbUpdate ( sdbCollectionHandle cHandle,
                           SINT32 flag,
                           bson *rule,
                           bson *condition,
                           bson *hint,
                           bson *pResult )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION );
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( pResult )
   {
      flag |= FLG_UPDATE_RETURNNUM ;
   }
   rc = clientBuildUpdateMsg ( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                cs->_collectionFullName, flag, 0, condition,
                                rule, hint, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock, (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer, cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   //get result
   if ( pResult )
   {
      bson_destroy( pResult ) ;
      bson_init( pResult ) ;
      _getLastResultObj( cs->_connection, pResult ) ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   if ( pResult )
   {
      _resetBsonToEmpty( pResult ) ;
   }
   goto done ;
}

static INT32 _sdbStartStopNode ( sdbNodeHandle cHandle,
                                 BOOLEAN start )
{
   INT32 rc         = SDB_OK ;
   BOOLEAN bsoninit = FALSE ;
   bson configuration ;
   sdbRNStruct *r   = (sdbRNStruct*)cHandle ;

   BSON_INIT( configuration ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICANODE);
   BSON_APPEND( configuration, CAT_HOST_FIELD_NAME,
                r->_hostName, string ) ;
   BSON_APPEND( configuration, PMD_OPTION_SVCNAME, r->_serviceName, string ) ;
   BSON_FINISH( configuration ) ;
   rc = _runCommand ( r->_connection, r->_sock, &r->_pSendBuffer,
                      &r->_sendBufferSize,
                      &r->_pReceiveBuffer,
                      &r->_receiveBufferSize,
                      r->_endianConvert,
                      start?
                         (CMD_ADMIN_PREFIX CMD_NAME_STARTUP_NODE) :
                         (CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_NODE),
                      &configuration,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( configuration ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _sdbRGExtractNode ( sdbReplicaGroupHandle cHandle,
                                 sdbNodeHandle *handle,
                                 const CHAR *data,
                                 BOOLEAN endianConvert )
{
   INT32 rc       = SDB_OK ;
   sdbRNStruct *r = NULL ;
   sdbRGStruct *s = (sdbRGStruct *)cHandle ;

   // build a node handle
   ALLOC_HANDLE( r, sdbRNStruct ) ;
   r->_handleType = SDB_HANDLE_TYPE_REPLICANODE ;
   r->_connection = s->_connection ;
   r->_sock = s->_sock ;
   r->_endianConvert = endianConvert ;

   _regSocket( s->_connection, &r->_sock ) ;

   rc = clientReplicaGroupExtractNode ( data,
                                        r->_hostName,
                                        CLIENT_MAX_HOSTNAME,
                                        r->_serviceName,
                                        CLIENT_MAX_SERVICENAME,
                                        &r->_nodeID ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   ossStrncpy ( r->_nodeName, r->_hostName, CLIENT_MAX_HOSTNAME ) ;
   ossStrncat ( r->_nodeName, NODE_NAME_SERVICE_SEP, 1 ) ;
   ossStrncat ( r->_nodeName, r->_serviceName,
                CLIENT_MAX_SERVICENAME ) ;

   *handle = (sdbNodeHandle)r ;
done :
   return rc ;
error :
   if ( r )
   {
      SDB_OSS_FREE ( r ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

static INT32 _sdbGetList ( sdbConnectionHandle cHandle,
                           INT32 listType,
                           bson *condition,
                           bson *selector,
                           bson *orderBy,
                           bson *hint,
                           INT64 numToSkip,
                           INT64 numToReturn,
                           sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   const CHAR *p                   = NULL ;
   sdbConnectionStruct *connection = NULL ;

   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   switch ( listType )
   {
   case SDB_LIST_CONTEXTS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXTS ;
      break ;
   case SDB_LIST_CONTEXTS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CONTEXTS_CURRENT ;
      break ;
   case SDB_LIST_SESSIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSIONS ;
      break ;
   case SDB_LIST_SESSIONS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_SESSIONS_CURRENT ;
      break ;
   case SDB_LIST_COLLECTIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONS ;
      break ;
   case SDB_LIST_COLLECTIONSPACES :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_COLLECTIONSPACES ;
      break ;
   case SDB_LIST_STORAGEUNITS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_STORAGEUNITS ;
      break ;
   case SDB_LIST_GROUPS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPS ;
      break ;
   case SDB_LIST_STOREPROCEDURES :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_PROCEDURES ;
      break ;
   case SDB_LIST_DOMAINS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_DOMAINS ;
      break ;
   case SDB_LIST_TASKS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_TASKS ;
      break ;
   case SDB_LIST_TRANSACTIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_TRANSACTIONS ;
      break ;
   case SDB_LIST_TRANSACTIONS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_TRANSACTIONS_CUR ;
      break ;
   case SDB_LIST_SVCTASKS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_SVCTASKS ;
      break ;
   case SDB_LIST_CS_IN_DOMAIN :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CS_IN_DOMAIN ;
      break ;
   case SDB_LIST_CL_IN_DOMAIN :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CL_IN_DOMAIN ;
      break ;
   case SDB_LIST_SEQUENCES :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_SEQUENCES ;
	  break ;
   case SDB_LIST_USERS :
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_USERS ;
      break ;
   case SDB_LIST_BACKUPS:
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_BACKUPS ;
      break ;
   case SDB_LIST_DATASOURCES:
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_DATASOURCES ;
      break ;
   case SDB_LIST_RECYCLEBIN:
      p = CMD_ADMIN_PREFIX CMD_NAME_LIST_RECYCLEBIN ;
      break ;
   default :
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   connection = (sdbConnectionStruct *)cHandle ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      p,
                      0, 0, numToSkip, numToReturn,
                      condition, selector, orderBy, hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   /// set output result
   *handle = cursor ;

done:
   return rc ;
error:
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

static INT32 _sdbGetReplicaGroupDetail ( sdbReplicaGroupHandle cHandle,
                                         bson *result )
{
   INT32 rc               = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   CHAR *pName            = FIELD_NAME_GROUPNAME ;
   sdbRGStruct *r         = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit       = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   BSON_APPEND( newObj, pName, r->_replicaGroupName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _sdbGetList ( r->_connection,
                      SDB_LIST_GROUPS,
                      &newObj, NULL, NULL, NULL,
                      0, -1,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbNext ( cursor, result ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }

   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _sdbGetReplicaGroup( sdbConnectionHandle cHandle,
                                  bson condition,
                                  sdbReplicaGroupHandle *handle )
{
   INT32 rc                        = SDB_OK;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbRGStruct *r                  = NULL ;
   BOOLEAN bsoninit                = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   bson result ;

   BSON_INIT( result ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = sdbGetList ( cHandle, SDB_LIST_GROUPS, &condition, NULL, NULL,
                     &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( SDB_OK == ( rc = sdbNext ( cursor, &result ) ) )
   {
      bson_iterator it ;
      const CHAR *pGroupName = NULL ;
      if ( BSON_STRING != bson_find ( &it, &result, CAT_GROUPNAME_NAME ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      pGroupName = bson_iterator_string ( &it ) ;

      ALLOC_HANDLE( r, sdbRGStruct ) ;
      r->_handleType    = SDB_HANDLE_TYPE_REPLICAGROUP ;
      r->_connection    = cHandle ;
      r->_sock          = connection->_sock ;
      r->_endianConvert = connection->_endianConvert ;
      rc = _setRGName ( (sdbReplicaGroupHandle)r, pGroupName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _regSocket( cHandle, &r->_sock ) ;

      if ( !ossStrcmp ( pGroupName, CATALOG_GROUPNAME ) )
      {
         r->_isCatalog = TRUE ;
      }
      *handle = (sdbReplicaGroupHandle)r ;
   }
   else if ( SDB_DMS_EOC != rc )
   {
      goto error ;
   }
   else
   {
      rc = SDB_CLS_GRP_NOT_EXIST ;
      goto error ;
   }

done :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }

   BSON_DESTROY( result ) ;
   return rc;
error:
   if ( r )
   {
      SDB_OSS_FREE ( r ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

// Internal functions for session attribute cache
static void _sdbClearSessionAttrCache ( sdbConnectionStruct * connection,
                                        BOOLEAN needLock )
{
   if ( NULL == connection )
   {
      return ;
   }

   if ( needLock )
   {
      ossMutexLock( &connection->_sockMutex ) ;
   }
   if ( NULL != connection->_attributeCache )
   {
      bson_dispose( connection->_attributeCache ) ;
      connection->_attributeCache = NULL ;
   }
   if ( needLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
}

static void _sdbSetSessionAttrCache ( sdbConnectionStruct * connection,
                                      bson * attribute )
{
   if ( NULL == connection )
   {
      return ;
   }
   if ( NULL == attribute )
   {
      _sdbClearSessionAttrCache( connection, TRUE ) ;
      return ;
   }
   ossMutexLock( &connection->_sockMutex ) ;
   if ( NULL == connection->_attributeCache )
   {
      connection->_attributeCache = bson_create() ;
   }
   if ( NULL != connection->_attributeCache )
   {
      INT32 rc = bson_copy( connection->_attributeCache, attribute ) ;
      if ( BSON_ERROR == rc )
      {
         bson_dispose( connection->_attributeCache ) ;
         connection->_attributeCache = NULL ;
      }
   }
   ossMutexUnlock( &connection->_sockMutex ) ;
}

static BOOLEAN _sdbGetSessionAttrCache ( sdbConnectionStruct * connection,
                                         bson * attribute )
{
   BOOLEAN gotCache = FALSE ;

   if ( NULL != attribute )
   {
      ossMutexLock( &connection->_sockMutex ) ;
      if ( NULL != connection->_attributeCache )
      {
         INT32 rc = bson_copy( attribute, connection->_attributeCache ) ;
         if ( BSON_OK == rc )
         {
            gotCache = TRUE ;
         }
      }
      ossMutexUnlock( &connection->_sockMutex ) ;
   }

   return gotCache ;
}

// internal function used by spidermonkey. _retval must be saved in this slot
SDB_EXPORT INT32 __sdbGetReserveSpace1 ( sdbConnectionHandle cHandle,
                                         UINT64 *space )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !space )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *space = connection->reserveSpace1 ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 __sdbSetReserveSpace1 ( sdbConnectionHandle cHandle,
                                         UINT64 space )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   connection->reserveSpace1 = space ;
done :
   return rc ;
error :
   goto done ;
}

#define ENCRYTED_STR_LEN   ( SDB_MD5_DIGEST_LENGTH * 2 + 1 )

static INT32 _sdbConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                           const CHAR *pUsrName, const CHAR *pPasswd,
                           BOOLEAN useSSL,
                           sdbConnectionHandle *handle )
{
   INT32 rc                            = SDB_OK ;
   BOOLEAN hasMutexInit                = FALSE ;
   //for the encryted password
   CHAR md5[ENCRYTED_STR_LEN + 1]      = {0} ;
   SINT64 contextID                    = 0 ;
   sdbConnectionStruct *connection     = NULL ;

   if ( !pHostName || !pServiceName || !pUsrName || !pPasswd || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // malloc connection handle and init it
   ALLOC_HANDLE( connection, sdbConnectionStruct ) ;
   connection->_handleType = SDB_HANDLE_TYPE_CONNECTION ;
   initHashTable( &connection->_tb ) ;
   // init mutex for the socket
   ossMutexInit ( &connection->_sockMutex ) ;
   hasMutexInit = TRUE ;
   connection->_msgConvertor = NULL ;

   // connect to the specify address
   rc = clientConnect ( pHostName, pServiceName, useSSL, &connection->_sock ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // request system information
   rc = requestSysInfo ( connection ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( SDB_PROTOCOL_VER_1 == connection->_peerProtocolVersion && !connection->_msgConvertor )
   {
      connection->_msgConvertor = (sdbMsgConvertor*)SDB_OSS_MALLOC ( sizeof(sdbMsgConvertor) ) ;
      if ( !connection->_msgConvertor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      connection->_msgConvertor->_buff = NULL ;
      connection->_msgConvertor->_buffSize = 0 ;
      connection->_msgConvertor->_hasData = FALSE ;
   }

   //encryt the password
   //do we need to take care of endianess?
   if ( FALSE == g_disablePassEncode )
   {
      rc = md5Encrypt( pPasswd, md5, ENCRYTED_STR_LEN ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else
   {
      ossStrncpy( md5, pPasswd, ENCRYTED_STR_LEN ) ;
   }

   //build checking message
   rc = clientBuildAuthVer0Msg( &connection->_pSendBuffer,
                                &connection->_sendBufferSize,
                                pUsrName, md5, 0,
                                connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( (sdbConnectionHandle)connection, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( (sdbConnectionHandle)connection,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        (sdbConnectionHandle)connection ) ;
   // set the return handle
   *handle = (sdbConnectionHandle)connection ;

done:
   return rc ;
error:
   if ( NULL !=connection )
   {
      sdbDisconnect( (sdbConnectionHandle)connection ) ;
      // destroy mutex
      if ( TRUE == hasMutexInit )
      {
         ossMutexDestroy ( &connection->_sockMutex ) ;
      }
      sdbReleaseConnection( (sdbConnectionHandle)connection ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                              const CHAR *pUsrName, const CHAR *pPasswd ,
                              sdbConnectionHandle *handle )
{
   return _sdbConnect ( pHostName, pServiceName, pUsrName, pPasswd, FALSE, handle ) ;
}

SDB_EXPORT INT32 sdbSecureConnect ( const CHAR *pHostName, const CHAR *pServiceName,
                                    const CHAR *pUsrName, const CHAR *pPasswd ,
                                    sdbConnectionHandle *handle )
{
   return _sdbConnect ( pHostName, pServiceName, pUsrName, pPasswd, TRUE, handle ) ;
}

SDB_EXPORT void sdbSetErrorOnReplyCallback( ERROR_ON_REPLY_FUNC func )
{
   _sdbErrorOnReplyCallback = func ;
}

SDB_EXPORT INT32 initClient( sdbClientConf* config )
{
   INT32 rc = SDB_OK ;
   if ( NULL == config )
   {
      goto done ;
   }

   rc = initCacheStrategy( config->enableCacheStrategy,
                           config->cacheTimeInterval ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = initNetworkTimeout( config->networkTimeout ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

//address[i] == "192.168.20.40:12345"
static INT32 _sdbConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                            const CHAR *pUsrName, const CHAR *pPasswd,
                            BOOLEAN useSSL,
                            sdbConnectionHandle *handle )
{
   INT32 rc                 = SDB_OK ;
   const CHAR *pHostName    = NULL ;
   const CHAR *pServiceName = NULL ;
   const CHAR *addr         = NULL ;
   CHAR *pStr               = NULL ;
   CHAR *pTmp               = NULL ;
   INT32 mark               = 0 ;
   INT32 i                  = 0 ;

   if ( !pConnAddrs || arrSize <= 0 || !pUsrName || !pPasswd || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // calculate the start position
   i = _sdbRand() % arrSize ;
   mark = i ;
   // get host and port
   do
   {
      addr = pConnAddrs[i] ;
      i++ ;
      i = i % arrSize ;
      pTmp = ossStrchr ( addr, ':' ) ;
      if ( !pTmp )
      {
         continue ;
      }

      pStr = ossStrdup ( addr ) ;
      if ( !pStr )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      pStr[pTmp - addr] = 0 ;
      pHostName = pStr ;
      pServiceName = &(pStr[pTmp - addr]) + 1;
      rc = _sdbConnect ( pHostName, pServiceName, pUsrName, pPasswd, useSSL, handle ) ;
      SDB_OSS_FREE ( pStr ) ;
      pStr = NULL ;
      pTmp = NULL ;
      if ( SDB_OK == rc )
         goto done ;
   } while ( mark != i ) ;
   // if we go here, means no valid addresses
   rc = SDB_NET_CANNOT_CONNECT ;
done:
   return rc ;
error:
   if ( handle )
   {
      *handle = SDB_INVALID_HANDLE ;
   }
   goto done;
}

SDB_EXPORT INT32 sdbConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                               const CHAR *pUsrName, const CHAR *pPasswd ,
                               sdbConnectionHandle *handle )
{
   return _sdbConnect1 ( pConnAddrs, arrSize, pUsrName, pPasswd, FALSE, handle) ;
}

SDB_EXPORT INT32 sdbSecureConnect1 ( const CHAR **pConnAddrs, INT32 arrSize,
                               const CHAR *pUsrName, const CHAR *pPasswd ,
                               sdbConnectionHandle *handle )
{
   return _sdbConnect1 ( pConnAddrs, arrSize, pUsrName, pPasswd, TRUE, handle) ;
}

SDB_EXPORT INT32 sdbGetPasswdByCipherFile( const CHAR *pUsrName,
                                           const CHAR *pToken,
                                           const CHAR *pCipherFile,
                                           CHAR *pUser, CHAR *pPasswd )
{
   INT32 rc = SDB_OK ;

   if ( NULL == pUsrName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = utilDecryptUserCipher( pUsrName, pToken,
                               pCipherFile, pUser, pPasswd ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

void _sdbDisconnect_inner ( sdbConnectionHandle handle )
{
   INT32 rc                        = SDB_OK ;
   Node *cursors                   = NULL ;
   Node *sockets                   = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)handle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( handle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // if we had disconnected
   if ( NULL == connection->_sock )
   {
      return ;
   }

   clientDisconnect ( &connection->_sock ) ;

   // notify all sockets to invalid
   sockets = connection->_sockets ;
   while ( sockets )
   {
      *((Socket**)sockets->data) = NULL ;
      connection->_sockets = sockets->next ;
      SDB_OSS_FREE( sockets ) ;
      sockets = connection->_sockets ;
   }

   // close all cursors
   cursors = connection->_cursors ;
   while ( cursors )
   {
      ((sdbCursorStruct*)cursors->data)->_isClosed = TRUE ;
      ((sdbCursorStruct*)cursors->data)->_contextID = -1 ;
      connection->_cursors = cursors->next ;
      SDB_OSS_FREE( cursors ) ;
      cursors = connection->_cursors ;
   }

   _sdbClearSessionAttrCache( connection, FALSE ) ;

done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbDisconnect ( sdbConnectionHandle cHandle )
{
   INT32 rc                        = SDB_OK ;
   BOOLEAN hasLock                 = FALSE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   ossMutexLock( &connection->_sockMutex );
   hasLock = TRUE ;

   // if we had disconnected
   if ( NULL == connection->_sock )
   {
      goto done ;
   }

   // build disconnect msg
   if ( SDB_OK == clientBuildDisconnectMsg ( &connection->_pSendBuffer,
                                             &connection->_sendBufferSize,
                                             0, connection->_endianConvert ) )
   {
      _send ( cHandle, connection->_sock, (MsgHeader*)connection->_pSendBuffer,
              connection->_endianConvert ) ;
   }

   // release cursor and socket resource and set socket to be NULL
   _sdbDisconnect_inner( cHandle ) ;

done:
   if ( TRUE == hasLock )
   {
      ossMutexUnlock( &connection->_sockMutex ) ;
   }
   return ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetDataBlocks ( sdbCollectionHandle cHandle,
                                    bson *condition,
                                    bson *select,
                                    bson *orderBy,
                                    bson *hint,
                                    INT64 numToSkip,
                                    INT64 numToReturn,
                                    sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *p                         = CMD_ADMIN_PREFIX CMD_NAME_GET_DATABLOCKS ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      p,
                      0, 0, numToSkip, numToReturn,
                      condition, select, orderBy, hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   /// set output result
   *handle = cursor ;

done:
   return rc ;
error:
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetQueryMeta ( sdbCollectionHandle cHandle,
                                   bson *condition,
                                   bson *orderBy,
                                   bson *hint,
                                   INT64 numToSkip,
                                   INT64 numToReturn,
                                   sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *p                         = CMD_ADMIN_PREFIX CMD_NAME_GET_QUERYMETA ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newHint ;
   bson_iterator itr ;

   /// check
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   /// build new hint
   BSON_INIT( newHint ) ;
   BSON_APPEND( newHint, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;

   rc = bson_append_start_object( &newHint, FIELD_NAME_HINT ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   if ( NULL != hint )
   {
      bson_iterator_init ( &itr, hint ) ;
      while ( BSON_EOO != bson_iterator_next ( &itr ) )
      {
         BSON_APPEND( newHint, NULL, &itr, element ) ;
      }
   }
   rc = bson_append_finish_object( &newHint ) ;
   if ( SDB_OK != rc )
   {
       rc = SDB_DRIVER_BSON_ERROR ;
       goto error ;
   }
   BSON_FINISH ( newHint ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      p,
                      0, 0, numToSkip, numToReturn,
                      condition, NULL, orderBy, &newHint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   /// set output result
   *handle = cursor ;

done:
   BSON_DESTROY( newHint ) ;
   return rc ;
error:
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

static INT32 _sdbGetSnapshot ( sdbConnectionHandle cHandle,
                               INT32 snapType,
                               bson *condition,
                               bson *selector,
                               bson *orderBy,
                               bson *hint,
                               INT64 numToSkip,
                               INT64 numToReturn,
                               sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   const CHAR *p                   = NULL ;
   sdbConnectionStruct *connection = NULL ;

   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   switch ( snapType )
   {
   case SDB_SNAP_CONTEXTS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTS ;
      break ;
   case SDB_SNAP_CONTEXTS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONTEXTS_CURRENT ;
      break ;
   case SDB_SNAP_SESSIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONS ;
      break ;
   case SDB_SNAP_SESSIONS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SESSIONS_CURRENT ;
      break ;
   case SDB_SNAP_COLLECTIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTIONS ;
      break ;
   case SDB_SNAP_COLLECTIONSPACES :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_COLLECTIONSPACES ;
      break ;
   case SDB_SNAP_DATABASE :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE ;
      break ;
   case SDB_SNAP_SYSTEM :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SYSTEM ;
      break ;
   case SDB_SNAP_CATALOG :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CATA ;
      break ;
   case SDB_SNAP_TRANSACTIONS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSACTIONS ;
      break ;
   case SDB_SNAP_TRANSACTIONS_CURRENT :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSACTIONS_CUR ;
      break ;
   case SDB_SNAP_ACCESSPLANS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_ACCESSPLANS ;
      break ;
   case SDB_SNAP_HEALTH :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_HEALTH ;
      break ;
   case SDB_SNAP_CONFIGS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONFIGS ;
      break ;
   case SDB_SNAP_SVCTASKS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SVCTASKS ;
      break ;
   case SDB_SNAP_SEQUENCES :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_SEQUENCES ;
      break ;
   case SDB_SNAP_QUERIES :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_QUERIES ;
      break ;
   case SDB_SNAP_LATCHWAITS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_LATCHWAITS ;
      break ;
   case SDB_SNAP_LOCKWAITS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_LOCKWAITS ;
      break ;
   case SDB_SNAP_INDEXSTATS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_INDEXSTATS ;
      break ;
   case SDB_SNAP_TRANSWAITS :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSWAITS ;
      break ;
   case SDB_SNAP_TRANSDEADLOCK :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TRANSDEADLOCK ;
      break ;
   case SDB_SNAP_RECYCLEBIN :
      p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RECYCLEBIN ;
      break ;
   default :
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   connection = (sdbConnectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      p,
                      0, 0, numToSkip, numToReturn,
                      condition, selector, orderBy, hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   /// set output result
   *handle = cursor ;

done:
   return rc ;
error:
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetSnapshot ( sdbConnectionHandle cHandle,
                                  INT32 snapType,
                                  bson *condition,
                                  bson *selector,
                                  bson *orderBy,
                                  sdbCursorHandle *handle )
{
   INT32 rc = SDB_OK ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = _sdbGetSnapshot ( cHandle, snapType,
                          condition, selector, orderBy, NULL,
                          0, -1, handle ) ;
   if ( rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetSnapshot1 ( sdbConnectionHandle cHandle,
                                   INT32 snapType,
                                   bson *condition,
                                   bson *selector,
                                   bson *orderBy,
                                   bson *hint,
                                   INT64 numToSkip,
                                   INT64 numToReturn,
                                   sdbCursorHandle *handle )
{
   INT32 rc = SDB_OK ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = _sdbGetSnapshot ( cHandle, snapType,
                          condition, selector, orderBy, hint,
                          numToSkip, numToReturn, handle ) ;
   if ( rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbCreateUsr( sdbConnectionHandle cHandle,
                               const CHAR *pUsrName,
                               const CHAR *pPasswd )
{
   INT32 rc                        = SDB_OK ;
   CHAR md5[ENCRYTED_STR_LEN]      = {0};
   SINT64 contextID                = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION );
   if ( !pUsrName || !pPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( MAX_USERNAME_LENGTH < ossStrlen( pUsrName ) || MAX_PASSWORD_LENGTH < ossStrlen( pPasswd ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = md5Encrypt( pPasswd, md5, ENCRYTED_STR_LEN) ;
   if ( rc )
   {
      goto error ;
   }
   rc = clientBuildAuthCrtMsg( &connection->_pSendBuffer,
                               &connection->_sendBufferSize,
                               pUsrName, pPasswd, md5, NULL,
                               0, connection->_endianConvert,
                               connection->_authVersion ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done:
  return rc ;
error:
  goto done ;
}

SDB_EXPORT INT32 sdbRemoveUsr( sdbConnectionHandle cHandle,
                               const CHAR *pUsrName,
                               const CHAR *pPasswd )
{
   INT32 rc                        = SDB_OK ;
   CHAR md5[ENCRYTED_STR_LEN]      = {0};
   SINT64 contextID                = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pUsrName || !pPasswd )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = md5Encrypt( pPasswd, md5, ENCRYTED_STR_LEN ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = clientBuildAuthDelMsg( &connection->_pSendBuffer,
                               &connection->_sendBufferSize,
                               pUsrName, md5, 0, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done:
  return rc ;
error:
  goto done ;
}

SDB_EXPORT INT32 sdbResetSnapshot ( sdbConnectionHandle cHandle,
                                    bson *options )
{
   INT32 rc                        = SDB_OK ;
   CHAR *p                         = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RESET ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      p, options,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetList ( sdbConnectionHandle cHandle,
                              INT32 listType,
                              bson *condition,
                              bson *selector,
                              bson *orderBy,
                              sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;

   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = _sdbGetList ( cHandle,
                      listType,
                      condition, selector, orderBy, NULL,
                      0, -1,
                      handle ) ;
   if ( SDB_OK != rc )
   {
      goto done ;
   }
done :
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetList1( sdbConnectionHandle cHandle,
                              INT32 listType,
                              bson *condition,
                              bson *selector,
                              bson *orderBy,
                              bson *hint,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;

   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = _sdbGetList ( cHandle,
                      listType,
                      condition, selector, orderBy, hint,
                      numToSkip, numToReturn,
                      handle ) ;
   if ( SDB_OK != rc )
   {
      goto done ;
   }
done :
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetCollection ( sdbConnectionHandle cHandle,
                                    const CHAR *pCollectionFullName,
                                    sdbCollectionHandle *handle )
{
   INT32 rc               = SDB_OK ;
   CHAR *pTestCollection  = CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION ;
   CHAR *pName            = FIELD_NAME_NAME ;
   sdbCollectionStruct *s = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit       = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pCollectionFullName || !handle ||
        ossStrlen ( pCollectionFullName) > CL_FULLNAME_LEN )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( fetchCachedObject( connection->_tb, pCollectionFullName ) )
   {
      // DO NOTHING
   }
   else
   {
      BSON_APPEND( newObj, pName, pCollectionFullName, string ) ;
      BSON_FINISH ( newObj ) ;

      rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                         &connection->_sendBufferSize,
                         &connection->_pReceiveBuffer,
                         &connection->_receiveBufferSize,
                         connection->_endianConvert,
                         pTestCollection, &newObj,
                         NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( connection->_tb, pCollectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ALLOC_HANDLE( s, sdbCollectionStruct ) ;
   s->_handleType    = SDB_HANDLE_TYPE_COLLECTION ;
   s->_connection    = cHandle ;
   s->_sock          = connection->_sock ;
   s->_endianConvert = connection->_endianConvert ;
   rc = _setCollectionName ( (sdbCollectionHandle)s, pCollectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   _regSocket( cHandle, &s->_sock ) ;

   *handle = (sdbCollectionHandle)s ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( s )
   {
      SDB_OSS_FREE ( s ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetCollectionSpace ( sdbConnectionHandle cHandle,
                                         const CHAR *pCollectionSpaceName,
                                         sdbCSHandle *handle )
{
   INT32 rc              = SDB_OK ;
   CHAR *pTestCollection = CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTIONSPACE ;
   CHAR *pName           = FIELD_NAME_NAME ;
   sdbCSStruct *s        = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit      = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pCollectionSpaceName || !handle ||
        ossStrlen ( pCollectionSpaceName) > CLIENT_CS_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( fetchCachedObject( connection->_tb, pCollectionSpaceName ) )
   {
      // DO NOTHING
   }
   else
   {
      BSON_APPEND( newObj, pName, pCollectionSpaceName, string ) ;
      BSON_FINISH ( newObj ) ;

      rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                         &connection->_sendBufferSize,
                         &connection->_pReceiveBuffer,
                         &connection->_receiveBufferSize,
                         connection->_endianConvert,
                         pTestCollection, &newObj,
                         NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( connection->_tb, pCollectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ALLOC_HANDLE( s, sdbCSStruct ) ;
   s->_handleType    = SDB_HANDLE_TYPE_CS ;
   s->_connection    = cHandle ;
   s->_sock          = connection->_sock ;
   s->_endianConvert = connection->_endianConvert ;
   rc = _setCSName ( (sdbCSHandle)s, pCollectionSpaceName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   _regSocket( cHandle, &s->_sock ) ;

   *handle = (sdbCSHandle)s ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( s )
   {
      SDB_OSS_FREE ( s ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetReplicaGroup ( sdbConnectionHandle cHandle,
                                      const CHAR *pGroupName,
                                      sdbReplicaGroupHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *pName                     = CAT_GROUPNAME_NAME ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pGroupName || !handle ||
        ossStrlen ( pGroupName ) > CLIENT_RG_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pGroupName, string ) ;
   //BSAON_APPEND_STRING( newObj, pName, pGroupName ) ;
   BSON_FINISH ( newObj ) ;

   rc = _sdbGetReplicaGroup( cHandle, newObj, handle ) ;
   if (SDB_OK != rc )
   {
      goto error;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetReplicaGroup1 ( sdbConnectionHandle cHandle,
                                       UINT32 id,
                                       sdbReplicaGroupHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *pName                     = CAT_GROUPID_NAME ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, (INT32)id, int ) ;
   BSON_FINISH ( newObj ) ;

   rc = _sdbGetReplicaGroup( cHandle, newObj, handle ) ;
   if (SDB_OK != rc )
   {
      goto error;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetReplicaGroupName ( sdbReplicaGroupHandle cHandle,
                                          CHAR **ppRGName )
{
   INT32 rc       = SDB_OK ;
   sdbRGStruct *r = (sdbRGStruct*)cHandle ;

   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( ppRGName )
   {
      *ppRGName = r->_replicaGroupName ;
   }
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetRGName ( sdbReplicaGroupHandle cHandle,
                                CHAR *pBuffer, INT32 size )
{
   INT32 rc       = SDB_OK ;
   INT32 name_len = 0 ;
   sdbRGStruct *r = (sdbRGStruct*)cHandle ;

   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( NULL == pBuffer )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( size <= 0 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   name_len = ossStrlen( r->_replicaGroupName ) ;
   if ( size < name_len + 1 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }
   ossStrncpy( pBuffer, r->_replicaGroupName, name_len ) ;
   pBuffer[name_len] = 0 ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT BOOLEAN sdbIsReplicaGroupCatalog ( sdbReplicaGroupHandle cHandle )
{
   INT32 rc          = SDB_OK ;
   BOOLEAN isCatalog = FALSE ;
   sdbRGStruct *r    = (sdbRGStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   isCatalog = r->_isCatalog ;
done :
   return isCatalog ;
error :
   goto done;
}

SDB_EXPORT INT32 sdbCreateReplicaCataGroup ( sdbConnectionHandle cHandle,
                                             const CHAR *pHostName,
                                             const CHAR *pServiceName,
                                             const CHAR *pDatabasePath,
                                             bson *configure )
{
   INT32 rc         = SDB_OK ;
   CHAR *pCataRG    = CMD_ADMIN_PREFIX CMD_NAME_CREATE_CATA_GROUP ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson configuration ;

   BSON_INIT( configuration ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pHostName || !pServiceName || !pDatabasePath )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // HostName is required
   BSON_APPEND( configuration, CAT_HOST_FIELD_NAME, pHostName, string ) ;
   // ServiceName is required
   BSON_APPEND( configuration, PMD_OPTION_SVCNAME, pServiceName, string ) ;
   // database path is required
   BSON_APPEND( configuration, PMD_OPTION_DBPATH, pDatabasePath, string ) ;

   // append all other parameters
   if ( configure )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, configure ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key ( &it ) ;
         if ( !ossStrcmp ( key, PMD_OPTION_DBPATH )  ||
              !ossStrcmp ( key, PMD_OPTION_SVCNAME ) ||
              !ossStrcmp ( key, CAT_HOST_FIELD_NAME ) )
         {
            // skip the ones we already created
            continue ;
         }

         bson_append_element( &configuration, NULL, &it ) ;
      } // while
   } // if ( configure )
   BSON_FINISH ( configuration ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCataRG, &configuration,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( configuration ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCreateNode ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pHostName,
                                 const CHAR *pServiceName,
                                 const CHAR *pDatabasePath,
                                 bson *configure )
{
   INT32 rc = SDB_OK ;
   CHAR *pCreateNode = CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE ;
   sdbRGStruct *r    = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit  = FALSE ;
   bson configuration ;

   BSON_INIT( configuration ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !pHostName || !pServiceName || !pDatabasePath )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // GroupName is required
   BSON_APPEND( configuration,
                CAT_GROUPNAME_NAME, r->_replicaGroupName, string ) ;
   // HostName is required
   BSON_APPEND( configuration, CAT_HOST_FIELD_NAME, pHostName, string ) ;
   // ServiceName is required
   BSON_APPEND( configuration, PMD_OPTION_SVCNAME, pServiceName, string ) ;

   // database path is required
   BSON_APPEND( configuration, PMD_OPTION_DBPATH, pDatabasePath, string ) ;
   // append all other parameters
   if ( configure )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, configure ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key ( &it ) ;
         if ( !ossStrcmp ( key, PMD_OPTION_DBPATH )   ||
              !ossStrcmp ( key, PMD_OPTION_SVCNAME )  ||
              !ossStrcmp ( key, CAT_HOST_FIELD_NAME ) ||
              !ossStrcmp ( key, CAT_GROUPNAME_NAME ) )
         {
            // skip the ones we already created
            continue ;
         }

         rc = bson_append_element( &configuration, NULL, &it ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      } // while
   } // if ( configure )
   BSON_FINISH ( configuration ) ;

   rc = _runCommand ( r->_connection, r->_sock, &r->_pSendBuffer,
                      &r->_sendBufferSize,
                      &r->_pReceiveBuffer,
                      &r->_receiveBufferSize,
                      r->_endianConvert,
                      pCreateNode, &configuration,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( configuration ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbRemoveNode ( sdbReplicaGroupHandle cHandle,
                                 const CHAR *pHostName,
                                 const CHAR *pServiceName,
                                 bson *configure )
{
   INT32 rc          = SDB_OK ;
   CHAR *pRemoveNode = CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE ;
   sdbRGStruct *r    = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit  = FALSE ;
   bson removeInfo ;

   BSON_INIT( removeInfo ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !pHostName || !pServiceName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // GroupName is required
   BSON_APPEND( removeInfo, CAT_GROUPNAME_NAME,
                r->_replicaGroupName, string ) ;

   // HostName is required
   BSON_APPEND( removeInfo, FIELD_NAME_HOST, pHostName, string ) ;

   // ServiceName is required
   BSON_APPEND( removeInfo, PMD_OPTION_SVCNAME, pServiceName, string ) ;
   if ( configure )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, configure ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key ( &it ) ;
         if ( ossStrcmp ( key, FIELD_NAME_HOST ) == 0  ||
              ossStrcmp ( key, FIELD_NAME_SERVICE_NAME ) == 0 ||
              ossStrcmp ( key, PMD_OPTION_SVCNAME ) == 0 ||
              ossStrcmp ( key, CAT_GROUPNAME_NAME ) == 0 )
         {
            // skip the ones we already created
            continue ;
         }
         else
         {
            BSON_APPEND( removeInfo, NULL, &it, element ) ;
         }
      }
   }

   BSON_FINISH( removeInfo ) ;

   rc = _runCommand ( r->_connection, r->_sock, &r->_pSendBuffer,
                      &r->_sendBufferSize,
                      &r->_pReceiveBuffer,
                      &r->_receiveBufferSize,
                      r->_endianConvert,
                      pRemoveNode, &removeInfo,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( removeInfo ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbCreateCollectionSpaceV2 ( sdbConnectionHandle cHandle,
                                              const CHAR *pCollectionSpaceName,
                                              bson *options,
                                              sdbCSHandle *handle )
{
   INT32 rc                = SDB_OK ;
   CHAR *pCreateCollection = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ;
   sdbCSStruct *s          = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit        = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pCollectionSpaceName || !handle ||
        ossStrlen ( pCollectionSpaceName) > CLIENT_CS_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME, pCollectionSpaceName, string ) ;

   if ( options )
   {
      bson_iterator itr ;
      bson_iterator_init( &itr, options ) ;
      while ( bson_iterator_more( &itr ) )
      {
         bson_iterator_next( &itr ) ;
         BSON_APPEND( newObj, NULL, &itr, element ) ;
      }
   }

   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCreateCollection, &newObj,
                      NULL, NULL, NULL ) ;

   if ( SDB_OK != rc )
   {
      goto error ;
   }

   ALLOC_HANDLE( s, sdbCSStruct ) ;
   s->_handleType    = SDB_HANDLE_TYPE_CS ;
   s->_connection    = cHandle ;
   s->_sock          = connection->_sock ;
   s->_endianConvert = connection->_endianConvert ;
   rc = _setCSName ( (sdbCSHandle)s, pCollectionSpaceName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = insertCachedObject( connection->_tb, pCollectionSpaceName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   _regSocket( cHandle, &s->_sock ) ;
   *handle = (sdbCSHandle)s ;

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   if ( s )
   {
      SDB_OSS_FREE( s ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbCreateCollectionSpace ( sdbConnectionHandle cHandle,
                                            const CHAR *pCollectionSpaceName,
                                            INT32 iPageSize,
                                            sdbCSHandle *handle )
{
   INT32 rc               = SDB_OK ;
   BOOLEAN bsoninit       = FALSE ;
   bson options ;

   BSON_INIT( options );
   BSON_APPEND( options, FIELD_NAME_PAGE_SIZE, iPageSize, int ) ;
   BSON_FINISH( options ) ;
   rc = sdbCreateCollectionSpaceV2( cHandle, pCollectionSpaceName,
                                    &options, handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( options ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbDropCollectionSpace ( sdbConnectionHandle cHandle,
                                          const CHAR *pCollectionSpaceName )
{
   INT32 rc                        = SDB_OK ;

   rc = sdbDropCollectionSpace1( cHandle, pCollectionSpaceName, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbDropCollectionSpace1 ( sdbConnectionHandle cHandle,
                                           const CHAR *pCollectionSpaceName,
                                           bson *options)
{
   INT32 rc                        = SDB_OK ;
   CHAR *pDropCollection           = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ;
   CHAR *pName                     = FIELD_NAME_NAME ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pCollectionSpaceName ||
        ossStrlen ( pCollectionSpaceName) > CLIENT_CS_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   BSON_APPEND( newObj, pName, pCollectionSpaceName, string ) ;
   if ( NULL != options )
   {
      rc = _mergeBson( &newObj, options ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pDropCollection, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = removeCachedObject( connection->_tb, pCollectionSpaceName, TRUE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCreateReplicaGroup ( sdbConnectionHandle cHandle,
                                         const CHAR *pGroupName,
                                         sdbReplicaGroupHandle *handle )
{
   INT32 rc           = SDB_OK ;
   CHAR *pCreateRG    = CMD_ADMIN_PREFIX CMD_NAME_CREATE_GROUP ;
   CHAR *pName        = FIELD_NAME_GROUPNAME ;
   sdbRGStruct *r     = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit   = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pGroupName || !handle ||
        ossStrlen ( pGroupName ) > CLIENT_RG_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pGroupName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCreateRG, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   ALLOC_HANDLE( r, sdbRGStruct ) ;
   r->_handleType    = SDB_HANDLE_TYPE_REPLICAGROUP ;
   r->_connection    = cHandle ;
   r->_sock          = connection->_sock ;
   r->_endianConvert = connection->_endianConvert ;
   rc = _setRGName ( (sdbReplicaGroupHandle)r, pGroupName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   *handle = (sdbReplicaGroupHandle)r ;
   _regSocket( cHandle, &r->_sock ) ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( r )
   {
      SDB_OSS_FREE ( r ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbRemoveReplicaGroup ( sdbConnectionHandle cHandle,
                                         const CHAR *pGroupName )
{
   INT32 rc         = SDB_OK ;
   INT32 nameLength = 0 ;
   CHAR *pCommand   = CMD_ADMIN_PREFIX CMD_NAME_REMOVE_GROUP ;
   CHAR *pName      = FIELD_NAME_GROUPNAME ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pGroupName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   nameLength = ossStrlen( pGroupName ) ;
   if ( 0 == nameLength || CLIENT_RG_NAMESZ < nameLength )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pGroupName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCommand, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbStartReplicaGroup ( sdbReplicaGroupHandle cHandle )
{
   INT32 rc             = SDB_OK ;
   CHAR *pActivateRG    = CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP ;
   CHAR *pName          = FIELD_NAME_GROUPNAME ;
   sdbRGStruct *r       = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit     = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   BSON_APPEND( newObj, pName, r->_replicaGroupName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( r->_connection, r->_sock, &r->_pSendBuffer,
                      &r->_sendBufferSize,
                      &r->_pReceiveBuffer,
                      &r->_receiveBufferSize,
                      r->_endianConvert,
                      pActivateRG, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbStopReplicaGroup ( sdbReplicaGroupHandle cHandle )
{
   INT32 rc         = SDB_OK ;
   sdbRGStruct *r   = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson configuration ;

   BSON_INIT( configuration );
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   BSON_APPEND( configuration,
                FIELD_NAME_GROUPNAME, r->_replicaGroupName, string ) ;
   BSON_FINISH ( configuration ) ;

   rc = _runCommand ( r->_connection, r->_sock, &r->_pSendBuffer,
                      &r->_sendBufferSize,
                      &r->_pReceiveBuffer,
                      &r->_receiveBufferSize,
                      r->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_GROUP,
                      &configuration,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( configuration ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetNodeMaster ( sdbReplicaGroupHandle cHandle,
                                    sdbNodeHandle *handle )
{
   INT32 rc                = SDB_OK ;
   const CHAR *primaryData = NULL ;
   INT32 primaryNode       = -1 ;
   sdbRGStruct *r          = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit        = FALSE ;
   bson_type bType         = BSON_EOO ;
   bson_iterator it ;
   bson result ;

   BSON_INIT( result );
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
     goto error ;
   }

   rc = _sdbGetReplicaGroupDetail ( cHandle, &result ) ;
   if ( SDB_OK != rc )
   {
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
      }
      goto error ;
   }
   // extract the primary node and find out the node id
   if ( BSON_ARRAY != bson_find ( &it, &result, CAT_GROUP_NAME ) )
   {
      // the Group is not array
      rc = SDB_SYS ;
      goto error ;
   }
   {
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      bson_iterator_from_buffer ( &i, groupList ) ;
      // loop for all elements in Group
      if ( BSON_EOO == bson_iterator_next ( &i ) )
      {
         rc = SDB_CLS_EMPTY_GROUP ;
         goto error ;
      }
   }
   // check has primary or not
   bType = bson_find ( &it, &result, CAT_PRIMARY_NAME ) ;
   if ( BSON_EOO == bType )
   {
      // cannot find primary
      rc = SDB_RTN_NO_PRIMARY_FOUND ;
      goto error ;
   }
   if ( BSON_INT != bType )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   primaryNode = bson_iterator_int ( &it ) ;
   if ( -1 == primaryNode )
   {
      // cannot find primary
      rc = SDB_RTN_NO_PRIMARY_FOUND ;
      goto error ;
   }
   // extract the primary node and find out the node id
   bson_find ( &it, &result, CAT_GROUP_NAME ) ;
   // walk through Group and find out the NodeID
   {
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      bson_iterator_from_buffer ( &i, groupList ) ;
      // loop for all elements in Group
      while ( bson_iterator_next ( &i ) )
      {
         bson intObj ;
         bson_init( &intObj ) ;
         // make sure each element is object and construct intObj object
         // bson_init_finished_data does not accept const CHAR*,
         // however since we are NOT going to perform any change, it's safe to
         // cast const CHAR* to CHAR* here
         if ( BSON_OBJECT == (signed int)bson_iterator_type ( &i ) &&
              BSON_OK == bson_init_finished_data ( &intObj,
                                       (CHAR*)bson_iterator_value ( &i ) ) )
         {
            bson_iterator k ;
            // look for "NodeID" in each object
            if ( BSON_INT != bson_find ( &k, &intObj, CAT_NODEID_NAME ) )
            {
               rc = SDB_SYS ;
               bson_destroy ( &intObj ) ;
               goto error ;
            }
            // if we find the master, let's record the pointer and jump out
            if ( primaryNode == bson_iterator_int ( &k ) )
            {
               primaryData = intObj.data ;
               break ;
            }
         }
         bson_destroy ( &intObj ) ;
      }
   }
   if ( primaryData )
   {
      rc = _sdbRGExtractNode ( cHandle, handle, primaryData,
                               r->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else
   {
      // it is impossible for us to find primary id but cannot
      // find primary node in list
      rc = SDB_SYS ;
      goto error ;
   }
done :
   BSON_DESTROY( result ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

static INT32 _sdbGetNodeSlave ( sdbReplicaGroupHandle cHandle,
                                const INT32 *positionsArray,
                                INT32 positionsCount,
                                BOOLEAN forcedInput,
                                sdbNodeHandle *handle )
{
   INT32 rc                      = SDB_OK ;
   sdbRGStruct *r                = (sdbRGStruct*)cHandle ;
   BOOLEAN needGeneratePosition  = FALSE ;
   BOOLEAN hasPrimary            = TRUE ;
   const CHAR *nodeDatas[7]      = { NULL } ;
   INT32 nodeCount               = 0 ;
   INT32 validPositions[7]       = { 0 } ;
   INT32 validPositionsCount     = 0 ;
   INT32 primaryNodePosition     = 0 ;
   INT32 primaryNodeId           = -1 ;
   bson_type bType               = BSON_EOO ;
   bson_iterator it ;
   bson result ;
   INT32 i = 0, j = 0 ;

   bson_init( &result ) ;
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // check arguments
   if ( FALSE == forcedInput && NULL == positionsArray && 0 == positionsCount )
   {
      needGeneratePosition = TRUE ;
   }
   else if ( NULL == positionsArray || positionsCount <= 0 || positionsCount > 7 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !needGeneratePosition )
   {
      for ( i = 0 ; i < positionsCount ; i++ )
      {
         BOOLEAN hasContained = FALSE ;
         INT32 pos = positionsArray[i] ;
         if ( pos < 1 || pos > 7 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         for ( j = 0 ; j < validPositionsCount ; j++ )
         {
            if ( pos == validPositions[j] )
            {
               hasContained = TRUE ;
               break ;
            }
         }
         if ( !hasContained )
         {
            validPositions[validPositionsCount++] = pos ;
         }
      }
   }
   // get detail of group from catalog
   rc = _sdbGetReplicaGroupDetail ( cHandle, &result ) ;
   if ( SDB_OK != rc )
   {
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
      }
      goto error ;
   }
   // check group's info
   if ( BSON_ARRAY != bson_find ( &it, &result, CAT_GROUP_NAME ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   {
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      bson_iterator_from_buffer ( &i, groupList ) ;
      // loop for all elements in Group
      if ( BSON_EOO == bson_iterator_next ( &i ) )
      {
         rc = SDB_CLS_EMPTY_GROUP ;
         goto error ;
      }
   }
   bType = bson_find ( &it, &result, CAT_PRIMARY_NAME ) ;
   if ( BSON_EOO == bType )
   {
      hasPrimary = FALSE ;
   }
   else if ( BSON_INT != bType )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   else
   {
      // get the primary node and skip it later
      primaryNodeId = bson_iterator_int ( &it ) ;
      if ( -1 == primaryNodeId )
      {
         hasPrimary = FALSE ;
      }
   }
   // walk through Group and skip primary node, and pickup a random one
   if ( BSON_ARRAY != bson_find ( &it, &result, CAT_GROUP_NAME ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   {
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      bson intObj ;

      // loop for all elements in Group
      bson_iterator_from_buffer ( &i, groupList ) ;
      while ( bson_iterator_next ( &i ) )
      {
         bson_init( &intObj ) ;
         // make sure each element is object and construct intObj object
         // bson_init_finished_data does not accept const CHAR*,
         // however since we are NOT going to perform any change, it's safe
         // to cast const CHAR* to CHAR* here
         if ( BSON_OBJECT == (signed int)bson_iterator_type ( &i ) &&
              BSON_OK == bson_init_finished_data ( &intObj,
                            (CHAR*)bson_iterator_value ( &i ) ) )
         {

            bson_iterator k ;
            // look for "NodeID" in each object
            if ( BSON_INT != bson_find ( &k, &intObj, CAT_NODEID_NAME ) )
            {
               bson_destroy( &intObj ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            nodeDatas[nodeCount] = intObj.data ;
            nodeCount++ ;
            // if we find the master, let's mark it's position
            if ( hasPrimary && primaryNodeId == bson_iterator_int ( &k ) )
            {
               primaryNodePosition = nodeCount ;
            }
         }
         else
         {
            bson_destroy ( &intObj ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         bson_destroy ( &intObj ) ;
      } // while ( bson_iterator_next ( &i ) )
   }
   // check
   if ( hasPrimary && 0 == primaryNodePosition )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   // try to generate slave node's positions
   if ( needGeneratePosition )
   {
      for ( i = 0 ; i < nodeCount; i++ )
      {
         if ( hasPrimary && primaryNodePosition == i + 1 )
         {
            continue ;
         }
         validPositions[validPositionsCount++] = i + 1 ;
      }
   }
   // build sdbNode
   if ( 1 == nodeCount )
   {
      rc = _sdbRGExtractNode ( cHandle, handle, nodeDatas[0],
                               r->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else if ( 1 == validPositionsCount )
   {
      INT32 idx = ( validPositions[0] - 1 ) % nodeCount ;
      rc = _sdbRGExtractNode ( cHandle, handle, nodeDatas[idx],
                               r->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   else
   {
      INT32 position = 0 ;
      INT32 nodeIndex = -1 ;
      INT32 rand = _sdbRand() ;
      INT32 flags[7] = { 0 } ;
      INT32 includePrimaryPositions[7]   = { 0 } ;
      INT32 includePrimaryPositionsCount = 0 ;
      INT32 excludePrimaryPositions[7]   = { 0 } ;
      INT32 excludePrimaryPositionsCount = 0 ;

      for ( i = 0 ; i < validPositionsCount ; i++ )
      {
         INT32 pos = validPositions[i] ;
         if ( pos <= nodeCount )
         {
            nodeIndex = pos - 1 ;
            if ( flags[nodeIndex] == 0 )
            {
               flags[nodeIndex] = 1 ;
               includePrimaryPositions[includePrimaryPositionsCount++] = pos ;
               if ( hasPrimary && primaryNodePosition != pos )
               {
                  excludePrimaryPositions[excludePrimaryPositionsCount++] = pos ;
               }
            }
         }
         else
         {
            nodeIndex = ( pos - 1 ) % nodeCount ;
            if ( flags[nodeIndex] == 0 )
            {
               flags[nodeIndex] = 1 ;
               includePrimaryPositions[includePrimaryPositionsCount++] = pos ;
               if ( hasPrimary &&
                    primaryNodePosition != nodeIndex + 1 )
               {
                  excludePrimaryPositions[excludePrimaryPositionsCount++] = pos ;
               }
            }
         }
      }
      if ( excludePrimaryPositionsCount > 0 )
      {
         position = rand % excludePrimaryPositionsCount ;
         position = excludePrimaryPositions[position] ;
      }
      else
      {
         position = rand % includePrimaryPositionsCount ;
         position = includePrimaryPositions[position] ;
         if ( needGeneratePosition )
         {
            position += 1 ;
         }
      }
      nodeIndex = ( position - 1 ) % nodeCount ;
      rc = _sdbRGExtractNode( cHandle, handle, nodeDatas[nodeIndex],
                              r->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

done :
   bson_destroy( &result ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetNodeSlave1 ( sdbReplicaGroupHandle cHandle,
                                    const INT32 *positionsArray,
                                    INT32 positionsCount,
                                    sdbNodeHandle *handle )
{
   return _sdbGetNodeSlave( cHandle, positionsArray,
                            positionsCount, TRUE, handle ) ;
}

SDB_EXPORT INT32 sdbGetNodeSlave ( sdbReplicaGroupHandle cHandle,
                                   sdbNodeHandle *handle )
{
   return _sdbGetNodeSlave( cHandle, NULL, 0, FALSE, handle ) ;
}

SDB_EXPORT INT32 sdbGetNodeByName ( sdbReplicaGroupHandle cHandle,
                                    const CHAR *pNodeName,
                                    sdbNodeHandle *handle )
{
   INT32 rc           = SDB_OK ;
   CHAR *pHostName    = NULL ;
   CHAR *pServiceName = NULL ;
   INT32 nodeNameLen  = 0 ;
   sdbRGStruct *r     = (sdbRGStruct*)cHandle ;

   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !pNodeName || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   nodeNameLen = ossStrlen ( pNodeName ) + 1 ;
   pHostName = (CHAR*)SDB_OSS_MALLOC ( nodeNameLen ) ;
   if ( !pHostName )
   {
      rc = SDB_OOM ;
      goto error ;
   }

   ossStrncpy(pHostName, pNodeName, nodeNameLen ) ;
   pServiceName = ossStrchr ( pHostName, NODE_NAME_SERVICE_SEPCHAR ) ;
   if ( !pServiceName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *pServiceName = '\0' ;
   pServiceName ++ ;
   rc = sdbGetNodeByHost ( cHandle, pHostName, pServiceName, handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   if ( pHostName )
   {
      SDB_OSS_FREE ( pHostName ) ;
   }
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetNodeByHost ( sdbReplicaGroupHandle cHandle,
                                    const CHAR *pHostName,
                                    const CHAR *pServiceName,
                                    sdbNodeHandle *handle )
{
   INT32 rc                = SDB_OK ;
   const CHAR *hostName    = NULL ;
   const CHAR *serviceName = NULL ;
   const CHAR *nodeName    = NULL ;
   sdbRGStruct *r          = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit        = FALSE ;
   bson_iterator it ;
   bson result ;

   BSON_INIT( result );
   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( !pHostName || !pServiceName || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *handle = SDB_INVALID_HANDLE ;

   rc = _sdbGetReplicaGroupDetail ( cHandle, &result ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // walk through Group and find out the NodeID
   if ( BSON_ARRAY != bson_find ( &it, &result, CAT_GROUP_NAME ) )
   {
      // the Group is not array
      rc = SDB_SYS ;
      goto error ;
   }

   {
      INT32 nodeID = 0 ;
      const CHAR *groupList = bson_iterator_value ( &it ) ;
      bson_iterator i ;
      sdbNodeHandle interhandle = SDB_INVALID_HANDLE ;
      bson_iterator_from_buffer ( &i, groupList ) ;

      // loop for all elements in Group
      while ( BSON_EOO != bson_iterator_next ( &i ) )
      {
         rc = _sdbRGExtractNode ( cHandle, &interhandle,
                                  (CHAR*)bson_iterator_value ( &i ),
                                  r->_endianConvert ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         sdbGetNodeAddr ( interhandle, &hostName,
                          &serviceName, &nodeName,
                          &nodeID ) ;

         if ( !ossStrcmp ( hostName, pHostName ) &&
              !ossStrcmp ( serviceName, pServiceName ))
         {
            break ;
         }
         sdbReleaseNode ( interhandle ) ;
         interhandle = SDB_INVALID_HANDLE ;
      }

      *handle = interhandle ;
   }

   if ( SDB_INVALID_HANDLE == *handle )
   {
      // if we can't find the given id
      rc = SDB_CLS_NODE_NOT_EXIST ;
      goto error ;
   }
done :
   BSON_DESTROY( result ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetNodeAddr ( sdbNodeHandle cHandle,
                                  const CHAR **ppHostName,
                                  const CHAR **ppServiceName,
                                  const CHAR **ppNodeName,
                                  INT32 *pNodeID )
{
   INT32 rc       = SDB_OK ;
   sdbRNStruct *r = (sdbRNStruct*)cHandle ;

   HANDLE_CHECK( cHandle, r, SDB_HANDLE_TYPE_REPLICANODE ) ;
   if ( ppHostName )
   {
      *ppHostName = r->_hostName ;
   }
   if ( ppServiceName )
   {
      *ppServiceName = r->_serviceName ;
   }
   if ( ppNodeName )
   {
      *ppNodeName = r->_nodeName ;
   }
   if ( pNodeID )
   {
      *pNodeID = r->_nodeID ;
   }
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbStartNode ( sdbNodeHandle cHandle )
{
   return _sdbStartStopNode ( cHandle, TRUE ) ;
}

SDB_EXPORT INT32 sdbStopNode ( sdbNodeHandle cHandle )
{
   return _sdbStartStopNode ( cHandle, FALSE ) ;
}

SDB_EXPORT INT32 sdbListCollectionSpaces ( sdbConnectionHandle cHandle,
                                           sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_COLLECTIONSPACES, NULL, NULL, NULL,
                       handle ) ;
}

SDB_EXPORT INT32 sdbListCollections ( sdbConnectionHandle cHandle,
                                      sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_COLLECTIONS, NULL, NULL, NULL,
                       handle ) ;
}

SDB_EXPORT INT32 sdbListSequences ( sdbConnectionHandle cHandle,
                                      sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_SEQUENCES, NULL, NULL, NULL,
                       handle ) ;
}

SDB_EXPORT INT32 sdbListReplicaGroups ( sdbConnectionHandle cHandle,
                                        sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_GROUPS, NULL, NULL, NULL,
                       handle ) ;
}

SDB_EXPORT INT32 sdbFlushConfigure( sdbConnectionHandle cHandle,
                                    bson *options )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_EXPORT_CONFIG),
                      0, 0, 0, -1,
                      options, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbCrtJSProcedure( sdbConnectionHandle cHandle,
                                    const CHAR *code )
{
   INT32 rc         = SDB_OK ;
   BOOLEAN bsoninit = FALSE ;
   bson bs ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   BSON_INIT( bs );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !code )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( bs, FIELD_NAME_FUNC, code, code ) ;
   BSON_APPEND( bs, FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS, int );

   BSON_FINISH( bs ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_CRT_PROCEDURE),
                      0, 0, 0, -1,
                      &bs, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( bs ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbRmProcedure( sdbConnectionHandle cHandle,
                                 const CHAR *spName )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson bs ;

   BSON_INIT( bs ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !spName )
   {
      rc = SDB_INVALIDARG ;
     goto error ;
   }

   BSON_APPEND( bs, FIELD_NAME_FUNC, spName, string ) ;
   BSON_FINISH( bs ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_RM_PROCEDURE),
                      0, 0, 0, -1,
                      &bs, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( bs ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbListProcedures( sdbConnectionHandle cHandle,
                                    bson *condition,
                                    sdbCursorHandle *handle )
{
   return sdbGetList( cHandle, SDB_LIST_STOREPROCEDURES, condition, NULL, NULL,
                      handle ) ;
}

SDB_EXPORT INT32 sdbEvalJS(sdbConnectionHandle cHandle,
                           const CHAR *code,
                           SDB_SPD_RES_TYPE *type,
                           sdbCursorHandle *handle,
                           bson *errmsg )
{
   INT32 rc                = SDB_OK ;
   SINT64 contextID        = 0 ;
   sdbCursorStruct *cursor = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN r               = FALSE ;
   BOOLEAN bsoninit        = FALSE ;
   bson bs ;

   BSON_INIT( bs );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !code || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !type )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( bs, FIELD_NAME_FUNC, code, code );
   BSON_APPEND( bs, FIELD_NAME_FUNCTYPE, FMP_FUNC_TYPE_JS, int ) ;
   BSON_FINISH( bs ) ;
   // CAN NOT add FLG_QUERY_WITH_RETURNDATA flag, for the return result
   // is not the expected result
   rc = clientBuildQueryMsg( &(connection->_pSendBuffer),
                             &(connection->_sendBufferSize),
                             (CMD_ADMIN_PREFIX CMD_NAME_EVAL),
                             0, 0, 0, -1,
                             &bs, NULL, NULL, NULL,
                             connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      ossPrintf ( "Failed to build flush msg, rc = %d"OSS_NEWLINE, rc ) ;
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extractEval ( (MsgHeader*)connection->_pReceiveBuffer,
                       connection->_receiveBufferSize, &contextID,
                       type, &r, errmsg, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
   ALLOC_HANDLE( cursor, sdbCursorStruct ) ;
   INIT_CURSOR( cursor, connection, connection, contextID );

   // register cursor in connection
   rc = _regCursor ( cursor->_connection, (sdbCursorHandle)cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // set output result
   *handle = (sdbCursorHandle)cursor ;
done:
   BSON_DESTROY( bs ) ;
   if ( NULL != errmsg )
   {
      bson_finish( errmsg ) ;
   }
   return rc ;
error:
   if ( cursor )
   {
      SDB_OSS_FREE ( cursor ) ;
   }

   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetCollection1 ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName,
                                     sdbCollectionHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *pTestCollection           = CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION ;
   CHAR *pName                     = FIELD_NAME_NAME ;
   CHAR fullCollectionName [ CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 2 ] = {0};
   sdbCollectionStruct *s          = NULL ;
   sdbConnectionStruct *connection = NULL ;
   sdbCSStruct *cs                 = (sdbCSStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   connection                      = ( sdbConnectionStruct *)(cs->_connection) ;

   if ( !pCollectionName || !handle ||
        ossStrlen ( pCollectionName) > CLIENT_COLLECTION_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy ( fullCollectionName, cs->_CSName, sizeof(cs->_CSName) ) ;
   ossStrncat ( fullCollectionName, ".", 1 ) ;
   ossStrncat ( fullCollectionName, pCollectionName, CLIENT_COLLECTION_NAMESZ );

   if ( fetchCachedObject( connection->_tb, fullCollectionName ) )
   {
      // DO NOTHING
   }
   else
   {
      BSON_APPEND( newObj, pName, fullCollectionName, string ) ;
      BSON_FINISH ( newObj ) ;

      rc = _runCommand ( cs->_connection, cs->_sock, &cs->_pSendBuffer,
                         &cs->_sendBufferSize,
                         &cs->_pReceiveBuffer,
                         &cs->_receiveBufferSize,
                         cs->_endianConvert,
                         pTestCollection, &newObj,
                         NULL, NULL, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( connection->_tb, fullCollectionName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

   ALLOC_HANDLE( s, sdbCollectionStruct ) ;
   s->_handleType    = SDB_HANDLE_TYPE_COLLECTION ;
   s->_connection    = cs->_connection ;
   s->_sock          = cs->_sock ;
   s->_endianConvert = cs->_endianConvert ;
   rc = _setCollectionName ( (sdbCollectionHandle)s, fullCollectionName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   _regSocket( cs->_connection, &s->_sock ) ;
   *handle = (sdbCollectionHandle)s ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( s )
   {
      SDB_OSS_FREE ( s ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbCreateCollection1 ( sdbCSHandle cHandle,
                                        const CHAR *pCollectionName,
                                        bson *options,
                                        sdbCollectionHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   CHAR *pTestCollection           = CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ;
   CHAR *pName                     = FIELD_NAME_NAME ;
   CHAR fullCollectionName [ CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 2 ] = {0};
   sdbCollectionStruct *s          = NULL ;
   sdbConnectionStruct *connection = NULL ;
   sdbCSStruct *cs                 = (sdbCSStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   connection                      = (sdbConnectionStruct *)(cs->_connection) ;
   if ( !pCollectionName || !handle ||
        ossStrlen ( pCollectionName) > CLIENT_COLLECTION_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy ( fullCollectionName, cs->_CSName, sizeof(cs->_CSName) ) ;
   ossStrncat ( fullCollectionName, ".", 1 ) ;
   ossStrncat ( fullCollectionName, pCollectionName, CLIENT_COLLECTION_NAMESZ );
   BSON_APPEND( newObj, pName, fullCollectionName, string ) ;
   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND ( newObj, NULL, &it, element ) ;
      }
   }
   BSON_FINISH ( newObj ) ;
   rc = _runCommand ( cs->_connection, cs->_sock, &cs->_pSendBuffer,
                      &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      cs->_endianConvert,
                      pTestCollection, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   ALLOC_HANDLE( s, sdbCollectionStruct ) ;
   s->_handleType    = SDB_HANDLE_TYPE_COLLECTION ;
   s->_connection    = cs->_connection ;
   s->_sock          = cs->_sock ;
   s->_endianConvert = cs->_endianConvert ;
   rc = _setCollectionName ( (sdbCollectionHandle)s, fullCollectionName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = insertCachedObject( connection->_tb, fullCollectionName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   _regSocket( cs->_connection, &s->_sock ) ;
   *handle = (sdbCollectionHandle)s ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( s )
   {
       SDB_OSS_FREE ( s ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbCreateCollection ( sdbCSHandle cHandle,
                                       const CHAR *pCollectionName,
                                       sdbCollectionHandle *handle )
{
   return sdbCreateCollection1 ( cHandle, pCollectionName, NULL, handle ) ;
}

static INT32 _sdbAlterCollectionV1 ( sdbCollectionHandle cHandle,
                                     bson *options  )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct *)(cs->_connection) ;
   if ( !options ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME,
                cs->_collectionFullName, string ) ;

   BSON_APPEND( newObj, FIELD_NAME_OPTIONS, options, bson ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _sdbAlterCollectionV2( sdbCollectionHandle cHandle,
                                    bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson_iterator itr ;
   bson obj ;
   bson_type alterType = BSON_EOO ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection              = (sdbConnectionStruct*)(cs->_connection) ;
   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_ALTER_TYPE, SDB_CATALOG_CL, string ) ;
   BSON_APPEND( obj, FIELD_NAME_VERSION, SDB_ALTER_VERSION, int ) ;
   BSON_APPEND( obj, FIELD_NAME_NAME, cs->_collectionFullName, string ) ;

   alterType = bson_find( &itr, options, FIELD_NAME_ALTER ) ;

   if ( BSON_OBJECT == alterType || BSON_ARRAY == alterType )
   {
      rc = bson_append_element( &obj, NULL, &itr ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else
   {
       rc = SDB_INVALIDARG ;
       goto error ;
   }

   /// optional
   if ( BSON_OBJECT == bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
   {
      rc = bson_append_element( &obj, NULL, &itr ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if ( BSON_EOO != bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_FINISH( obj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION),
                      0, 0, 0, -1,
                      &obj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbAlterCollection ( sdbCollectionHandle cHandle,
                                      bson *options  )
{
   INT32 rc = SDB_OK ;
   bson_iterator i ;

   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( BSON_EOO == bson_find( &i, options, FIELD_NAME_ALTER ) )
   {
      rc = _sdbAlterCollectionV1( cHandle, options ) ;
   }
   else
   {
      rc = _sdbAlterCollectionV2( cHandle, options ) ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDropCollection ( sdbCSHandle cHandle,
                                     const CHAR *pCollectionName )
{
   INT32 rc                        = SDB_OK ;
   CHAR *pTestCollection           = CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ;
   CHAR *pName                     = FIELD_NAME_NAME ;
   CHAR fullCollectionName [ CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 2 ] = {0};
   sdbConnectionStruct *connection = NULL ;
   sdbCSStruct *cs                 = (sdbCSStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !pCollectionName ||
        ossStrlen ( pCollectionName) > CLIENT_COLLECTION_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossStrncpy ( fullCollectionName, cs->_CSName, sizeof(cs->_CSName) ) ;
   ossStrncat ( fullCollectionName, ".", 1 ) ;
   ossStrncat ( fullCollectionName, pCollectionName, CLIENT_COLLECTION_NAMESZ );
   BSON_APPEND( newObj, pName, fullCollectionName, string ) ;
   BSON_FINISH ( newObj ) ;
   rc = _runCommand ( cs->_connection, cs->_sock, &cs->_pSendBuffer,
                      &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      cs->_endianConvert,
                      pTestCollection, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = removeCachedObject( connection->_tb, fullCollectionName, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetCSName ( sdbCSHandle cHandle,
                                CHAR *pBuffer, INT32 size )
{
   INT32 rc                        = SDB_OK ;
   INT32 name_len                  = 0 ;
   sdbCSStruct *cs                 = (sdbCSStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   if ( NULL == pBuffer )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( size <= 0 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   name_len = ossStrlen( cs->_CSName ) ;
   if ( size < name_len + 1 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }
   ossStrncpy( pBuffer, cs->_CSName, name_len ) ;
   pBuffer[name_len] = 0 ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbRenameCollection( sdbCSHandle cHandle,
                                      const CHAR *pOldName,
                                      const CHAR *pNewName,
                                      bson *options )
{
   INT32 rc                        = SDB_OK ;
   CHAR fullCollectionName [ CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 2 ] = {0};
   sdbCSStruct *cs                 = (sdbCSStruct*)cHandle ;
   sdbConnectionStruct *connection = NULL ;
   BOOLEAN bsoninit                = FALSE ;
   bson query ;

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   if ( !pOldName || !*pOldName || !pNewName || !*pNewName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   BSON_APPEND( query, FIELD_NAME_COLLECTIONSPACE, cs->_CSName, string ) ;
   BSON_APPEND( query, FIELD_NAME_OLDNAME, pOldName, string ) ;
   BSON_APPEND( query, FIELD_NAME_NEWNAME, pNewName, string ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_OLDNAME ) &&
              0 != ossStrcmp( key, FIELD_NAME_NEWNAME ) &&
              0 != ossStrcmp( key, FIELD_NAME_COLLECTIONSPACE ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;

   rc = _runCommand2( cs->_connection, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTION,
                      0, 0, -1, -1,
                      &query, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   ossStrncpy ( fullCollectionName, cs->_CSName, sizeof(cs->_CSName) ) ;
   ossStrncat ( fullCollectionName, ".", 1 ) ;
   ossStrncat ( fullCollectionName, pOldName, CLIENT_COLLECTION_NAMESZ );
   rc = removeCachedObject( connection->_tb, fullCollectionName, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( query ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbAlterCollectionSpace ( sdbCSHandle cHandle,
                                           bson * options )
{
   INT32 rc = SDB_OK ;

   const CHAR * command = CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION_SPACE ;

   sdbCSStruct * cs = ( sdbCSStruct * )cHandle ;
   sdbConnectionStruct * connection = NULL ;

   BOOLEAN bsoninit = FALSE ;
   bson newObj ;
   bson_iterator itr ;
   bson_type alterType = BSON_EOO ;

   BSON_INIT( newObj ) ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   connection = ( sdbConnectionStruct  *)( cs->_connection ) ;

   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   alterType = bson_find( &itr, options, FIELD_NAME_ALTER ) ;
   if ( BSON_EOO == alterType )
   {
      BSON_APPEND( newObj, FIELD_NAME_NAME, cs->_CSName, string ) ;
      BSON_APPEND( newObj, FIELD_NAME_OPTIONS, options, bson ) ;
   }
   else
   {
      BSON_APPEND( newObj, FIELD_NAME_ALTER_TYPE, SDB_CATALOG_CS, string ) ;
      BSON_APPEND( newObj, FIELD_NAME_VERSION, SDB_ALTER_VERSION, int ) ;
      BSON_APPEND( newObj, FIELD_NAME_NAME, cs->_CSName, string ) ;
      if ( BSON_OBJECT == alterType || BSON_ARRAY == alterType )
      {
         rc = bson_append_element( &newObj, NULL, &itr ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else
      {
          rc = SDB_INVALIDARG ;
          goto error ;
      }

      /// optional
      if ( BSON_OBJECT == bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
      {
         rc = bson_append_element( &newObj, NULL, &itr ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else if ( BSON_EOO != bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }

   BSON_FINISH( newObj ) ;

   rc = _runCommand( cs->_connection, connection->_sock,
                     &( connection->_pSendBuffer ),
                     &( connection->_sendBufferSize ),
                     &( connection->_pReceiveBuffer ),
                     &( connection->_receiveBufferSize ),
                     cs->_endianConvert, command, &newObj,
                     NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

INT32 _sdbAlterCollectionSpaceInternal ( sdbCSHandle cHandle,
                                         const CHAR * taskName,
                                         const bson * options,
                                         BOOLEAN allowNullArgs )
{
   INT32 rc = SDB_OK ;

   sdbCSStruct * cs = ( sdbCSStruct * )cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;

   if ( NULL == taskName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _sdbBuildAlterObject( taskName, options, allowNullArgs, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbAlterCollectionSpace( cHandle, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( obj ) ;
   return rc ;

error :
   goto done ;
}

SDB_EXPORT INT32 sdbCSSetDomain ( sdbCSHandle cHandle,
                                  bson * options )
{
   return _sdbAlterCollectionSpaceInternal( cHandle, SDB_ALTER_CS_SET_DOMAIN, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbCSGetDomainName ( sdbCSHandle cHandle,
                                      CHAR *pResult, INT32 size )
{
   INT32 rc                                              = SDB_OK ;
   sdbCSStruct *cs                                       = ( sdbCSStruct* )cHandle ;
   CHAR sql[ CLIENT_SQL_MAX_LEN + CLIENT_CS_NAMESZ + 1 ] = { 0 } ;
   sdbCursorHandle cursor                                = SDB_INVALID_HANDLE ;
   bson_type type                                        = SDB_DMS_EOC ;
   bson tempObj ;
   bson_iterator iter ;

   bson_init( &tempObj ) ;
   if ( !cs->_connection || '\0' == cs->_CSName[0] || 0 >= size  || NULL == pResult )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossMemset( pResult, 0, size ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   // build sql
   ossSnprintf( sql, CLIENT_SQL_MAX_LEN + CLIENT_CS_NAMESZ,
                "select Domain from $LIST_CS where Name = '%s'",
                cs->_CSName ) ;
   rc = sdbExec ( cs->_connection, sql, &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = sdbNext ( cursor, &tempObj ) ;
   if ( SDB_OK != rc )
   {
      // SDB_DMS_EOC will return because the collectionspace was deleted
      // and this error code doesn't need to be exposed.
      // SDB_DMS_CS_NOTEXIST is better.
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_DMS_CS_NOTEXIST ;
      }
      goto error ;
   }
   //if domain exists, server retruns { "Domain": "xxx" }
   //if domain does not exist, server returns { "Domain": null }
   type = bson_find ( &iter, &tempObj, "Domain" ) ;
   if ( BSON_NULL != type )
   {
      ossStrncpy( pResult, bson_iterator_string( &iter ), size - 1 ) ;
   }
   else
   {
      pResult[0] = '\0' ;
   }

done :
   bson_destroy( &tempObj ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCSRemoveDomain ( sdbCSHandle cHandle )
{
   return _sdbAlterCollectionSpaceInternal( cHandle, SDB_ALTER_CS_REMOVE_DOMAIN, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbCSEnableCapped ( sdbCSHandle cHandle )
{
   return _sdbAlterCollectionSpaceInternal( cHandle, SDB_ALTER_CS_ENABLE_CAPPED, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbCSDisableCapped ( sdbCSHandle cHandle )
{
   return _sdbAlterCollectionSpaceInternal( cHandle, SDB_ALTER_CS_DISABLE_CAPPED, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbCSSetAttributes ( sdbCSHandle cHandle,
                                      bson * options )
{
   return _sdbAlterCollectionSpaceInternal( cHandle, SDB_ALTER_CS_SET_ATTR, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbCSListCollections ( sdbCSHandle cHandle,
                                        sdbCursorHandle *handle )
{
   INT32 rc                                  = SDB_OK ;
   sdbCSStruct *cs                           = ( sdbCSStruct* ) cHandle ;
   BOOLEAN bsoninit                          = FALSE ;
   CHAR lowBound[ CLIENT_CS_NAMESZ + 1 + 1 ] = { 0 } ;
   CHAR upBound[ CLIENT_CS_NAMESZ + 1 + 1 ]  = { 0 } ;
   bson condition ;
   bson subObj ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   if ( !cs->_connection || '\0' == cs->_CSName[0] || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossStrncpy( lowBound, cs->_CSName, CLIENT_CS_NAMESZ ) ;
   ossStrncat( lowBound, ".", 1 ) ;
   ossStrncpy( upBound, cs->_CSName, CLIENT_CS_NAMESZ ) ;
   ossStrncat( upBound, "/", 1 ) ;
   BSON_INIT( condition ) ;
   BSON_INIT( subObj ) ;
   //build condition bson
   BSON_APPEND( subObj, "$gt", lowBound, string ) ;
   BSON_APPEND( subObj, "$lt", upBound, string ) ;
   BSON_FINISH( subObj ) ;
   BSON_APPEND( condition, FIELD_NAME_NAME, &subObj, bson ) ;
   BSON_FINISH( condition ) ;
   rc = sdbGetList ( cs->_connection, SDB_LIST_COLLECTIONS, &condition,
                     NULL, NULL, handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( subObj ) ;
   BSON_DESTROY( condition ) ;
   return rc ;
error:
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetCLName ( sdbCollectionHandle cHandle,
                                CHAR *pBuffer, INT32 size )
{
   INT32 rc                        = SDB_OK ;
   INT32 name_len                  = 0 ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION) ;
   if ( NULL == pBuffer )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( size <= 0 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   name_len = ossStrlen( cs->_collectionName ) ;
   if ( size < name_len + 1 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }
   ossStrncpy( pBuffer, cs->_collectionName, name_len ) ;
   pBuffer[name_len] = 0 ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetCLFullName ( sdbCollectionHandle cHandle,
                                    CHAR *pBuffer, INT32 size )
{
   INT32 rc                        = SDB_OK ;
   INT32 name_len                  = 0 ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION) ;
   if ( NULL == pBuffer )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( size <= 0 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   name_len = ossStrlen( cs->_collectionFullName ) ;
   if ( size < name_len + 1 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }
   ossStrncpy( pBuffer, cs->_collectionFullName, name_len ) ;
   pBuffer[name_len] = 0 ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbSplitCollection ( sdbCollectionHandle cHandle,
                                      const CHAR *pSourceGroup,
                                      const CHAR *pTargetGroup,
                                      const bson *pSplitCondition,
                                      const bson *pSplitEndCondition )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT ( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !pSourceGroup || !pTargetGroup || !pSplitCondition ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, CAT_COLLECTION_NAME,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, CAT_SOURCE_NAME, pSourceGroup, string ) ;
   BSON_APPEND( newObj, CAT_TARGET_NAME, pTargetGroup, string ) ;
   BSON_APPEND( newObj, CAT_SPLITQUERY_NAME, pSplitCondition, bson ) ;

   if ( pSplitEndCondition )
   {
      BSON_APPEND( newObj, CAT_SPLITENDQUERY_NAME, pSplitEndCondition, bson ) ;
   }

   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_SPLIT),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbSplitCLAsync ( sdbCollectionHandle cHandle,
                                   const CHAR *pSourceGroup,
                                   const CHAR *pTargetGroup,
                                   const bson *pSplitCondition,
                                   const bson *pSplitEndCondition,
                                   SINT64 *taskID )
{
   INT32 rc                = SDB_OK ;
   sdbCursorHandle cursor  = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct *)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson_iterator it ;
   bson newObj ;
   bson retObj ;

   BSON_INIT( newObj );
   BSON_INIT( retObj );
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct *)(cs->_connection) ;

   // check arguments
   if ( !pSourceGroup || !pTargetGroup || !pSplitCondition || !taskID ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // append
   BSON_APPEND( newObj, CAT_COLLECTION_NAME,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, CAT_SOURCE_NAME, pSourceGroup, string ) ;
   BSON_APPEND( newObj, CAT_TARGET_NAME, pTargetGroup, string ) ;
   BSON_APPEND( newObj, CAT_SPLITQUERY_NAME, pSplitCondition, bson ) ;

   if ( NULL != pSplitEndCondition )
   {
      BSON_APPEND( newObj, CAT_SPLITENDQUERY_NAME,
                   pSplitEndCondition, bson ) ;
   }
   // async:true
   BSON_APPEND( newObj, FIELD_NAME_ASYNC, TRUE, bool ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_SPLIT),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // get the taskid
   rc = sdbNext ( cursor, &retObj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( BSON_LONG == bson_find ( &it, &retObj, FIELD_NAME_TASKID ) )
   {
      *taskID = bson_iterator_long ( &it ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   BSON_DESTROY( retObj ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbSplitCollectionByPercent( sdbCollectionHandle cHandle,
                                              const CHAR * pSourceGroup,
                                              const CHAR * pTargetGroup,
                                              double percent )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;
   if ( percent <= 0.0 || percent > 100.0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !pSourceGroup || !pTargetGroup || !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, CAT_COLLECTION_NAME,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, CAT_SOURCE_NAME, pSourceGroup, string ) ;
   BSON_APPEND( newObj, CAT_TARGET_NAME, pTargetGroup, string ) ;
   BSON_APPEND( newObj, CAT_SPLITPERCENT_NAME, percent, double ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_SPLIT),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbSplitCLByPercentAsync ( sdbCollectionHandle cHandle,
                                            const CHAR *pSourceGroup,
                                            const CHAR *pTargetGroup,
                                            FLOAT64 percent,
                                            SINT64 *taskID )
{
   INT32 rc                        = SDB_OK ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct *)cHandle ;
   BOOLEAN bsoninit                = FALSE;
   bson_iterator it ;
   bson newObj ;
   bson retObj ;

   BSON_INIT( newObj ) ;
   BSON_INIT( retObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct *)(cs->_connection) ;
   if ( percent <= 0.0 || percent > 100.0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !pSourceGroup || !pTargetGroup ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, CAT_COLLECTION_NAME,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, CAT_SOURCE_NAME, pSourceGroup, string ) ;
   BSON_APPEND( newObj, CAT_TARGET_NAME, pTargetGroup, string ) ;
   BSON_APPEND( newObj, CAT_SPLITPERCENT_NAME, percent, double ) ;
   // async:true
   BSON_APPEND( newObj, FIELD_NAME_ASYNC, TRUE, bool ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_SPLIT),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // get return tasdkID
   rc = sdbNext ( cursor, &retObj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( BSON_LONG == bson_find ( &it, &retObj, FIELD_NAME_TASKID ) )
   {
      *taskID = bson_iterator_long ( &it ) ;
   }
   else
   {
      rc = SDB_SYS ;
     goto error;
   }

done :
   BSON_DESTROY( newObj ) ;
   BSON_DESTROY( retObj ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _sdbCreateIndex( sdbCollectionHandle cHandle,
                              bson *indexDef,
                              const CHAR *pIndexName,
                              BOOLEAN isUnique,
                              BOOLEAN isEnforced,
                              INT32 sortBufferSize )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN indexInit               = FALSE;
   BOOLEAN newInit                 = FALSE ;
   BOOLEAN hintInit                = FALSE ;
   bson *hint                      = NULL ;
   bson indexObj ;
   bson newObj ;
   bson hintObj ;

   BSON_INIT2( indexObj, indexInit ) ;
   BSON_INIT2( newObj, newInit ) ;
   BSON_INIT2( hintObj, hintInit ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] || !indexDef || !pIndexName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( sortBufferSize < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( indexObj, IXM_FIELD_NAME_KEY, indexDef, bson ) ;
   BSON_APPEND( indexObj, IXM_FIELD_NAME_NAME, pIndexName, string ) ;
   BSON_APPEND( indexObj, IXM_FIELD_NAME_UNIQUE, isUnique, bool ) ;
   BSON_APPEND( indexObj, IXM_FIELD_NAME_ENFORCED, isEnforced, bool ) ;
   BSON_FINISH ( indexObj ) ;

   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;

   BSON_APPEND( newObj, FIELD_NAME_INDEX, &indexObj, bson ) ;
   BSON_APPEND( newObj, IXM_FIELD_NAME_SORT_BUFFER_SIZE, sortBufferSize, int ) ;
   BSON_FINISH ( newObj ) ;

   // For Compatibility with older engine( version <3.4 ), keep sort buffer
   // size in hint. After several versions, we can delete it.
   BSON_APPEND( hintObj, IXM_FIELD_NAME_SORT_BUFFER_SIZE, sortBufferSize, int ) ;
   BSON_FINISH ( hintObj ) ;
   hint = &hintObj ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, hint,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY2( indexObj, indexInit ) ;
   BSON_DESTROY2( newObj, newInit ) ;
   BSON_DESTROY2( hintObj, hintInit ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _sdbCreateIndexV1( sdbCollectionHandle cHandle,
                                bson *indexDef,
                                const CHAR *pIndexName,
                                bson *options )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   bson matcher, hint, index ;
   BOOLEAN bsoninit = FALSE ;

   BSON_INIT( matcher ) ;
   BSON_INIT( hint ) ;
   BSON_INIT( index ) ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || !indexDef || !pIndexName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // for example, build below message:
   // macher: { Collection: "foo.bar",
   //           Index:{ key: {a:1}, name: 'aIdx', Unique: true,
   //                   Enforced: true, NotNull: true },
   //           SortBufferSize: 1024 }

   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key ( &it ) ;
         if ( ossStrcmp ( key, IXM_FIELD_NAME_SORT_BUFFER_SIZE ) == 0 )
         {
            BSON_APPEND( matcher, NULL, &it, element ) ;
            // For Compatibility with older engine( version <3.4 ), keep sort buffer
            // size in hint. After several versions, we can delete it.
            BSON_APPEND( hint, NULL, &it, element ) ;
         }
         else
         {
            BSON_APPEND( index, NULL, &it, element ) ;
         }
      }
   }
   BSON_FINISH ( hint ) ;

   BSON_APPEND( index, IXM_FIELD_NAME_KEY, indexDef, bson ) ;
   BSON_APPEND( index, IXM_FIELD_NAME_NAME, pIndexName, string ) ;
   BSON_FINISH ( index ) ;

   BSON_APPEND( matcher, FIELD_NAME_COLLECTION, cs->_collectionFullName,
                string ) ;
   BSON_APPEND( matcher, FIELD_NAME_INDEX, &index, bson ) ;
   BSON_FINISH ( matcher ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX),
                      0, 0, 0, -1,
                      &matcher, NULL, NULL, &hint,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( index ) ;
   BSON_DESTROY( matcher ) ;
   BSON_DESTROY( hint ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCreateIndex ( sdbCollectionHandle cHandle,
                                  bson *indexDef,
                                  const CHAR *pIndexName,
                                  BOOLEAN isUnique,
                                  BOOLEAN isEnforced )
{
   return _sdbCreateIndex( cHandle, indexDef, pIndexName, isUnique, isEnforced,
                           SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE ) ;
}

SDB_EXPORT INT32 sdbCreateIndex1 ( sdbCollectionHandle cHandle,
                                   bson *indexDef,
                                   const CHAR *pIndexName,
                                   BOOLEAN isUnique,
                                   BOOLEAN isEnforced,
                                   INT32 sortBufferSize )
{
   return _sdbCreateIndex( cHandle, indexDef, pIndexName, isUnique, isEnforced, sortBufferSize ) ;
}

SDB_EXPORT INT32 sdbCreateIndex2 ( sdbCollectionHandle cHandle,
                                   bson *indexDef,
                                   const CHAR *pIndexName,
                                   bson *options )
{
   return _sdbCreateIndexV1( cHandle, indexDef, pIndexName, options ) ;
}

static INT32 _sdbGetIndexes ( sdbCollectionHandle cHandle,
                              const CHAR *pIndexName,
                              sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson queryCond ;
   bson newObj ;

   BSON_INIT( queryCond ) ;
   BSON_INIT( newObj ) ;

   if ( !handle )
   {
      rc = SDB_CLT_INVALID_HANDLE ;
      goto error ;
   }
   *handle = SDB_INVALID_HANDLE ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   /* build query condition */
   if ( pIndexName )
   {
      BSON_APPEND( queryCond, IXM_FIELD_NAME_INDEX_DEF "."IXM_FIELD_NAME_NAME,
                   pIndexName, string ) ;
      BSON_FINISH ( queryCond ) ;
   }
   /* build collection name */
   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES,
                      0, 0, -1, -1,
                      pIndexName ? &queryCond : NULL,
                      NULL, NULL, &newObj,
                      handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( queryCond ) ;
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( handle && SDB_INVALID_HANDLE != *handle )
   {
      sdbReleaseCursor ( *handle ) ;
      SET_INVALID_HANDLE( handle ) ;
   }
   goto done ;
}

SDB_EXPORT INT32 sdbGetIndexes ( sdbCollectionHandle cHandle,
                                 const CHAR *pIndexName,
                                 sdbCursorHandle *handle )
{
   return _sdbGetIndexes( cHandle, pIndexName, handle ) ;
}

SDB_EXPORT INT32 sdbGetIndex ( sdbCollectionHandle cHandle,
                               const CHAR *pIndexName,
                               bson *info )
{
   INT32 rc               = SDB_OK ;
   BOOLEAN hasInit        = FALSE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;

   if ( !pIndexName || !*pIndexName || !info )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   bson_init( &obj ) ;
   hasInit = TRUE ;

   rc = _sdbGetIndexes( cHandle, pIndexName, &cursor ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = sdbNext( cursor, &obj ) ;
   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_IXM_NOTEXIST ;
      goto error ;
   }
   else if ( rc )
   {
      goto error ;
   }
   rc = bson_copy( info, &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto done ;
   }

done:
   if ( hasInit )
   {
      bson_destroy( &obj ) ;
   }
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbCloseCursor( cursor ) ;
      sdbReleaseCursor( cursor ) ;
   }
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetIndexInfo ( sdbCollectionHandle cHandle,
                                   sdbCursorHandle *handle )
{
   INT32 rc = SDB_OK ;

   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = _sdbGetIndexes( cHandle, NULL, handle ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDropIndex ( sdbCollectionHandle cHandle,
                                const CHAR *pIndexName )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson indexObj ;
   bson newObj ;

   BSON_INIT( indexObj ) ;
   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] ||
        !pIndexName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( indexObj, "", pIndexName, string ) ;
   BSON_FINISH ( indexObj ) ;
   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, FIELD_NAME_INDEX,  &indexObj, bson ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( indexObj ) ;
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetCount ( sdbCollectionHandle cHandle,
                               bson *condition,
                               SINT64 *count )
{
   return sdbGetCount1(cHandle, condition, NULL, count) ;
}

SDB_EXPORT INT32 sdbGetCount1 ( sdbCollectionHandle cHandle,
                                bson *condition,
                                bson *hint,
                                SINT64 *count )
{
   INT32 rc                        = SDB_OK ;
   bson_iterator it ;
   bson newObj ;
   bson retObj ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &newObj ) ;
   bson_init( &retObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !count )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // build collection name
   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;
   // add the hint when it's need
   if ( hint )
   {
      BSON_APPEND( newObj, FIELD_NAME_HINT, hint, bson ) ;
   }
   BSON_FINISH ( newObj ) ;

   // run command
   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                      FLG_QUERY_WITH_RETURNDATA, 0, -1, -1,
                      condition, NULL, NULL, &newObj,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return cursor
   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   rc = sdbNext( cursor, &retObj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( BSON_LONG == bson_find ( &it, &retObj, FIELD_NAME_TOTAL ) )
   {
      *count = bson_iterator_long ( &it ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

done :
   bson_destroy( &newObj ) ;
   bson_destroy( &retObj ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbInsert ( sdbCollectionHandle cHandle,
                             bson *obj )
{
   return sdbInsert1 ( cHandle, obj, NULL ) ;
}

SDB_EXPORT INT32 sdbInsert1 ( sdbCollectionHandle cHandle,
                              bson *obj, bson_iterator *id )
{
   INT32 rc = SDB_OK ;
   // insert
   rc = sdbInsert2( cHandle, obj, 0, NULL ) ;
   if ( rc )
   {
      goto error ;
   }
   // return oid
   if ( id != NULL )
   {
      bson_type type = BSON_EOO ;
      type = bson_find ( id, obj, CLIENT_RECORD_ID_FIELD ) ;
      if ( BSON_EOO == type )
      {
         rc = SDB_SYS ;
         goto error ;
      }
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbInsert2 ( sdbCollectionHandle cHandle,
                              bson *obj, INT32 flags,
                              bson *pResult )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = 0 ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   bson_iterator oid_itr ;
   bson result ;

   bson_init( &result ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct *)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] || !obj )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientAppendOID ( obj, &oid_itr ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // Inform coord or data nodes that the '_id' field is included in the record.
   flags |= FLG_INSERT_HAS_ID_FIELD ;

   flags |= FLG_INSERT_RETURNNUM ;
   rc = clientBuildInsertMsg ( &cs->_pSendBuffer, &cs->_sendBufferSize,
                               cs->_collectionFullName, flags, 0, obj,
                               NULL, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock, (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer,
                  cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // check return msg header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   if ( pResult )
   {
      bson_destroy( pResult ) ;
      bson_init( pResult ) ;
      if ( flags & FLG_INSERT_RETURN_OID )
      {
         bson_append_element( pResult, CLIENT_RECORD_ID_FIELD, &oid_itr ) ;
      }
      rc = _getLastResultObj( cs->_connection, &result ) ;
      if ( SDB_OK == rc )
      {
         bson_append_elements( pResult, &result ) ;
      }
      rc = bson_finish( pResult ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   bson_destroy( &result ) ;
   return rc ;
error :
   if ( pResult )
   {
      _resetBsonToEmpty( pResult ) ;
   }
   goto done ;
}

SDB_EXPORT INT32 sdbBulkInsert ( sdbCollectionHandle cHandle,
                                 SINT32 flags, bson **obj, SINT32 num )
{
   flags &= ~FLG_INSERT_RETURN_OID ;
   return sdbBulkInsert2( cHandle, flags, obj, num, NULL ) ;
}

SDB_EXPORT INT32 sdbBulkInsert2 ( sdbCollectionHandle cHandle,
                                  SINT32 flags, bson **obj, SINT32 num,
                                  bson *pResult )
{
   INT32 rc                                   = SDB_OK ;
   SINT64 contextID                           = 0 ;
   SINT32 count                               = 0 ;
   CHAR keyBuf[ CLI_INT_TO_STR_MAX_SIZE + 1 ] = { 0 } ;
   sdbConnectionStruct *connection            = NULL ;
   sdbCollectionStruct *cs                    = (sdbCollectionStruct*)cHandle ;
   bson_iterator oid_itr ;
   bson result ;
   bson oidResult ;

   bson_init( &result ) ;
   bson_init( &oidResult ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] || !obj )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( num < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   else if ( !num )
   {
      // in this case, prevent use the cs->_pSendBuffer to send thing to engine
      goto done ;
   }
   if ( pResult )
   {
      if ( flags & FLG_INSERT_RETURN_OID )
      {
         bson_append_start_array( &oidResult, CLIENT_RECORD_ID_FIELD ) ;
      }
   }
   flags |= FLG_INSERT_RETURNNUM ;

   // Inform coord or data nodes that the '_id' field is included in records.
   flags |= FLG_INSERT_HAS_ID_FIELD ;

   for ( count = 0; count < num; ++count )
   {
      if ( !obj[count] )
      {
         break ;
      }
      if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
      {
         rc = clientAppendOID ( obj[count], &oid_itr ) ;
         if ( rc )
         {
            goto error ;
         }
         clientItoa( count, keyBuf, CLI_INT_TO_STR_MAX_SIZE + 1 ) ;
         bson_append_element( &oidResult, keyBuf, &oid_itr ) ;
      }
      else
      {
         rc = clientAppendOID ( obj[count], NULL ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      if ( 0 == count )
         rc = clientBuildInsertMsg ( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                     cs->_collectionFullName, flags, 0,
                                     obj[count], NULL, cs->_endianConvert ) ;
      else
         rc = clientAppendInsertMsg ( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                      obj[count], cs->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( pResult )
   {
      bson_destroy( pResult ) ;
      bson_init( pResult ) ;
      if ( flags & FLG_INSERT_RETURN_OID )
      {
         bson_append_finish_array( &oidResult ) ;
         rc = bson_finish( &oidResult ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
         bson_append_elements ( pResult, &oidResult ) ;
      }
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer,
                  cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // check return msg header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   if ( pResult )
   {
      rc = _getLastResultObj( cs->_connection, &result ) ;
      if ( SDB_OK == rc )
      {
         bson_append_elements ( pResult, &result ) ;
      }
      rc = bson_finish( pResult )  ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   bson_destroy( &oidResult ) ;
   bson_destroy( &result ) ;
   return rc ;
error :
   if ( pResult )
   {
      _resetBsonToEmpty( pResult ) ;
   }
   goto done ;
}

/*
static INT32 _sdbUpdate ( SOCKET sock, CHAR *pCollectionFullName,
                          CHAR **ppSendBuffer, INT32 *sendBufferSize,
                          CHAR **ppReceiveBuffer, INT32 *receiveBufferSize,
                          BOOLEAN endianConvert,
                          bson *rule, bson *condition, bson *hint )
{
   INT32 rc = SDB_OK ;
   SINT64 contextID ;
   BOOLEAN result ;
   if ( !pCollectionFullName || !ppSendBuffer || !sendBufferSize ||
        !ppReceiveBuffer || !receiveBufferSize || !rule )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = clientBuildUpdateMsg ( ppSendBuffer, sendBufferSize,
                               pCollectionFullName, 0, 0, condition,
                               rule, hint, endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = _send ( sock, (MsgHeader*)(*ppSendBuffer), endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = _recvExtract ( sock, (MsgHeader**)ppReceiveBuffer,
                       receiveBufferSize, &contextID, &result,
                       endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}
*/

SDB_EXPORT INT32 sdbUpdate ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint )
{
   return __sdbUpdate ( cHandle, 0, rule, condition, hint, NULL ) ;
}

SDB_EXPORT INT32 sdbUpdate1 ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint,
                             INT32 flag )
{
   return __sdbUpdate ( cHandle, flag, rule, condition, hint, NULL ) ;
}

SDB_EXPORT INT32 sdbUpdate2 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              INT32 flag,
                              bson *pResult )
{
   return __sdbUpdate ( cHandle, flag, rule, condition, hint, pResult ) ;
}

SDB_EXPORT INT32 sdbUpsert ( sdbCollectionHandle cHandle,
                             bson *rule,
                             bson *condition,
                             bson *hint )
{
   return __sdbUpdate ( cHandle, FLG_UPDATE_UPSERT, rule, condition, hint, NULL ) ;
}

SDB_EXPORT INT32 sdbUpsert1 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              bson *setOnInsert )
{
   return sdbUpsert2 ( cHandle, rule, condition, hint, setOnInsert, 0 ) ;
}

SDB_EXPORT INT32 sdbUpsert2 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              bson *setOnInsert,
                              INT32 flag )
{
   return sdbUpsert3( cHandle, rule, condition, hint, setOnInsert, flag, NULL ) ;
}

SDB_EXPORT INT32 sdbUpsert3 ( sdbCollectionHandle cHandle,
                              bson *rule,
                              bson *condition,
                              bson *hint,
                              bson *setOnInsert,
                              INT32 flag,
                              bson *pResult )
{
   INT32 rc = SDB_OK ;
   bson* hintPtr = hint ;
   bson newHint ;

   bson_init ( &newHint ) ;
   if ( NULL != setOnInsert )
   {
      if ( NULL != hint )
      {
         bson_append_elements( &newHint, hint ) ;
      }

      bson_append_bson( &newHint, FIELD_NAME_SET_ON_INSERT, setOnInsert ) ;
      rc = bson_finish( &newHint ) ;
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      hintPtr = &newHint ;
   }
   rc = __sdbUpdate ( cHandle, flag | FLG_UPDATE_UPSERT,
                      rule, condition, hintPtr, pResult ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &newHint ) ;
   return rc ;
error:
   goto done ;
}

/*
static INT32 _sdbDelete ( SOCKET sock, CHAR *pCollectionFullName,
                          CHAR **ppSendBuffer, INT32 *sendBufferSize,
                          CHAR **ppReceiveBuffer, INT32 *receiveBufferSize,
                          BOOLEAN endianConvert,
                          bson *condition, bson *hint )
{
   INT32 rc = SDB_OK ;
   SINT64 contextID ;
   BOOLEAN result ;
   if ( !pCollectionFullName || !ppSendBuffer || !sendBufferSize ||
        !ppReceiveBuffer || !receiveBufferSize )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = clientBuildDeleteMsg ( ppSendBuffer, sendBufferSize,
                               pCollectionFullName, 0, 0, condition,
                               hint, endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = _send ( sock, (MsgHeader*)(*ppSendBuffer), endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   rc = _recvExtract ( sock, (MsgHeader**)ppReceiveBuffer,
                       receiveBufferSize, &contextID, &result,
                       endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}
*/

SDB_EXPORT INT32 sdbDelete ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *hint )
{
   return sdbDelete1( cHandle, condition, hint, 0, NULL ) ;
}

SDB_EXPORT INT32 sdbDelete1 ( sdbCollectionHandle cHandle,
                              bson *condition,
                              bson *hint,
                              INT32 flag,
                              bson *pResult )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = 0 ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct *)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( pResult )
   {
      flag |= FLG_DELETE_RETURNNUM ;
   }
   rc = clientBuildDeleteMsg ( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                cs->_collectionFullName, flag, 0, condition,
                                hint, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock, (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer, cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   //get result
   if ( pResult )
   {
      bson_destroy( pResult ) ;
      bson_init( pResult ) ;
      _getLastResultObj( cs->_connection, pResult ) ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   if ( pResult )
   {
      _resetBsonToEmpty( pResult ) ;
   }
   goto done ;
}

static INT32 _sdbQuery ( sdbCollectionHandle cHandle,
                              bson *condition,
                              bson *select,
                              bson *orderBy,
                              bson *hint,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              INT32 flags,
                              sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   INT32 newFlags                  = 0 ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   newFlags = regulateQueryFlags( flags ) ;
   newFlags |= QUERY_PREPARE_MORE ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      cs->_collectionFullName,
                      newFlags, 0, numToSkip, numToReturn,
                      condition, select, orderBy, hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   *handle = cursor ;

done :
   return rc ;
error :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbExplain ( sdbCollectionHandle cHandle,
                              bson *condition,
                              bson *selector,
                              bson *orderBy,
                              bson *hint,
                              INT32 flag,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              bson *options,
                              sdbCursorHandle *handle )
{
   INT32 rc = SDB_OK ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( hint )
   {
      BSON_APPEND( newObj, FIELD_NAME_HINT, hint, bson ) ;
   }

   if ( options )
   {
      BSON_APPEND( newObj, FIELD_NAME_OPTIONS, options, bson ) ;
   }
   BSON_FINISH ( newObj ) ;

   if ( 0 != flag )
   {
      flag = eraseSingleFlag( flag, FLG_QUERY_MODIFY ) ;
   }
   rc = _sdbQuery( cHandle, condition, selector, orderBy, &newObj,
                   numToSkip, numToReturn, flag | FLG_QUERY_EXPLAIN,
                   handle ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbQuery ( sdbCollectionHandle cHandle,
                            bson *condition,
                            bson *select,
                            bson *orderBy,
                            bson *hint,
                            INT64 numToSkip,
                            INT64 numToReturn,
                            sdbCursorHandle *handle )
{
   return sdbQuery1 ( cHandle, condition, select, orderBy, hint,
                      numToSkip, numToReturn, 0, handle ) ;
}

SDB_EXPORT INT32 sdbQuery1 ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *select,
                             bson *orderBy,
                             bson *hint,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             INT32 flag,
                             sdbCursorHandle *handle )
{
    // remove "query plan" and "query and modify" flag
    if ( 0 != flag )
    {
       flag = eraseSingleFlag( flag, FLG_QUERY_EXPLAIN ) ;
       flag = eraseSingleFlag( flag, FLG_QUERY_MODIFY ) ;
    }
    return _sdbQuery( cHandle, condition, select, orderBy, hint,
                      numToSkip, numToReturn, flag, handle ) ;
}

static INT32 _sdbQueryAndModify ( sdbCollectionHandle cHandle,
                                  bson *condition,
                                  bson *select,
                                  bson *orderBy,
                                  bson *hint,
                                  bson *update,
                                  INT64 numToSkip,
                                  INT64 numToReturn,
                                  INT32 flag,
                                  BOOLEAN returnNew,
                                  BOOLEAN isUpdate,
                                  sdbCursorHandle *handle )
{
   INT32 rc = SDB_OK ;
   BOOLEAN hintInit = FALSE;
   BOOLEAN modifyInit = FALSE;
   bson newHint ;
   bson modify ;

   BSON_INIT2( modify, modifyInit ) ;
   BSON_INIT2( newHint, hintInit ) ;
   /* create $Modify object */
   if ( isUpdate )
   {
      if ( NULL == update )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      BSON_APPEND( modify, FIELD_NAME_OP, FIELD_OP_VALUE_UPDATE, string ) ;
      BSON_APPEND( modify, FIELD_NAME_OP_UPDATE, update, bson ) ;
      BSON_APPEND( modify, FIELD_NAME_RETURNNEW, returnNew, bool ) ;
   }
   else
   {
      BSON_APPEND( modify, FIELD_NAME_OP, FIELD_OP_VALUE_REMOVE, string ) ;
      BSON_APPEND( modify, FIELD_NAME_OP_REMOVE, TRUE, bool ) ;
   }
   BSON_FINISH( modify ) ;

   /* create new hint */
   if ( NULL != hint )
   {
      rc = _mergeBson( &newHint, hint ) ;
      if ( rc )
      {
         goto error ;
      }
   }
   BSON_APPEND( newHint, FIELD_NAME_MODIFY, &modify, bson ) ;
   BSON_FINISH( newHint ) ;
   if ( 0 != flag )
   {
      flag = eraseSingleFlag( flag, FLG_QUERY_EXPLAIN ) ;
   }
   flag |= FLG_QUERY_MODIFY ;
   rc = _sdbQuery( cHandle, condition, select, orderBy, &newHint,
                     numToSkip, numToReturn, flag, handle ) ;

done:
   BSON_DESTROY2( modify, modifyInit ) ;
   BSON_DESTROY2( newHint, hintInit ) ;
   return rc ;
error:
   goto done ;
}

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
                             sdbCursorHandle *handle )
{
   return _sdbQueryAndModify( cHandle, condition, select, orderBy, hint, update,
                           numToSkip, numToReturn, flag, returnNew, TRUE, handle ) ;
}

SDB_EXPORT INT32 sdbQueryAndRemove ( sdbCollectionHandle cHandle,
                             bson *condition,
                             bson *select,
                             bson *orderBy,
                             bson *hint,
                             INT64 numToSkip,
                             INT64 numToReturn,
                             INT32 flag,
                             sdbCursorHandle *handle )
{
   return _sdbQueryAndModify( cHandle, condition, select, orderBy, hint, NULL,
                           numToSkip, numToReturn, flag, FALSE, FALSE, handle ) ;
}

SDB_EXPORT INT32 sdbNext ( sdbCursorHandle cHandle,
                           bson *obj )
{
   INT32 rc            = SDB_OK ;
   MsgOpReply *pReply  = NULL ;
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;
   BOOLEAN bsoninit    = FALSE;
   bson localobj ;

   BSON_INIT( localobj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CURSOR ) ;
   if ( !obj )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // check whether the cursor had been close
   if ( cs->_isClosed )
   {
      rc = SDB_DMS_CONTEXT_IS_CLOSE ;
      goto error ;
   }

   /*
   if ( cs->_modifiedCurrent )
   {
      bson_destroy ( cs->_modifiedCurrent ) ;
      SDB_OSS_FREE ( cs->_modifiedCurrent ) ;
      cs->_modifiedCurrent = NULL ;
   }
   */
   if ( !cs->_pReceiveBuffer )
   {
      cs->_offset = -1 ;
      rc = _readNextBuffer ( cHandle ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
retry :
   pReply = (MsgOpReply*)cs->_pReceiveBuffer ;
   if ( !pReply )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( -1 == cs->_offset )
   {
      cs->_offset = ossRoundUpToMultipleX ( sizeof(MsgOpReply), 4 ) ;
   }
   else
   {
      cs->_offset += ossRoundUpToMultipleX
            ( *(INT32*)&cs->_pReceiveBuffer[cs->_offset], 4 ) ;
   }
   if ( cs->_offset >= pReply->header.messageLength ||
        cs->_offset >= cs->_receiveBufferSize )
   {
      cs->_offset = -1 ;
      rc = _readNextBuffer ( cHandle ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      goto retry ;
   }
   rc = bson_init_finished_data ( &localobj, &cs->_pReceiveBuffer [ cs->_offset] ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_CORRUPTED_RECORD ;
      goto done ;
   }
   // copy to output result
   rc = bson_copy ( obj, &localobj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto done ;
   }
   ++ cs->_totalRead ;
done :
   BSON_DESTROY( localobj ) ;
//   cs->_isDeleteCurrent = FALSE ;
   return rc ;
error :
   if ( SDB_DMS_EOC == rc )
   {
      INT32 ret = SDB_OK ;
      cs->_contextID = -1 ;
      // when exhaust records, engine will close the context auto
      // so we need to mark here
      cs->_isClosed = TRUE ;
      ret = _unregCursor ( cs->_connection, cHandle ) ;
      if ( ret )
      {
         rc = ret ;
      }
   }
   goto done ;
}

SDB_EXPORT INT32 sdbCurrent ( sdbCursorHandle cHandle,
                              bson *obj )
{
   INT32 rc            = SDB_OK ;
   MsgOpReply *pReply  = NULL ;
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;
   BOOLEAN bsoninit    = FALSE;
   bson localobj ;

   BSON_INIT( localobj );
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CURSOR ) ;
   // check whether the cursor had been close
   if ( cs->_isClosed )
   {
      rc = SDB_DMS_CONTEXT_IS_CLOSE ;
      goto error ;
   }

   //invalid parameter
   if ( !obj )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   /*
   //we can't get the current record when it was deleted
   if(cs->_isDeleteCurrent)
   {
      rc = SDB_CURRENT_RECORD_DELETED ;
      goto error ;
   }
   */

   /*
   if ( cs->_modifiedCurrent )
   {
      //deep copy,never use ossmencpy() which is shallow copy
      rc = bson_copy ( obj, cs->_modifiedCurrent ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
      }
      goto done ;
   }
   */
   if ( !cs->_pReceiveBuffer )
   {
      cs->_offset = -1 ;
      rc = _readNextBuffer ( cHandle ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }
retry :
   pReply = (MsgOpReply*)cs->_pReceiveBuffer ;
   if ( !pReply )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( -1 == cs->_offset )
   {
      cs->_offset = ossRoundUpToMultipleX ( sizeof(MsgOpReply), 4 ) ;
   }

   if ( cs->_offset >= pReply->header.messageLength ||
        cs->_offset >= cs->_receiveBufferSize )
   {
      cs->_offset = -1 ;
      rc = _readNextBuffer ( cHandle ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      goto retry ;
   }
   rc = bson_init_finished_data ( &localobj, &cs->_pReceiveBuffer [ cs->_offset] ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_CORRUPTED_RECORD ;
      goto done ;
   }
   rc = bson_copy ( obj, &localobj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   ++ cs->_totalRead ;
done :
   BSON_DESTROY( localobj ) ;
   return rc ;
error :
   if ( SDB_DMS_EOC == rc )
   {
      INT32 ret = SDB_OK ;
      cs->_contextID = -1 ;
      // when exhaust records, engine will close the context auto
      // so we need to mark here
      cs->_isClosed = TRUE ;
      ret = _unregCursor ( cs->_connection, cHandle ) ;
      if ( ret )
      {
         rc = ret ;
      }
   }
   goto done ;
}

/*
SDB_EXPORT INT32 sdbUpdateCurrent ( sdbCursorHandle cHandle,
                                    bson *rule )
{
   INT32 rc = SDB_OK ;
   // declare variables
   bson obj ;
   bson_init ( &obj ) ;
   bson updateCondition ;
   bson_init ( &updateCondition ) ;
   bson_iterator it ;
   bson hintObj ;
   bson_init ( &hintObj ) ;
   // create index scan hint in order to reduce cost for optimizer
   bson_append_string ( &hintObj, "", CLIENT_RECORD_ID_INDEX ) ;
   bson_finish ( &hintObj ) ;

   // declare buffer variables
   CHAR *pSendBuffer       = NULL ;
   INT32 sendBufferSize    = 0 ;
   CHAR *pReceiveBuffer    = NULL ;
   INT32 receiveBufferSize = 0 ;

   bson modifiedObj ;
   bson_init ( &modifiedObj ) ;

   // declare cursor handle
   sdbCursorHandle tempQuery = SDB_INVALID_HANDLE ;
   // convert handle to struct
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;
   // make sure the handle is valid and the type is correct
   if ( !cs || cs->_handleType != SDB_HANDLE_TYPE_CURSOR )
   {
      rc = SDB_CLT_INVALID_HANDLE ;
      goto error ;
   }
   // validate collection got valid name
   if ( cs->_collectionFullName[0] == '\0' )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   //we can't update the current record when it was deleted
//   if( cs->_isDeleteCurrent )
//   {
//      rc = SDB_CURRENT_RECORD_DELETED ;
//      goto error ;
//   }

   // get the current record from handl
   rc = sdbCurrent ( cHandle, &obj ) ;
   if ( rc )
   {
      goto error ;
   }
   // extract the record id field from object
   if ( BSON_EOO != bson_find ( &it, &obj, CLIENT_RECORD_ID_FIELD ) )
   {
      // construct update condition using the id field
      rc = bson_append_element ( &updateCondition, NULL, &it ) ;
      // sanity check
      if ( rc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      // finish building update condition
      rc = bson_finish ( &updateCondition ) ;
      if ( rc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
   }
   else
   {
      // all record must have _id field
      rc = SDB_CORRUPTED_RECORD ;
      goto error ;
   }
   // perform update command and validate the return value
   rc = _sdbUpdate ( cs->_sock, cs->_collectionFullName,
                     &pSendBuffer, &sendBufferSize,
                     &pReceiveBuffer, &receiveBufferSize,
                      cs->_endianConvert,
                      rule,
                     &updateCondition, &hintObj ) ;
   if ( rc )
   {
      goto error ;
   }
   // after update, we have to perform another query, note the cursor object is
   // stored in tempQuery
   rc = _sdbQuery ( cs->_sock, cs->_collectionFullName, &pSendBuffer,
                    &sendBufferSize, &pReceiveBuffer, &receiveBufferSize,
                    cs->_endianConvert,
                    &updateCondition, NULL, NULL,
                    &hintObj, 0, 1, &tempQuery ) ;
   if ( rc )
   {
      goto error ;
   }
   // extract the record from cursor
   rc = sdbNext ( tempQuery, &modifiedObj ) ;
   if ( rc )
   {
      // we should not hit error even for SDB_DMS_EOC, since the record supposed
      // to be in database, unless the record is deleted before the query
      goto error ;
   }
   // now the new modified data is stored in modifiedObj
   // then let's delete the current one if there's exist
   if ( !cs->_modifiedCurrent )
   {
      // allocate memory for a temporary bson object and save it in the cursor
      // the memory is freed when the cursor move to next record ( call sdbNext
      // ) or destroyed. We need to be careful here to avoid memory leak
      cs->_modifiedCurrent = (bson*)SDB_OSS_MALLOC ( sizeof(bson) ) ;
      if ( !cs->_modifiedCurrent )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      bson_init ( cs->_modifiedCurrent ) ;
   }
   // perform a copy because modifiedObj is local variable, the memory will
   // be freed once release tempQuery handle
   rc = bson_copy ( cs->_modifiedCurrent, &modifiedObj ) ;
   if ( BSON_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
done :
   // release the temporary cursor handle
   if ( SDB_INVALID_HANDLE != tempQuery )
   {
      sdbReleaseCursor ( tempQuery ) ;
      tempQuery = SDB_INVALID_HANDLE ;
   }
   // release send buffer for temporary update request
   if ( pSendBuffer )
   {
      SDB_OSS_FREE ( pSendBuffer ) ;
   }
   // release receive buffer
   if ( pReceiveBuffer )
   {
      SDB_OSS_FREE ( pReceiveBuffer ) ;
   }
   // destroy local bson objects
   bson_destroy ( &updateCondition ) ;
   bson_destroy ( &hintObj ) ;
   bson_destroy ( &modifiedObj ) ;
   return rc ;
error :
   goto done ;
}


SDB_EXPORT INT32 sdbDeleteCurrent ( sdbCursorHandle cHandle )
{
   INT32 rc = SDB_OK ;
   // declare variables
   bson obj ;
   bson_init ( &obj ) ;
   bson updateCondition ;
   bson_init ( &updateCondition ) ;
   bson_iterator it ;
   bson hintObj ;
   bson_init ( &hintObj ) ;
   // create index scan hint in order to reduce cost for optimizer
   bson_append_string ( &hintObj, "", CLIENT_RECORD_ID_INDEX ) ;
   bson_finish ( &hintObj ) ;
   // declare buffer variables
   CHAR *pSendBuffer = NULL ;
   INT32 sendBufferSize = 0 ;
   CHAR *pReceiveBuffer = NULL ;
   INT32 receiveBufferSize = 0 ;
   // convert handle to struct
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;
   if ( !cs || cs->_handleType != SDB_HANDLE_TYPE_CURSOR )
   {
      rc = SDB_CLT_INVALID_HANDLE ;
      goto error ;
   }
   // make sure the handle is valid and the type is correct
   if ( cs->_collectionFullName[0] == '\0' )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   //we can't delete current record twice
   if(cs->_isDeleteCurrent)
   {
      rc = SDB_CURRENT_RECORD_DELETED ;
      goto error ;
   }
   // get the current record from handl
   rc = sdbCurrent ( cHandle, &obj ) ;
   if ( rc )
   {
      goto error ;
   }
   // extract the record id field from object
   if ( BSON_EOO != bson_find ( &it, &obj, CLIENT_RECORD_ID_FIELD ) )
   {
      // construct update condition using the id field
      rc = bson_append_element ( &updateCondition, NULL, &it ) ;
      if ( rc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      // finish building update condition
      rc = bson_finish ( &updateCondition ) ;
      if ( rc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
   }
   else
   {
      // all record must have _id field
      rc = SDB_CORRUPTED_RECORD ;
      goto error ;
   }
   // perform delete command and validate the return value
   rc = _sdbDelete ( cs->_sock, cs->_collectionFullName,
                     &pSendBuffer, &sendBufferSize,
                     &pReceiveBuffer, &receiveBufferSize,
                     cs->_endianConvert,
                     &updateCondition, &hintObj ) ;
   if ( rc )
   {
      goto error ;
   }
   if ( cs->_modifiedCurrent )
   {
      bson_destroy ( cs->_modifiedCurrent ) ;
      SDB_OSS_FREE ( cs->_modifiedCurrent ) ;
      cs->_modifiedCurrent = NULL ;
   }
done :
   if ( pSendBuffer )
   {
      SDB_OSS_FREE ( pSendBuffer ) ;
   }
   if ( pReceiveBuffer )
   {
      SDB_OSS_FREE ( pReceiveBuffer ) ;
   }
   bson_destroy ( &updateCondition ) ;
   bson_destroy ( &hintObj ) ;
   cs->_isDeleteCurrent = TRUE ;
   return rc ;
error :
   goto done ;
}
*/

SDB_EXPORT INT32 sdbCloseCursor ( sdbCursorHandle cHandle )
{
   INT32 rc            = SDB_OK ;
   SINT64 contextID    = -1 ;
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CURSOR ) ;
   // check whether the cursor had been close
   if ( cs->_isClosed )
   {
      goto done ;
   }
   if ( NULL == cs->_sock || -1 == cs->_contextID )
   {
      goto unregister ;
   }

   rc = clientBuildKillContextsMsg ( &cs->_pSendBuffer,
                                     &cs->_sendBufferSize, 0, 1,
                                     &cs->_contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock, (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer, cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;

unregister:
   // unregister from connection handle
   _unregCursor ( cs->_connection, cHandle ) ;
   cs->_contextID = -1 ;
   cs->_isClosed = TRUE ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCloseAllCursors ( sdbConnectionHandle cHandle )
{
   return sdbInterrupt( cHandle ) ;
}

SDB_EXPORT INT32 sdbInterrupt ( sdbConnectionHandle cHandle )
{
   INT32 rc            = SDB_OK ;
   Node *pNodeHandle = NULL ;
   Node *pNextHandle   = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // build msg
   rc = clientBuildInterruptMsg( &connection->_pSendBuffer,
                                 &connection->_sendBufferSize, 0,
                                 FALSE, connection->_endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   // send msg
   rc = _send ( cHandle, connection->_sock, (MsgHeader*)connection->_pSendBuffer,
                connection->_endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   // unregister cursor handles
   pNodeHandle = connection->_cursors ;
   while ( pNodeHandle )
   {
      // mark the cursor to be closed
      ((sdbCursorStruct*)pNodeHandle->data)->_contextID = -1 ;
      ((sdbCursorStruct*)pNodeHandle->data)->_isClosed = TRUE ;
      pNextHandle = pNodeHandle->next ;
      // unregister from connection handle, _unregCursor() will free(pNodeHandle)
      // so, we can not use pNodeHandle after calling _unregCursor()
      _unregCursor ( cHandle, pNodeHandle->data ) ;
      pNodeHandle = pNextHandle ;
   }

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbInterruptOperation ( sdbConnectionHandle cHandle )
{
   INT32 rc            = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // build msg
   rc = clientBuildInterruptMsg( &connection->_pSendBuffer,
                                 &connection->_sendBufferSize, 0,
                                 TRUE, connection->_endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }
   // send msg
   rc = _send ( cHandle, connection->_sock, (MsgHeader*)connection->_pSendBuffer,
                connection->_endianConvert ) ;
   if ( rc )
   {
      goto error ;
   }

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT BOOLEAN sdbIsClosed( sdbConnectionHandle cHandle )
{
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;
   if ( NULL == connection ||
        SDB_HANDLE_TYPE_CONNECTION != connection->_handleType )
   {
      return TRUE ;
   }

   return NULL == connection->_sock ? TRUE : FALSE ;
}

SDB_EXPORT BOOLEAN sdbIsValid( sdbConnectionHandle cHandle )
{
   INT32 ret   = SDB_OK ;
   SOCKET sock = 0 ;
   fd_set fds ;
   struct timeval maxSelectTime = { 0, 1000 }; // 1ms
   sdbConnectionStruct *connection = (sdbConnectionStruct *)cHandle ;

   if ( sdbIsClosed( cHandle ) )
   {
      return FALSE ;
   }
   sock = clientGetRawSocket ( connection->_sock ) ;
   // invalid sock
   if ( sock < 0 )
   {
      return FALSE ;
   }
   while ( TRUE )
   {
      FD_ZERO ( &fds ) ;
      FD_SET ( sock, &fds ) ;
      ret = select ( sock+1, &fds, NULL,  NULL, &maxSelectTime ) ;
      // if = 0, time out, means connection is not closed
      if ( !ret )
      {
         return TRUE ;
      }
      // if < 0, means something wrong
      else if ( ret < 0 )
      {
         // if we failed due to interrupt, let's continue
         if (
#if defined (_WINDOWS)
         WSAEINTR
#else
         EINTR
#endif
         == errno )
         {
            continue ;
         }
         else // else, we failed to select from socket
         {
            return FALSE ;
         }
      }
      else
      {
         return TRUE ;
      }
   } // while
}

SDB_EXPORT INT32 sdbTraceStart ( sdbConnectionHandle cHandle,
                                 UINT32 traceBufferSize,
                                 CHAR * comp,
                                 CHAR * breakPoint ,
                                 UINT32 *tids,
                                 UINT32 nTids )
{
   INT32 rc         = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;
   UINT32 itid = 0 ;
   CHAR key[128] = { 0 } ;

   BSON_INIT( obj );
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // init obj
   BSON_APPEND( obj, FIELD_NAME_SIZE, (INT64)traceBufferSize, long ) ;

   if ( comp )
   {
      rc = bson_append_start_array ( &obj, FIELD_NAME_COMPONENTS ) ;
      if( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error;
      }
      rc = sdbTraceStrtok ( &obj, comp ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = bson_append_finish_array( &obj );
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   if ( breakPoint )
   {
      rc = bson_append_start_array( &obj, FIELD_NAME_BREAKPOINTS );
      if( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error;
      }
      rc = sdbTraceStrtok ( &obj, breakPoint ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = bson_append_finish_array( &obj );
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   if ( nTids )
   {
      rc = bson_append_start_array( &obj, FIELD_NAME_THREADS );
      if( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error;
      }
      for ( ; itid < nTids; itid++ )
      {
         ossSnprintf ( key, sizeof(key), "%d", itid ) ;
         BSON_APPEND( obj, key, tids[itid], int ) ;
      }
      rc = bson_append_finish_array( &obj );
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   BSON_FINISH ( obj ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_TRACE_START, &obj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( obj ) ;
   return rc ;
error :
   goto done ;
}


SDB_EXPORT INT32 sdbTraceResume ( sdbConnectionHandle cHandle )
{
   INT32 rc       = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_TRACE_RESUME, NULL,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbTraceStop ( sdbConnectionHandle cHandle,
                                const CHAR *pDumpFileName )
{
   INT32 rc         = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( pDumpFileName )
   {
      BSON_APPEND( obj, FIELD_NAME_FILENAME, pDumpFileName, string ) ;
   }
   BSON_FINISH ( obj ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_TRACE_STOP, &obj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( obj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbTraceStatus ( sdbConnectionHandle cHandle,
                                  sdbCursorHandle *handle )
{
   INT32 rc                = SDB_OK ;
   sdbCursorHandle cursor  = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS),
                      0, 0, 0, -1,
                      NULL, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   *handle = cursor ;

done :
   return rc ;
error :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}


SDB_EXPORT INT32 sdbExecUpdate( sdbConnectionHandle cHandle,
                                const CHAR *sql )
{
   INT32 rc         = SDB_OK ;
   SINT64 contextID = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !sql )
   {
      rc = SDB_INVALIDARG ;
     goto error ;
   }

   rc = clientValidateSql( sql, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = clientBuildSqlMsg( &connection->_pSendBuffer,
                           &connection->_sendBufferSize, sql,
                           0, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbExec( sdbConnectionHandle cHandle,
                          const CHAR *sql,
                          sdbCursorHandle *result )
{
   INT32 rc                = SDB_OK ;
   SINT64 contextID        = 0 ;
   sdbCursorStruct *cursor = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !result || !sql )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientValidateSql( sql, TRUE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = clientBuildSqlMsg( &connection->_pSendBuffer,
                           &connection->_sendBufferSize, sql,
                           0, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
   ALLOC_HANDLE( cursor, sdbCursorStruct ) ;
   INIT_CURSOR( cursor, connection, connection, contextID ) ;
   // register cursor in connection
   rc = _regCursor ( cursor->_connection, (sdbCursorHandle)cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // set output result
   *result = (sdbCursorHandle)cursor ;
done :
   return rc ;
error :
   if ( cursor )
   {
      SDB_OSS_FREE ( cursor ) ;
   }
   if ( result )
   {
      *result = SDB_INVALID_HANDLE ;
   }
   goto done ;
}

SDB_EXPORT INT32 sdbTransactionBegin( sdbConnectionHandle cHandle )
{
   INT32 rc         = SDB_OK ;
   SINT64 contextID = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = clientBuildTransactionBegMsg( &connection->_pSendBuffer,
                                      &connection->_sendBufferSize,
                                      0, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbTransactionCommit( sdbConnectionHandle cHandle )
{
   INT32 rc         = SDB_OK ;
   SINT64 contextID = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = clientBuildTransactionCommitMsg( &connection->_pSendBuffer,
                                         &connection->_sendBufferSize,
                                         0, NULL,
                                         connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbTransactionRollback( sdbConnectionHandle cHandle )
{
   INT32 rc         = SDB_OK ;
   SINT64 contextID = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = clientBuildTransactionRollbackMsg( &connection->_pSendBuffer,
                                           &connection->_sendBufferSize,
                                           0, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseConnection ( sdbConnectionHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *cs = (sdbConnectionStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CONNECTION ) ;
   sdbCleanLastErrorObj( cHandle ) ;
   if ( cs->_pSendBuffer )
   {
      SDB_OSS_FREE ( cs->_pSendBuffer ) ;
   }
   if ( cs->_pReceiveBuffer )
   {
      SDB_OSS_FREE ( cs->_pReceiveBuffer ) ;
   }
   if ( NULL != cs->_tb )
   {
      releaseHashTable( &(cs->_tb) ) ;
   }
   _sdbClearSessionAttrCache( cs, FALSE ) ;
   if ( cs->_msgConvertor )
   {
      if ( cs->_msgConvertor->_buff )
      {
         SDB_OSS_FREE( cs->_msgConvertor->_buff ) ;
      }
      SDB_OSS_FREE( cs->_msgConvertor ) ;
   }
   SDB_OSS_FREE ( (sdbConnectionStruct*)cHandle ) ;

done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseCollection ( sdbCollectionHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;

   _unregSocket( cs->_connection, &cs->_sock ) ;

   if ( cs->_pSendBuffer )
   {
      SDB_OSS_FREE ( cs->_pSendBuffer ) ;
   }
   if ( cs->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, cs->_pReceiveBuffer, cs->_receiveBufferSize ) ;
      SDB_OSS_FREE ( cs->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbCollectionStruct*)cHandle ) ;

done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseCS ( sdbCSHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbCSStruct *cs = (sdbCSStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CS ) ;
   _unregSocket( cs->_connection, &cs->_sock ) ;

   if ( cs->_pSendBuffer )
   {
      SDB_OSS_FREE (cs->_pSendBuffer ) ;
   }
   if ( cs->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, cs->_pReceiveBuffer, cs->_receiveBufferSize ) ;
      SDB_OSS_FREE ( cs->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbCSStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseReplicaGroup ( sdbReplicaGroupHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbRGStruct *rg = (sdbRGStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, rg, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   _unregSocket( rg->_connection, &rg->_sock ) ;

   if ( rg->_pSendBuffer )
   {
      SDB_OSS_FREE ( rg->_pSendBuffer ) ;
   }
   if ( rg->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, rg->_pReceiveBuffer, rg->_receiveBufferSize ) ;
      SDB_OSS_FREE ( rg->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbRGStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseNode ( sdbNodeHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbRNStruct *rn = (sdbRNStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, rn, SDB_HANDLE_TYPE_REPLICANODE ) ;
   _unregSocket( rn->_connection, &rn->_sock ) ;

   if ( rn->_pSendBuffer )
   {
      SDB_OSS_FREE ( rn->_pSendBuffer ) ;
   }
   if ( rn->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, rn->_pReceiveBuffer, rn->_receiveBufferSize ) ;
      SDB_OSS_FREE ( rn->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbRNStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseDomain ( sdbDomainHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbDomainStruct *s = (sdbDomainStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DOMAIN ) ;
   _unregSocket( s->_connection, &s->_sock ) ;

   if ( s->_pSendBuffer )
   {
      SDB_OSS_FREE ( s->_pSendBuffer ) ;
   }
   if ( s->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, s->_pReceiveBuffer, s->_receiveBufferSize ) ;
      SDB_OSS_FREE ( s->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbDomainStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseCursor ( sdbCursorHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbCursorStruct *cs = (sdbCursorStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_CURSOR ) ;
   sdbCloseCursor( cHandle ) ;

   if ( !cs->_isClosed )
   {
      // unregister
      _unregCursor ( cs->_connection, cHandle ) ;
   }
   if ( cs->_pSendBuffer )
   {
      SDB_OSS_FREE ( cs->_pSendBuffer ) ;
   }
   if ( cs->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, cs->_pReceiveBuffer, cs->_receiveBufferSize ) ;
      SDB_OSS_FREE ( cs->_pReceiveBuffer ) ;
   }
   /*
   if ( cs->_modifiedCurrent )
   {
      bson_destroy ( cs->_modifiedCurrent ) ;
      SDB_OSS_FREE ( cs->_modifiedCurrent ) ;
      cs->_modifiedCurrent = NULL ;
   }
   */
   /*
   if( cs->_isDeleteCurrent )
   {
     cs->_isDeleteCurrent = FALSE ;
   }
   */
   SDB_OSS_FREE ( (sdbCursorStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

SDB_EXPORT void sdbReleaseDC ( sdbDCHandle cHandle )
{
   INT32 rc = SDB_OK ;
   sdbDCStruct *s = (sdbDCStruct*)cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DC ) ;
   _unregSocket( s->_connection, &s->_sock ) ;

   if ( s->_pSendBuffer )
   {
      SDB_OSS_FREE ( s->_pSendBuffer ) ;
   }
   if ( s->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, s->_pReceiveBuffer, s->_receiveBufferSize ) ;
      SDB_OSS_FREE ( s->_pReceiveBuffer ) ;
   }
   SDB_OSS_FREE ( (sdbDCStruct*)cHandle ) ;
done :
   return ;
error :
   goto done ;
}

/*
   Release lob handle
*/
static void sdbReleaseLob( sdbLobHandle cHandle )
{
   INT32 rc          = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * ) cHandle ;

   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( cHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   _unregSocket( lob->_connection, &lob->_sock ) ;

   if ( lob->_pSendBuffer )
   {
      SDB_OSS_FREE ( lob->_pSendBuffer ) ;
   }
   if ( lob->_pReceiveBuffer )
   {
      UNSET_ERROR_BUFF( cHandle, lob->_pReceiveBuffer,
                        lob->_receiveBufferSize ) ;
      SDB_OSS_FREE ( lob->_pReceiveBuffer ) ;
   }

   SDB_OSS_FREE( lob ) ;

done:
   return ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbAggregate ( sdbCollectionHandle cHandle,
                                bson **obj, SINT32 num,
                                sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   SINT32 count                    = 0 ;
   sdbCursorStruct *cursor         = NULL ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs = (sdbCollectionStruct *)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection              = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !obj || num <=0 || !handle || !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   for ( count = 0; count < num; ++count )
   {
      if ( !obj[count] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 == count )
         rc = clientBuildAggrRequest ( &cs->_pSendBuffer,
                                       &cs->_sendBufferSize,
                                       cs->_collectionFullName, obj[count],
                                       cs->_endianConvert ) ;
      else
         rc = clientAppendAggrRequest ( &cs->_pSendBuffer,
                                        &cs->_sendBufferSize,
                                        obj[count], cs->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer,
                  cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   ALLOC_HANDLE( cursor, sdbCursorStruct );
   INIT_CURSOR( cursor, cs->_connection, cs, contextID );
   // register cursor in connection
   rc = _regCursor ( cursor->_connection, (sdbCursorHandle)cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // set output result
   *handle = (sdbCursorHandle)cursor;

done :
   return rc ;
error :
   if ( cursor )
   {
      SDB_OSS_FREE ( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbAttachCollection ( sdbCollectionHandle cHandle,
                                       const CHAR *subClFullName,
                                       bson *options )
{
   INT32 rc                        = SDB_OK ;
   BOOLEAN bsoninit                = TRUE ;
   bson_iterator it ;
   bson newObj ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   BSON_INIT( newObj );
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !subClFullName || !options ||
        ossStrlen ( subClFullName) > CLIENT_COLLECTION_NAMESZ ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME, cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, FIELD_NAME_SUBCLNAME, subClFullName, string ) ;

   bson_iterator_init ( &it, options ) ;
   while ( BSON_EOO != ( bson_iterator_next ( &it ) ) )
   {
      BSON_APPEND ( newObj, NULL, &it, element ) ;
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_LINK_CL),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbDetachCollection( sdbCollectionHandle cHandle,
                                      const CHAR *subClFullName)
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct* connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !subClFullName ||
        ossStrlen ( subClFullName) > CLIENT_COLLECTION_NAMESZ ||
        !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME, cs->_collectionFullName, string ) ;
   BSON_APPEND( newObj, FIELD_NAME_SUBCLNAME, subClFullName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbBackupOffline ( sdbConnectionHandle cHandle, bson *options )
{
   return sdbBackup( cHandle, options ) ;
}

SDB_EXPORT INT32 sdbBackup ( sdbConnectionHandle cHandle, bson *options )
{
   INT32 rc                      = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit              = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // options is optional
   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND( newObj, NULL, &it, element ) ;
      }
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_BACKUP_OFFLINE),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbListBackup ( sdbConnectionHandle cHandle,
                                 bson *options,
                                 bson *condition,
                                 bson *selector,
                                 bson *orderBy,
                                 sdbCursorHandle *handle )
{
   INT32 rc                      = SDB_OK ;
   sdbCursorHandle cursor        = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit              = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !handle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // options is optional
   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND( newObj, NULL, &it, element ) ;
      }
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_LIST_BACKUPS),
                      0, 0, 0, -1,
                      condition, selector, orderBy, &newObj,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   *handle = cursor ;

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbRemoveBackup ( sdbConnectionHandle cHandle,
                                   bson* options )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // options is optional
   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND( newObj, NULL, &it, element ) ;
      }
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_REMOVE_BACKUP),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbListTasks ( sdbConnectionHandle cHandle,
                                bson *condition,
                                bson *selector,
                                bson *orderBy,
                                bson *hint,
                                sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_TASKS, condition,
                       selector, orderBy, handle ) ;
}

SDB_EXPORT INT32 sdbWaitTasks ( sdbConnectionHandle cHandle,
                                const SINT64 *taskIDs,
                                SINT32 num )
{
   INT32 rc                      = SDB_OK ;
   SINT32 i                      = 0 ;
   INT32 pos                     = 0 ;
   CHAR pos_buf[128]             = { 0 } ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*) cHandle ;
   BOOLEAN bsoninit              = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // check argument
   if ( !taskIDs || num < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // append argument
   rc = bson_append_start_object ( &newObj, FIELD_NAME_TASKID ) ;
   if ( rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_start_array ( &newObj, "$in" ) ;
   if ( rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   for ( i = 0; i < num; i++ )
   {
      ossSnprintf ( pos_buf, sizeof(pos_buf), "%d", pos++ ) ;
      BSON_APPEND( newObj, pos_buf, taskIDs[i], long ) ;
   }
   rc = bson_append_finish_array ( &newObj ) ;
   if ( rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = bson_append_finish_object ( &newObj ) ;
   if ( rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_WAITTASK),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbCancelTask ( sdbConnectionHandle cHandle,
                                 SINT64 taskID,
                                 BOOLEAN isAsync )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*) cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // check argument
   if ( taskID <= 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND ( newObj, FIELD_NAME_TASKID, taskID, long ) ;
   BSON_APPEND ( newObj, FIELD_NAME_ASYNC, isAsync, bool ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT void sdbSetConnectionInterruptFunc(
                                          sdbConnectionHandle cHandle,
                                          socketInterruptFunc func )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*) cHandle ;

   SDB_UNUSED( rc ) ;
   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   clientSetInterruptFunc( connection->_sock, func ) ;

done :
   return ;
error :
   goto done ;

}

SDB_EXPORT INT32 sdbSetSessionAttr ( sdbConnectionHandle cHandle,
                                     bson *options )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*) cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson_iterator it ;
   bson newObj ;

   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   // check argument
   if ( !options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( bson_is_empty( options ) )
   {
      _sdbClearSessionAttrCache( connection, TRUE ) ;
      goto done ;
   }

   BSON_INIT( newObj ) ;

   bson_iterator_init ( &it, options ) ;
   while ( BSON_EOO != bson_iterator_next( &it ) )
   {
      // get key
      const CHAR * key = bson_iterator_key( &it ) ;

      if ( 0 == ossStrcasecmp( FIELD_NAME_PREFERRED_INSTANCE, key ) ||
           0 == ossStrcasecmp( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, key ) )
      {
         switch ( bson_iterator_type( &it ) )
         {
            case BSON_STRING :
            {
               INT32 value = PREFER_REPL_TYPE_MAX ;
               const CHAR * str_value = bson_iterator_string( &it ) ;
               if ( 0 == ossStrcasecmp( "M", str_value ) ||
                    0 == ossStrcasecmp( "-M", str_value ) )
               {
                  // master
                  value = PREFER_REPL_MASTER ;
               }
               else if ( 0 == ossStrcasecmp( "S", str_value ) ||
                         0 == ossStrcasecmp( "-S", str_value ) )
               {
                  // slave
                  value = PREFER_REPL_SLAVE ;
               }
               else if ( 0 == ossStrcasecmp( "A", str_value ) ||
                         0 == ossStrcasecmp( "-A", str_value ) )
               {
                  // anyone
                  value = PREFER_REPL_ANYONE ;
               }
               else
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               rc = bson_append_int( &newObj, key, value ) ;
               if ( BSON_OK != rc )
               {
                  rc = SDB_DRIVER_BSON_ERROR ;
                  goto error ;
               }
               break ;
            }
            case BSON_INT :
            {
               rc = bson_append_element( &newObj, NULL, &it ) ;
               if ( BSON_OK != rc )
               {
                  rc = SDB_DRIVER_BSON_ERROR ;
                  goto error ;
               }
               break ;
            }
            default :
            {
               break ;
            }
         }

         rc = bson_append_element( &newObj, FIELD_NAME_PREFERED_INSTANCE_V1, &it ) ;
         if ( BSON_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else
      {
         rc = bson_append_element( &newObj, NULL, &it ) ;
         if ( BSON_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
   }

   BSON_FINISH ( newObj ) ;

   _sdbClearSessionAttrCache( connection, TRUE ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR),
                      0, 0, 0, -1,
                      &newObj, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetSessionAttrEx ( sdbConnectionHandle cHandle,
                                       BOOLEAN useCache,
                                       bson * result )
{
   INT32 rc = SDB_OK ;
   BOOLEAN gotHandle = FALSE ;
   BOOLEAN gotAttribute = FALSE ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbConnectionStruct * connection = (sdbConnectionStruct *)cHandle ;

   if ( NULL == result  )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   gotHandle = TRUE ;

   if ( useCache && _sdbGetSessionAttrCache( connection, result ) )
   {
      gotAttribute = TRUE ;
      goto done ;
   }

   // run command
   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GETSESS_ATTR,
                      0, 0, 0, -1,
                      NULL, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( SDB_INVALID_HANDLE == cursor )
   {
      // Cursor is empty
      _sdbClearSessionAttrCache( connection, TRUE ) ;
      rc = SDB_OK ;
   }
   else
   {
      rc = sdbNext( cursor, result ) ;
      if ( SDB_OK == rc )
      {
         _sdbSetSessionAttrCache( connection, result ) ;
         gotAttribute = TRUE ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         _sdbClearSessionAttrCache( connection, TRUE ) ;
         rc = SDB_OK ;
      }
      else
      {
         goto error ;
      }
   }

   if ( !gotAttribute )
   {
      bson emptyResult ;
      bson_init( &emptyResult ) ;
      if ( BSON_OK != bson_finish( &emptyResult ) )
      {
         bson_destroy( &emptyResult ) ;
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      if ( BSON_OK != bson_copy( result, &emptyResult ) )
      {
         bson_destroy( &emptyResult ) ;
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      bson_destroy( &emptyResult ) ;
   }

done :
   if ( SDB_INVALID_HANDLE != (sdbCursorHandle)cursor )
   {
      sdbReleaseCursor( (sdbCursorHandle)cursor ) ;
   }
   return rc ;

error :
   if ( gotHandle )
   {
      _sdbClearSessionAttrCache( connection, TRUE ) ;
   }
   goto done ;
}

SDB_EXPORT INT32 sdbGetSessionAttr ( sdbConnectionHandle cHandle,
                                     bson * result )
{
   return sdbGetSessionAttrEx( cHandle, TRUE, result ) ;
}

SDB_EXPORT INT32 _sdbMsg ( sdbConnectionHandle cHandle, const CHAR *msg )
{
   INT32 rc              = SDB_OK ;
   SINT64 contextID      = 0 ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*) cHandle ;

   // check handle
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // check argument
   if ( !msg )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   rc = clientBuildTestMsg ( &connection->_pSendBuffer,
                             &connection->_sendBufferSize,
                             msg, 0, connection->_endianConvert) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // send and recv
   rc = _sendAndRecv( cHandle, connection->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cHandle,
                  (MsgHeader*)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check the return header
   CHECK_RET_MSGHEADER( connection->_pSendBuffer, connection->_pReceiveBuffer,
                        cHandle ) ;
done :
   return rc ;
error :
   goto done ;
}

#define INIT_DOMAINHANDLE( dhandle, cHandle, pDoaminName )                  \
do                                                                          \
{                                                                           \
   dhandle->_handleType    = SDB_HANDLE_TYPE_DOMAIN ;                       \
   dhandle->_connection    = ( sdbConnectionHandle )cHandle ;               \
   dhandle->_sock          = cHandle->_sock ;                               \
   dhandle->_endianConvert = cHandle->_endianConvert ;                      \
   ossStrncpy ( dhandle->_domainName, pDomainName, CLIENT_DOMAIN_NAMESZ ) ; \
}while(0)

SDB_EXPORT INT32 sdbCreateDomain ( sdbConnectionHandle cHandle,
                                   const CHAR *pDomainName,
                                   bson *options,
                                   sdbDomainHandle *handle )
{
   INT32 rc                     = SDB_OK ;
   CHAR *pCreateDomain          = CMD_ADMIN_PREFIX CMD_NAME_CREATE_DOMAIN ;
   sdbDomainStruct *s           = NULL ;
   CHAR *pName                  = FIELD_NAME_NAME ;
   CHAR *pOptions               = FIELD_NAME_OPTIONS ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit             = FALSE ;
   bson newObj ;

   BSON_INIT( newObj );
   // sanity check for connection
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   // sanity check for input variables
   // not options are optional and can be NULL
   if ( !pDomainName || !handle ||
        ossStrlen ( pDomainName ) > CLIENT_DOMAIN_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pDomainName, string ) ;
   if ( options )
   {
      BSON_APPEND( newObj, pOptions, options, bson ) ;
   }
   BSON_FINISH ( newObj ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCreateDomain, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   ALLOC_HANDLE( s, sdbDomainStruct ) ;
   INIT_DOMAINHANDLE( s, connection, pDomainName ) ;
   _regSocket( cHandle, &s->_sock ) ;

   *handle = (sdbDomainHandle)s ;
done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbDropDomain ( sdbConnectionHandle cHandle,
                                 const CHAR *pDomainName )
{
   INT32 rc               = SDB_OK ;
   CHAR *pDropDomain      = CMD_ADMIN_PREFIX CMD_NAME_DROP_DOMAIN ;
   CHAR *pName            = FIELD_NAME_NAME ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit       = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pDomainName ||
        ossStrlen ( pDomainName) > CLIENT_DOMAIN_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pDomainName, string );
   BSON_FINISH ( newObj ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pDropDomain, &newObj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetDomain ( sdbConnectionHandle cHandle,
                                const CHAR *pDomainName,
                                sdbDomainHandle *handle )
{
   INT32 rc                 = SDB_OK ;
   sdbCursorHandle cursor   = SDB_INVALID_HANDLE ;
   CHAR *pName              = FIELD_NAME_NAME ;
   sdbDomainStruct *s       = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit         = FALSE ;
   bson newObj ;
   bson result ;

   BSON_INIT( newObj ) ;
   BSON_INIT( result ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pDomainName || !handle ||
        ossStrlen ( pDomainName ) > CLIENT_DOMAIN_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, pName, pDomainName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = sdbGetList ( cHandle, SDB_LIST_DOMAINS, &newObj, NULL, NULL,
                     &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( SDB_OK == ( rc = sdbNext ( cursor, &result ) ) )
   {
      ALLOC_HANDLE( s, sdbDomainStruct ) ;
      INIT_DOMAINHANDLE( s, connection, pDomainName ) ;
      _regSocket( cHandle, &s->_sock ) ;
      *handle = (sdbDomainHandle) s ;
   }
   else if ( SDB_DMS_EOC != rc )
   {
      goto error ;
   }
   else
   {
      rc = SDB_CAT_DOMAIN_NOT_EXIST ;
      goto error ;
   }
done :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }

   BSON_DESTROY( newObj ) ;
   BSON_DESTROY( result ) ;
   return rc ;
error :
   if ( s )
   {
      SDB_OSS_FREE( s );
   }

   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbListDomains ( sdbConnectionHandle cHandle,
                                  bson *condition,
                                  bson *selector,
                                  bson *orderBy,
                                  sdbCursorHandle *handle )
{
   return sdbGetList ( cHandle, SDB_LIST_DOMAINS, condition,
                       selector, orderBy, handle ) ;
}

static INT32 _sdbAlterDomainV1 ( sdbCollectionHandle cHandle,
                                 const bson * options  )
{
   INT32 rc                = SDB_OK ;
   const CHAR * command    = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN ;
   sdbDomainStruct * domain = ( sdbDomainStruct * )cHandle ;
   BOOLEAN bsoninit        = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, domain, SDB_HANDLE_TYPE_DOMAIN ) ;
   if ( !options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME, domain->_domainName, string ) ;
   BSON_APPEND( newObj, FIELD_NAME_OPTIONS, options, bson) ;
   BSON_FINISH( newObj ) ;

   rc = _runCommand( domain->_connection, domain->_sock,
                     &( domain->_pSendBuffer ),
                     &( domain->_sendBufferSize ),
                     &( domain->_pReceiveBuffer ),
                     &( domain->_receiveBufferSize ),
                     domain->_endianConvert,
                     command, &newObj,
                     NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   goto done ;
}

static INT32 _sdbAlterDomainV2 ( sdbCollectionHandle cHandle,
                                 const bson * options )
{
   INT32 rc = SDB_OK ;

   const CHAR * command = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN ;
   sdbDomainStruct * domain = ( sdbDomainStruct * )cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson_iterator itr ;
   bson newObj ;
   bson_type alterType = BSON_EOO ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, domain, SDB_HANDLE_TYPE_DOMAIN ) ;
   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_ALTER_TYPE, SDB_CATALOG_DOMAIN, string ) ;
   BSON_APPEND( newObj, FIELD_NAME_VERSION, SDB_ALTER_VERSION, int ) ;
   BSON_APPEND( newObj, FIELD_NAME_NAME, domain->_domainName, string ) ;

   alterType = bson_find( &itr, options, FIELD_NAME_ALTER ) ;
   if ( BSON_OBJECT == alterType || BSON_ARRAY == alterType )
   {
      rc = bson_append_element( &newObj, NULL, &itr ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else
   {
       rc = SDB_INVALIDARG ;
       goto error ;
   }

   /// optional
   if ( BSON_OBJECT == bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
   {
      rc = bson_append_element( &newObj, NULL, &itr ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else if ( BSON_EOO != bson_find( &itr, options, FIELD_NAME_OPTIONS ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_FINISH( newObj ) ;

   rc = _runCommand( domain->_connection, domain->_sock,
                     &( domain->_pSendBuffer ),
                     &( domain->_sendBufferSize ),
                     &( domain->_pReceiveBuffer ),
                     &( domain->_receiveBufferSize ),
                     domain->_endianConvert,
                     command, &newObj,
                     NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbAlterDomain( sdbDomainHandle cHandle,
                                 const bson *options )
{
   INT32 rc = SDB_OK ;
   bson_iterator i ;

   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( BSON_EOO == bson_find( &i, options, FIELD_NAME_ALTER ) )
   {
      rc = _sdbAlterDomainV1( cHandle, options ) ;
   }
   else
   {
      rc = _sdbAlterDomainV2( cHandle, options ) ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

static INT32 _sdbAlterDomainInternal ( sdbDomainHandle cHandle,
                                       const CHAR * taskName,
                                       const bson * options,
                                       BOOLEAN allowNullArgs )
{
   INT32 rc = SDB_OK ;

   sdbDomainStruct *domain = ( sdbDomainStruct * )cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;

   HANDLE_CHECK( cHandle, domain, SDB_HANDLE_TYPE_DOMAIN ) ;

   if ( NULL == taskName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _sdbBuildAlterObject( taskName, options, allowNullArgs, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbAlterDomain( cHandle, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDomainAddGroups ( sdbDomainHandle cHandle,
                                      const bson * options )
{
   return _sdbAlterDomainInternal( cHandle, SDB_ALTER_DOMAIN_ADD_GROUPS, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbDomainRemoveGroups ( sdbDomainHandle cHandle,
                                         const bson * options )
{
   return _sdbAlterDomainInternal( cHandle, SDB_ALTER_DOMAIN_REMOVE_GROUPS, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbDomainSetGroups ( sdbDomainHandle cHandle,
                                      const bson * options )
{
   return _sdbAlterDomainInternal( cHandle, SDB_ALTER_DOMAIN_SET_GROUPS, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbDomainSetAttributes ( sdbDomainHandle cHandle,
                                          const bson * options )
{
   return _sdbAlterDomainInternal( cHandle, SDB_ALTER_DOMAIN_SET_ATTR, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbListCollectionSpacesInDomain( sdbDomainHandle cHandle,
                                                  sdbCursorHandle *cursor )
{
   INT32 rc         = SDB_OK ;
   sdbDomainStruct *s = ( sdbDomainStruct * )cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson condition ;
   bson selector ;

   BSON_INIT( condition ) ;
   BSON_INIT( selector ) ;
   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DOMAIN ) ;
   if ( !cursor )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( condition, FIELD_NAME_DOMAIN, s->_domainName, string ) ;
   BSON_FINISH( condition ) ;
   BSON_APPEND_NULL( selector, FIELD_NAME_NAME ) ;
   BSON_FINISH( selector ) ;

   rc = sdbGetList ( s->_connection, SDB_LIST_CS_IN_DOMAIN, &condition,
                     &selector, NULL, cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error;
   }
done:
   BSON_DESTROY( condition );
   BSON_DESTROY( selector );
   return rc ;
error:
   SET_INVALID_HANDLE( cursor ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbListCollectionsInDomain( sdbDomainHandle cHandle,
                                             sdbCursorHandle *cursor )
{
   INT32 rc           = SDB_OK ;
   sdbDomainStruct *s = ( sdbDomainStruct * )cHandle ;
   BOOLEAN bsoninit   = FALSE ;
   bson condition ;
   bson selector ;

   BSON_INIT( condition ) ;
   BSON_INIT( selector ) ;
   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DOMAIN ) ;
   if ( !cursor )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( condition, FIELD_NAME_DOMAIN, s->_domainName, string) ;
   BSON_FINISH( condition ) ;

   BSON_APPEND_NULL( selector, FIELD_NAME_NAME ) ;
   BSON_FINISH( selector ) ;

   rc = sdbGetList ( s->_connection, SDB_LIST_CL_IN_DOMAIN, &condition,
                     &selector, NULL, cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( condition );
   BSON_DESTROY( selector );
   return rc ;
error:
   SET_INVALID_HANDLE( cursor ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbListGroupsInDomain( sdbDomainHandle cHandle,
                                        sdbCursorHandle *cursor )
{
   INT32 rc = SDB_OK ;
   sdbDomainStruct *s = ( sdbDomainStruct * )cHandle ;
   sdbConnectionHandle conn = SDB_INVALID_HANDLE ;
   BOOLEAN bsoninit = FALSE ;
   bson condition ;

   BSON_INIT( condition ) ;
   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DOMAIN ) ;
   if ( !cursor )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( condition, FIELD_NAME_NAME, s->_domainName, string) ;
   BSON_FINISH( condition ) ;

   conn = s->_connection ;
   rc = sdbListDomains( conn, &condition, NULL, NULL, cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( condition ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbInvalidateCache( sdbConnectionHandle cHandle,
                                     bson *condition )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      (CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE),
                      condition, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbForceSession( sdbConnectionHandle cHandle,
                                  SINT64 sessionID,
                                  bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson query ;

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   BSON_APPEND( query, FIELD_NAME_SESSIONID, sessionID, long ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_SESSIONID ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;
   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      (CMD_ADMIN_PREFIX CMD_NAME_FORCE_SESSION),
                      &query, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( query ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbCreateLobID( sdbCollectionHandle cHandle,
                                 bson_oid_t *oid )
{
   return sdbCreateLobID1(cHandle, NULL, oid);
}

SDB_EXPORT INT32 sdbCreateLobID1( sdbCollectionHandle cHandle,
                                  const CHAR *pTimeStamp,
                                  bson_oid_t *oid )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   const MsgOpReply *reply         = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   bson dataInfo ;
   bson oidObj ;

   bson_init( &dataInfo ) ;
   bson_init( &oidObj ) ;

   // check arguments
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // prepare meta info
   if ( NULL != pTimeStamp )
   {
      rc = bson_append_string( &dataInfo, FIELD_NAME_LOB_CREATETIME, pTimeStamp ) ;
      if ( SDB_OK != rc)
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error;
      }
   }
   rc = bson_finish( &dataInfo );
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   // build\send\recv msg
   rc = clientBuildCreateLobIDMsg( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                   &dataInfo, 0, 1, 0, cs->_endianConvert ) ;

   if ( SDB_OK != rc)
   {
      goto error;
   }
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                (MsgHeader*)cs->_pSendBuffer,
                (MsgHeader**)&cs->_pReceiveBuffer,
                &cs->_receiveBufferSize,
                TRUE, cs->_endianConvert);
   if ( SDB_OK != rc)
   {
      goto error;
   }
   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer,
                  cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc)
   {
      goto error;
   }
   // check the return header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   // get oid
   reply = ( const MsgOpReply * )( cs->_pReceiveBuffer ) ;
   if ( reply->numReturned > 0 &&
        (UINT32)reply->header.messageLength >
        ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
   {
      bson_iterator bsonItr ;
      bson_type bType = BSON_EOO ;
      const CHAR *body = cs->_pReceiveBuffer + sizeof( MsgOpReply ) ;
      bson_init_finished_data( &oidObj, body ) ;
      bType = bson_find( &bsonItr, &oidObj, FIELD_NAME_LOB_OID ) ;
      if ( BSON_OID == bType )
      {
         *oid =  *(bson_iterator_oid( &bsonItr )) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   done:
      bson_destroy( &dataInfo ) ;
      bson_destroy( &oidObj ) ;
      return rc ;
   error:
      goto done ;
}

SDB_EXPORT INT32 sdbGetLobId( sdbLobHandle lobHandle,
                              bson_oid_t *oid )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   ossMemcpy( oid, lob->_oid, 12 );
   done:
      return rc ;
   error:
      goto done ;
}

static INT32 _sdbCreateLob1( sdbCollectionHandle cHandle,
                             const bson_oid_t *oid,
                             sdbLobHandle* lobHandle,
                             BOOLEAN* isOldVersionLobServer)
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   sdbLobStruct *lobStruct         = NULL ;
   const CHAR *bsonBuf             = NULL ;
   bson_type bType                 = BSON_EOO ;
   bson obj ;
   bson_iterator bsonItr ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &obj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   rc = bson_append_string( &obj, FIELD_NAME_COLLECTION, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   if ( NULL != oid )
   {
      rc = bson_append_oid( &obj, FIELD_NAME_LOB_OID, oid ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   rc = bson_append_int( &obj, FIELD_NAME_LOB_OPEN_MODE, SDB_LOB_CREATEONLY ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   rc = clientBuildOpenLobMsg( &cs->_pSendBuffer, &cs->_sendBufferSize,
                               &obj, 0,
                               1, 0, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                     (MsgHeader*)cs->_pReceiveBuffer,
                     cs->_receiveBufferSize,
                     &contextID, cs->_endianConvert ) ;
   if ( SDB_INVALIDARG == rc && NULL == oid )
   {
      if ( NULL != isOldVersionLobServer )
      {
         *isOldVersionLobServer = TRUE ;
      }
   }
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // check the return header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   bsonBuf = cs->_pReceiveBuffer + sizeof( MsgOpReply ) ;
   if ( BSON_OK != bson_init_finished_data( &obj, bsonBuf ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   ALLOC_HANDLE( lobStruct, sdbLobStruct ) ;
   LOB_INIT( lobStruct, cs->_connection, cs ) ;
   lobStruct->_contextID = contextID ;
   lobStruct->_mode = SDB_LOB_CREATEONLY ;

   _regSocket( cs->_connection, &lobStruct->_sock ) ;

   // get meta info from the return object
   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_SIZE ) ;
   if ( BSON_INT == bType || BSON_LONG == bType )
   {
      lobStruct->_lobSize = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_CREATETIME ) ;
   if ( BSON_LONG == bType )
   {
      lobStruct->_createTime = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_MODIFICATION_TIME ) ;
   if ( BSON_LONG == bType )
   {
      lobStruct->_modificationTime = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      lobStruct->_modificationTime = lobStruct->_createTime ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_PAGE_SIZE ) ;
   if ( BSON_INT == bType )
   {
      lobStruct->_pageSize =  bson_iterator_int( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( NULL != oid )
   {
      ossMemcpy( lobStruct->_oid, oid, 12 );
   }
   else
   {
      bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_OID) ;
      if ( BSON_OID == bType )
      {
         ossMemcpy( lobStruct->_oid, bson_iterator_oid( &bsonItr ), 12 );
      }
   }

   *lobHandle = (sdbLobHandle)lobStruct ;

   done:
      bson_destroy( &obj ) ;
      return rc ;
   error:
      if ( NULL != lobStruct )
      {
         sdbReleaseLob( (sdbLobHandle)lobStruct ) ;
      }
      *lobHandle = SDB_INVALID_HANDLE ;
      goto done ;
}

static INT32 _sdbCreateLob( sdbCollectionHandle cHandle,
                            const bson_oid_t *oid,
                            sdbLobHandle* lobHandle )
{
   INT32 rc                        = SDB_OK ;
   bson_oid_t oidObj ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || NULL == lobHandle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !connection->_isOldVersionLobServer )
   {
      BOOLEAN isOldVersionLobServer = FALSE ;
      rc = _sdbCreateLob1( cHandle, oid, lobHandle, &isOldVersionLobServer) ;
      if ( isOldVersionLobServer )
      {
         // deal with old version server. oid should be generate in client side
         bson_oid_gen ( &oidObj ) ;
         rc = _sdbCreateLob1( cHandle, &oidObj, lobHandle, NULL) ;
         if ( SDB_OK == rc )
         {
            connection->_isOldVersionLobServer = TRUE ;
         }
      }
   }
   else
   {
      // deal with old version server. oid should be generate in client side
      if ( NULL == oid )
      {
         bson_oid_gen ( &oidObj ) ;
      }
      else
      {
         oidObj = *oid ;
      }
      rc = _sdbCreateLob1( cHandle, &oidObj, lobHandle, NULL) ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }

   done:
      return rc ;
   error:
      goto done ;
}

SDB_EXPORT INT32 sdbOpenLob( sdbCollectionHandle cHandle,
                             const bson_oid_t *oid,
                             INT32 mode,
                             sdbLobHandle* lobHandle )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   SINT32 flags                    = 0;
   sdbLobStruct *lobStruct         = NULL ;
   const CHAR *bsonBuf             = NULL ;
   bson_type bType                 = BSON_EOO ;
   bson obj ;
   bson_iterator bsonItr ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &obj ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || NULL == lobHandle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( SDB_LOB_CREATEONLY != mode &&
        !isLobReadOnlyMode( mode ) &&
        !hasLobWriteMode( mode ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // use _sdbCreateLob to create Lob.
   if (SDB_LOB_CREATEONLY == mode)
   {
      rc = _sdbCreateLob(cHandle, oid, lobHandle) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      else
      {
         goto done ;
      }
   }
   // mode not equal SDB_LOB_CREATEONLY, the oid can't be NULL
   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_string( &obj, FIELD_NAME_COLLECTION, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_oid( &obj, FIELD_NAME_LOB_OID, oid ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_int( &obj, FIELD_NAME_LOB_OPEN_MODE, mode ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   // when we open lob for reading, set the flag with returning data
   if ( SDB_LOB_READ & mode )
   {
      flags |= FLG_LOBOPEN_WITH_RETURNDATA;
   }

   rc = clientBuildOpenLobMsg( &cs->_pSendBuffer, &cs->_sendBufferSize,
                               &obj, flags,
                               1, 0, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer,
                  cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check the return header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;
   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   bsonBuf = cs->_pReceiveBuffer + sizeof( MsgOpReply ) ;
   if ( BSON_OK != bson_init_finished_data( &obj, bsonBuf ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   ALLOC_HANDLE( lobStruct, sdbLobStruct ) ;
   LOB_INIT( lobStruct, cs->_connection, cs ) ;
   ossMemcpy( lobStruct->_oid, oid, 12 ) ;
   lobStruct->_contextID = contextID ;
   lobStruct->_mode = mode ;

   _regSocket( cs->_connection, &lobStruct->_sock ) ;

   // get meta info from the return object
   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_SIZE ) ;
   if ( BSON_INT == bType || BSON_LONG == bType )
   {
      lobStruct->_lobSize = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_CREATETIME ) ;
   if ( BSON_LONG == bType )
   {
      lobStruct->_createTime = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_MODIFICATION_TIME ) ;
   if ( BSON_LONG == bType )
   {
      lobStruct->_modificationTime = bson_iterator_long( &bsonItr ) ;
   }
   else
   {
      lobStruct->_modificationTime = lobStruct->_createTime ;
   }

   bType = bson_find( &bsonItr, &obj, FIELD_NAME_LOB_PAGE_SIZE ) ;
   if ( BSON_INT == bType )
   {
      lobStruct->_pageSize =  bson_iterator_int( &bsonItr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

   if ( SDB_LOB_READ == mode )
   {
      // when mode is SDB_LOB_READ, we add flag "FLG_LOBOPEN_WITH_RETURNDATA"
      // to the message send for engine, so, when open lob, the data will be
      // return at the same time, the return message format is as below:
      // "MsgOpReply  |  Meta Object  |  _MsgLobTuple   | Data"
      const MsgLobTuple *tuple = NULL ;
      const CHAR *body = NULL ;
      UINT32 retMsgLen =
         (UINT32)(((MsgHeader*)(cs->_pReceiveBuffer))->messageLength);
      UINT32 tupleOffset =
         ossRoundUpToMultipleX( sizeof( MsgOpReply ) + bson_size( &obj ), 4 ) ;
      if ( retMsgLen > tupleOffset )
      {
         // let lob's receive buffer pointer points to the cl's receive buffer
         lobStruct->_pReceiveBuffer = cs->_pReceiveBuffer ;
         cs->_pReceiveBuffer = NULL ;
         lobStruct->_receiveBufferSize = cs->_receiveBufferSize ;
         cs->_receiveBufferSize = 0 ;
         // initialize lob with the return data
         tuple = (MsgLobTuple *)(lobStruct->_pReceiveBuffer + tupleOffset) ;
         // "tuple->columns.offset" is the offset of lob content
         // in engine, at the very beginning, the offset must be 0,
         // for we have not read anything yet
         if ( 0 != tuple->columns.offset )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( retMsgLen <
                   ( tupleOffset + sizeof( MsgLobTuple ) + tuple->columns.len )
                 )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         body = (CHAR*)tuple + sizeof( MsgLobTuple ) ;
         lobStruct->_currentOffset = 0 ;
         lobStruct->_cachedOffset = lobStruct->_currentOffset ;
         // "tuple->columns.len" is the length of lob content return
         // by engine
         lobStruct->_cachedSize = tuple->columns.len ;
         lobStruct->_dataCache = body ;
      }
   }

   *lobHandle = (sdbLobHandle)lobStruct ;

done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   if ( NULL != lobStruct )
   {
      sdbReleaseLob( (sdbLobHandle)lobStruct ) ;
   }
   *lobHandle = SDB_INVALID_HANDLE ;
   goto done ;
}

SDB_EXPORT INT32 sdbWriteLob( sdbLobHandle lobHandle,
                              const CHAR *buf,
                              UINT32 len )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;
   SINT64 contextID = -1 ;
   UINT32 totalLen = 0 ;
   const UINT32 maxSendLen = 2 * 1024 * 1024 ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   if ( NULL == buf )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( SDB_LOB_CREATEONLY != lob->_mode &&
        !hasLobWriteMode( lob->_mode ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if (isLobReadWriteMode( lob->_mode ) )
   {
      //clean the read cache
      lob->_cachedOffset= -1 ;
      lob->_cachedSize= 0 ;
      lob->_dataCache= NULL ;
   }

   if ( 0 == len )
   {
      goto done ;
   }
   // build msg
   do
   {
      INT64 offset = lob->_currentOffset;
      UINT32 sendLen = maxSendLen <= len - totalLen ?
                       maxSendLen : len - totalLen ;
      rc = clientBuildWriteLobMsg( &(lob->_pSendBuffer), &lob->_sendBufferSize,
                                   buf + totalLen, sendLen, offset, 0, 1,
                                   lob->_contextID, 0,
                                   lob->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // send and recv
      rc = _sendAndRecv( lob->_connection, lob->_sock,
                         (MsgHeader*)lob->_pSendBuffer,
                         (MsgHeader**)&lob->_pReceiveBuffer,
                         &lob->_receiveBufferSize,
                         TRUE, lob->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // extract revc message
      rc = _extract( lob->_connection,
                     (MsgHeader*)lob->_pReceiveBuffer, lob->_receiveBufferSize,
                     &contextID, lob->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // check return msg header
      CHECK_RET_MSGHEADER( lob->_pSendBuffer, lob->_pReceiveBuffer,
                           lob->_connection ) ;
      totalLen += sendLen ;

      lob->_currentOffset += sendLen ;
      lob->_lobSize = OSS_MAX( lob->_lobSize, lob->_currentOffset ) ;
   } while ( totalLen < len ) ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbLockLob( sdbLobHandle lobHandle,
                             INT64 offset,
                             INT64 length )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;
   INT64 contextID = -1 ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;

   if ( offset < 0 || length < -1 || length == 0)
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !hasLobWriteMode( lob->_mode ) && SDB_LOB_SHAREREAD != lob->_mode )
   {
      goto done ;
   }

   rc = clientBuildLockLobMsg( &(lob->_pSendBuffer), &(lob->_sendBufferSize),
                               offset, length, 0, 1,
                               lob->_contextID, 0,
                               lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( lob->_connection, lob->_sock,
                      (MsgHeader*)lob->_pSendBuffer,
                      (MsgHeader**)&lob->_pReceiveBuffer,
                      &lob->_receiveBufferSize,
                      TRUE, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( lob->_connection,
                  (MsgHeader*)lob->_pReceiveBuffer, lob->_receiveBufferSize,
                  &contextID, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( lob->_pSendBuffer, lob->_pReceiveBuffer,
                        lob->_connection ) ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbLockAndSeekLob( sdbLobHandle lobHandle,
                                    INT64 offset,
                                    INT64 length )
{
   INT32 rc = SDB_OK ;

   rc = sdbLockLob( lobHandle, offset, length ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbSeekLob( lobHandle, offset, SDB_LOB_SEEK_SET ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

static BOOLEAN sdbDataCached( const sdbLobStruct *lob,
                              UINT32 len )
{
   // generally, "lob->_currentOffset" should alway be increased,
   // however,it may be changed by sdbSeekLob(),
   // so take care of it
   return ( NULL != lob->_dataCache && 0 < lob->_cachedSize &&
            0 <= lob->_cachedOffset &&
            lob->_cachedOffset <= lob->_currentOffset &&
            lob->_currentOffset < ( lob->_cachedOffset +
                                    lob->_cachedSize ) ) ;
}

static void sdbReadInCache( sdbLobStruct *lob,
                            void *buf,
                            UINT32 len,
                            UINT32 *read )
{
   const CHAR *cache = NULL ;
   // "lob->_cachedOffset + lob->_cachedSize" is "tuple->len"
   UINT32 readInCache = lob->_cachedOffset + lob->_cachedSize -
                        lob->_currentOffset ;
   readInCache = readInCache <= len ?
                 readInCache : len ;
   // when we can read data from cache,
   // "lob->_currentOffset >= lob->_cachedOffset" will be true,
   // if we never use "sdbSeekLob()" to adjust "lob->_currentOffset"
   // "lob->_currentOffset == lob->_cachedOffset", otherwise,
   // "lob->_currentOffset > lob->_cachedOffset"
   cache = lob->_dataCache +
           lob->_currentOffset -
           lob->_cachedOffset ;
   ossMemcpy( buf, cache, readInCache ) ;
   lob->_cachedSize -= readInCache + cache - lob->_dataCache ;

   if ( 0 == lob->_cachedSize )
   {
      lob->_dataCache = NULL ;
      lob->_cachedOffset = -1 ;
   }
   else
   {
      lob->_dataCache = cache + readInCache ;
      lob->_cachedOffset = readInCache + lob->_currentOffset ;
   }

   *read = readInCache ;
   return ;
}

static UINT32 sdbReviseReadLen( sdbLobStruct *lob,
                                UINT32 needLen )
{
   UINT32 pageSize = lob->_pageSize ;
   UINT32 mod = lob->_currentOffset & ( pageSize - 1 ) ;
   UINT32 alignedLen = ossRoundUpToMultipleX( needLen,
                                              LOB_ALIGNED_LEN ) ;
   alignedLen -= mod ;
   if ( alignedLen < LOB_ALIGNED_LEN )
   {
      alignedLen += LOB_ALIGNED_LEN ;
   }
   return alignedLen ;
}

static INT32 sdbOnceRead( sdbLobStruct *lob,
                          CHAR *buf,
                          UINT32 len,
                          UINT32 *read )
{
   INT32 rc = SDB_OK ;
   UINT32 needRead = len ;
   UINT32 totalRead = 0 ;
   CHAR *localBuf = buf ;
   UINT32 onceRead = 0 ;
   const MsgOpReply *reply = NULL ;
   const MsgLobTuple *tuple = NULL ;
   const CHAR *body = NULL ;
   UINT32 alignedLen = 0 ;
   SINT64 contextID = -1 ;

   if ( sdbDataCached( lob, needRead ) )
   {
      sdbReadInCache( lob, localBuf, needRead, &onceRead ) ;

      totalRead += onceRead ;
      needRead -= onceRead ;
      lob->_currentOffset += onceRead ;
      localBuf += onceRead ;
      *read = totalRead ;
      goto done ;
   }

   lob->_cachedOffset = -1 ;
   lob->_cachedSize = 0 ;
   lob->_dataCache = NULL ;

   alignedLen = sdbReviseReadLen( lob, needRead ) ;

   rc = clientBuildReadLobMsg( &(lob->_pSendBuffer), &lob->_sendBufferSize,
                               alignedLen, lob->_currentOffset,
                               0, lob->_contextID, 0,
                               lob->_endianConvert ) ;

   // send and recv
   rc = _sendAndRecv( lob->_connection, lob->_sock,
                      (MsgHeader*)lob->_pSendBuffer,
                      (MsgHeader**)&lob->_pReceiveBuffer,
                      &lob->_receiveBufferSize,
                      TRUE, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( lob->_connection,
                  (MsgHeader*)lob->_pReceiveBuffer, lob->_receiveBufferSize,
                  &contextID, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( lob->_pSendBuffer, lob->_pReceiveBuffer,
                        lob->_connection ) ;
   // when it is read res, the struct of the
   // return message is |MsgOpReply|_MsgLobTuple|data|
   reply = ( const MsgOpReply * )( lob->_pReceiveBuffer ) ;
   if ( ( UINT32 )( reply->header.messageLength ) <
        ( sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   tuple = ( const MsgLobTuple *)
           ( lob->_pReceiveBuffer + sizeof( MsgOpReply ) ) ;
   if ( lob->_currentOffset != tuple->columns.offset )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   else if ( ( UINT32 )( reply->header.messageLength ) <
             ( sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) +
             tuple->columns.len ) )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   body = lob->_pReceiveBuffer + sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) ;

   // if what we got is more than what we expect,
   // let cache some for next reading request.
   if ( needRead < tuple->columns.len )
   {
      ossMemcpy( localBuf, body, needRead ) ;
      totalRead += needRead ;
      lob->_currentOffset += needRead ;
      lob->_cachedOffset = lob->_currentOffset ;
      lob->_cachedSize = tuple->columns.len - needRead ;
      lob->_dataCache = body + needRead ;
   }
   else
   {
      // else, no need to cache anything
      ossMemcpy( localBuf, body, tuple->columns.len ) ;
      totalRead += tuple->columns.len ;
      lob->_currentOffset += tuple->columns.len ;
      lob->_cachedOffset = -1 ;
      lob->_cachedSize = 0 ;
      lob->_dataCache = NULL ;
   }

   *read = totalRead ;
done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbReadLob( sdbLobHandle lobHandle,
                             UINT32 len,
                             CHAR *buf,
                             UINT32 *read )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;
   UINT32 needRead = len ;
   CHAR *localBuf = buf ;
   UINT32 onceRead = 0 ;
   UINT32 totalRead = 0 ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   if (  NULL == buf )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( ( !isLobReadOnlyMode( lob->_mode ) && !isLobReadWriteMode( lob->_mode ) ) || -1 == lob->_contextID )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( 0 == len )
   {
      *read = 0 ;
      goto done ;
   }

   if ( lob->_currentOffset == lob->_lobSize )
   {
      rc = SDB_EOF ;
      goto error ;
   }

   while ( 0 < needRead && lob->_currentOffset < lob->_lobSize )
   {
      rc = sdbOnceRead( lob, localBuf, needRead, &onceRead ) ;
      if ( SDB_EOF == rc )
      {
         if ( 0 < totalRead )
         {
            rc = SDB_OK ;
            break ;
         }
         else
         {
            goto error ;
         }
      }
      else if ( SDB_OK != rc )
      {
         goto error ;
      }

      needRead -= onceRead ;
      totalRead += onceRead ;
      localBuf += onceRead ;
      onceRead = 0 ;
   }

   *read = totalRead ;
done:
   return rc ;
error:
   *read = 0 ;
   goto done ;
}


SDB_EXPORT INT32 sdbCloseLob( sdbLobHandle *lobHandle )
{
   INT32 rc          = SDB_OK ;
   sdbLobStruct *lob = NULL ;
   SINT64 contextID  = -1 ;

   if ( NULL == lobHandle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   lob = ( sdbLobStruct * )( *lobHandle );
   HANDLE_CHECK( *lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;

   if ( NULL == lob->_sock )
   {
      goto done ;
   }

   rc = clientBuildCloseLobMsg( &lob->_pSendBuffer, &lob->_sendBufferSize,
                                0, 1, lob->_contextID, 0,
                                lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( lob->_connection, lob->_sock,
                      (MsgHeader*)lob->_pSendBuffer,
                      (MsgHeader**)&lob->_pReceiveBuffer,
                      &lob->_receiveBufferSize,
                      TRUE, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( lob->_connection,
                  (MsgHeader*)lob->_pReceiveBuffer, lob->_receiveBufferSize,
                  &contextID, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( lob->_pSendBuffer, lob->_pReceiveBuffer,
                        lob->_connection ) ;
done:
   if ( NULL != lobHandle && SDB_INVALID_HANDLE != *lobHandle )
   {
      sdbReleaseLob( *lobHandle ) ;
      *lobHandle = SDB_INVALID_HANDLE ;
   }
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbRemoveLob( sdbCollectionHandle cHandle,
                               const bson_oid_t *oid )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   bson meta ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &meta ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;
   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !cs->_collectionFullName[0] || NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_string( &meta, FIELD_NAME_COLLECTION,
                            cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_oid( &meta, FIELD_NAME_LOB_OID, oid ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &meta ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = clientBuildRemoveLobMsg( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                 &meta, 0, 1, 0, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer, cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check the return header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &meta ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbTruncateLob( sdbCollectionHandle cHandle,
                                 const bson_oid_t *oid, INT64 length )
{
   INT32 rc                        = SDB_OK ;
   SINT64 contextID                = -1 ;
   bson meta ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &meta ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;
   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( length < 0 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !cs->_collectionFullName[0] || NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == oid )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_string( &meta, FIELD_NAME_COLLECTION,
                            cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_oid( &meta, FIELD_NAME_LOB_OID, oid ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_long( &meta, FIELD_NAME_LOB_LENGTH, length) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &meta ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = clientBuildTruncateLobMsg( &cs->_pSendBuffer, &cs->_sendBufferSize,
                                   &meta, 0, 1, 0, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( cs->_connection, cs->_sock,
                      (MsgHeader*)cs->_pSendBuffer,
                      (MsgHeader**)&cs->_pReceiveBuffer,
                      &cs->_receiveBufferSize,
                      TRUE, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( cs->_connection,
                  (MsgHeader*)cs->_pReceiveBuffer, cs->_receiveBufferSize,
                  &contextID, cs->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check the return header
   CHECK_RET_MSGHEADER( cs->_pSendBuffer, cs->_pReceiveBuffer,
                        cs->_connection ) ;

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &meta ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeekLob( sdbLobHandle lobHandle,
                             SINT64 size,
                             SDB_LOB_SEEK whence )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;

   if ( -1 == lob->_contextID )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !isLobReadOnlyMode( lob->_mode ) &&
        SDB_LOB_CREATEONLY != lob->_mode &&
        !hasLobWriteMode( lob->_mode ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( SDB_LOB_SEEK_SET == whence )
   {
      if ( size < 0 || ( lob->_lobSize < size && SDB_LOB_READ == lob->_mode ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      lob->_currentOffset = size ;
   }
   else if ( SDB_LOB_SEEK_CUR == whence )
   {
      if ( ( lob->_lobSize < size + lob->_currentOffset &&
             SDB_LOB_READ == lob->_mode ) ||
           size + lob->_currentOffset < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      lob->_currentOffset += size ;
   }
   else if ( SDB_LOB_SEEK_END == whence )
   {
      if ( size < 0 || ( lob->_lobSize < size && SDB_LOB_READ == lob->_mode ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      lob->_currentOffset = lob->_lobSize - size ;
   }
   else
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetLobSize( sdbLobHandle lobHandle,
                                SINT64 *size )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   if ( NULL == size )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   *size = lob->_lobSize ;
done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetLobCreateTime( sdbLobHandle lobHandle,
                                      UINT64 *millis )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;

   if ( NULL == millis )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *millis = lob->_createTime ;


done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetLobModificationTime( sdbLobHandle lobHandle,
                                      UINT64 *millis )
{
   INT32 rc = SDB_OK ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;

   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   if ( NULL == millis )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   *millis = lob->_modificationTime ;

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT BOOLEAN sdbLobIsEof( sdbLobHandle lobHandle )
{
   INT32 rc = SDB_OK ;
   BOOLEAN result = TRUE ;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;
   CLIENT_UNUSED( rc ) ;
   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;
   result = lob->_currentOffset >= lob->_lobSize ? TRUE : FALSE ;
done:
   return result ;
error:
   goto done ;
}

static INT32 _sdbRunCmdOfLob( sdbCollectionHandle cHandle,
                              const CHAR *cmd,
                              const bson *query,
                              const bson *selected,
                              const bson *orderBy,
                              const bson *hint,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              sdbCursorHandle *cursorHandle )
{
   INT32 rc                = SDB_OK ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      cmd,
                      0, 0, numToSkip, numToReturn,
                      query, selected, orderBy, hint,
                      cursorHandle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   SET_INVALID_HANDLE( cursorHandle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbListLobs( sdbCollectionHandle cHandle,
                              sdbCursorHandle *cursor)
{
   return sdbListLobs1(cHandle, NULL, NULL, NULL, NULL, 0, -1 ,cursor);
}

SDB_EXPORT INT32 sdbListLobs1( sdbCollectionHandle cHandle,
                               bson *condition,
                               bson *selected,
                               bson *orderBy,
                               bson *hint,
                               INT64 numToSkip,
                               INT64 numToReturn,
                               sdbCursorHandle *cursor)
{
   INT32 rc                        = SDB_OK ;
   bson newHint;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &newHint) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( NULL == cursor )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !bson_is_empty( hint ) )
   {
      rc = bson_append_elements( &newHint, hint ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   rc = bson_append_string( &newHint, FIELD_NAME_COLLECTION, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &newHint) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   if ( !connection->_isOldVersionLobServer )
   {
      rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                         condition,selected, orderBy, &newHint, numToSkip, numToReturn, cursor ) ;

       if ( SDB_INVALIDARG == rc )
       {
          // check params are correct
          if ( !bson_is_empty(condition) || !bson_is_empty(selected)
            || !bson_is_empty(orderBy) || !bson_is_empty(hint)
            || 0 != numToSkip || -1 != numToReturn)
          {
             // recheck remote server is old or not
             bson tmpHint ;
             sdbCursorHandle tmpCursor = 0 ;
             bson_init( &tmpHint ) ;
             bson_append_string( &tmpHint, FIELD_NAME_COLLECTION, cs->_collectionFullName ) ;
             rc = bson_finish( &tmpHint ) ;
             if ( SDB_OK != rc )
             {
                bson_destroy( &tmpHint ) ;
                rc = SDB_DRIVER_BSON_ERROR ;
                goto error ;
             }
             rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                         NULL,NULL, NULL, &tmpHint, 0, -1, &tmpCursor ) ;
             bson_destroy( &tmpHint ) ;
             if ( SDB_OK == rc )
             {
                //Now we are sure that remote server is new version.
                sdbCloseCursor( tmpCursor );
                sdbReleaseCursor( tmpCursor ) ;
                rc = SDB_INVALIDARG ;
                goto error ;
             }
          }
          if ( SDB_INVALIDARG == rc )
          {
             // deal with old version server. clName is in the query field
             rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                         &newHint, NULL,NULL, NULL, 0, -1, cursor ) ;
             if ( SDB_OK == rc )
             {
                // set new version
                connection->_isOldVersionLobServer = TRUE ;
             }
          }

       }
   }
   else
   {
       // deal with old version server. clName is in the query field
       rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                         &newHint,NULL, NULL, NULL, 0, -1, cursor ) ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &newHint ) ;
   return rc ;
error:
   SET_INVALID_HANDLE( cursor ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbListLobPieces( sdbCollectionHandle cHandle,
                                   sdbCursorHandle *cursor )
{
    return sdbListLobPieces1(cHandle, NULL, NULL, NULL, NULL, 0, -1, cursor);
}

SDB_EXPORT INT32 sdbListLobPieces1( sdbCollectionHandle cHandle,
                                    bson *condition,
                                    bson *selected,
                                    bson *orderBy,
                                    bson *hint,
                                    INT64 numToSkip,
                                    INT64 numToReturn,
                                    sdbCursorHandle *cursor )
{
   INT32 rc                        = SDB_OK ;
   bson newHint ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;

   bson_init( &newHint ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection                      = (sdbConnectionStruct*)(cs->_connection) ;

   if ( NULL == cursor )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( !bson_is_empty( hint ) )
   {
      rc = bson_append_elements( &newHint, hint ) ;
      if ( SDB_OK != rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }

   rc = bson_append_string( &newHint, FIELD_NAME_COLLECTION, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_bool( &newHint, FIELD_NAME_LOB_LIST_PIECES_MODE, TRUE ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_finish( &newHint) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   if (!connection->_isOldVersionLobServer)
   {
      rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                      condition, selected, orderBy, &newHint, numToSkip, numToReturn, cursor ) ;
      if ( SDB_INVALIDARG == rc )
      {
         // deal with old version server. clName is in the query field
         rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                    &newHint, NULL, NULL, NULL, 0, -1, cursor ) ;
         if ( SDB_OK == rc )
         {
            // set old version
            connection->_isOldVersionLobServer = TRUE ;
         }
       }
   }
   else
   {
       // deal with old version server. clName is in the query field
       rc = _sdbRunCmdOfLob( cHandle, CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
       &newHint, NULL, NULL, NULL, 0, -1, cursor ) ;
   }

   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &newHint) ;
   return rc ;
error:
   SET_INVALID_HANDLE( cursor ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetRunTimeDetail( sdbLobHandle lobHandle,
	                                  bson *detail)
{
   INT32 rc = SDB_OK;
   sdbLobStruct *lob = ( sdbLobStruct * )lobHandle ;
   SINT64 contextID	= -1;
   bson resultObj;
   HANDLE_CHECK( lobHandle, lob, SDB_HANDLE_TYPE_LOB ) ;

   if ( -1 == lob->_contextID )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // build msg
   rc = clientBuildGetLobRTimeMsg(  &(lob->_pSendBuffer), &lob->_sendBufferSize,
                                     0, 1, lob->_contextID, 0,
                                     lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // send and recv
   rc = _sendAndRecv( lob->_connection, lob->_sock,
                      (MsgHeader*)lob->_pSendBuffer,
                      (MsgHeader**)&lob->_pReceiveBuffer,
                      &lob->_receiveBufferSize,
                      TRUE, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // extract revc message
   rc = _extract( lob->_connection,
                  (MsgHeader*)lob->_pReceiveBuffer, lob->_receiveBufferSize,
                  &contextID, lob->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return msg header
   CHECK_RET_MSGHEADER( lob->_pSendBuffer, lob->_pReceiveBuffer,
                        lob->_connection ) ;

   bson_init( &resultObj ) ;
   rc = bson_init_finished_data( &resultObj , lob->_pReceiveBuffer + sizeof( MsgOpReply ) );
   if ( SDB_OK != rc )
   {
      rc = SDB_CORRUPTED_RECORD ;
      goto done ;
   }
   rc = bson_copy ( detail, &resultObj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto done ;
   }

done:
   bson_destroy( &resultObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbReelect( sdbReplicaGroupHandle cHandle,
                             const bson *options )
{
   INT32 rc         = SDB_OK ;
   sdbRGStruct *rg  = (sdbRGStruct*)cHandle ;
   bson ops ;
   bson_iterator itr ;
   bson_init( &ops ) ;
   HANDLE_CHECK( cHandle, rg, SDB_HANDLE_TYPE_REPLICAGROUP ) ;

   rc = bson_append_string( &ops,
                            FIELD_NAME_GROUPNAME,
                            rg->_replicaGroupName ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   if ( NULL != options )
   {
      bson_iterator_init( &itr, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &itr ) )
      {
         rc = bson_append_element( &ops, NULL, &itr ) ;
         if ( SDB_OK != rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
   }

   rc = bson_finish( &ops ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = _runCommand2( rg->_connection,
                      &rg->_pSendBuffer, &rg->_sendBufferSize,
                      &rg->_pReceiveBuffer, &rg->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_REELECT),
                      0, 0, 0, -1,
                      &ops, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &ops ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbForceStepUp( sdbConnectionHandle cHandle,
                                 const bson *options )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = ( sdbConnectionStruct *)cHandle ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   rc = _runCommand2( cHandle,
                      &connection->_pSendBuffer, &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer, &connection->_receiveBufferSize,
                      (CMD_ADMIN_PREFIX CMD_NAME_FORCE_STEP_UP),
                      0, 0, 0, -1,
                      options, NULL, NULL, NULL,
                      NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbTruncateCollection( sdbConnectionHandle cHandle,
                                        const CHAR *fullName )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson option ;

   BSON_INIT( option ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( NULL == fullName ||
        0 == ossStrlen( fullName ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( option, FIELD_NAME_COLLECTION, fullName, string ) ;
   BSON_FINISH( option ) ;

   rc = _runCommand ( cHandle, connection->_sock,
                      &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE,
                      &option,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( option ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbPop( sdbCollectionHandle cHandle, bson *options )
{
   INT32 rc = SDB_OK ;
   bson newObj ;
   BOOLEAN bsoninit = FALSE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs = (sdbCollectionStruct*)cHandle ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct *)(cs->_connection) ;

   if ( !cs->_collectionFullName[0] || !options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_INIT( newObj ) ;
   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;

   {
      bson_iterator it ;
      bson_iterator_init( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 == ossStrcmp( key, FIELD_NAME_LOGICAL_ID) ||
              0 == ossStrcmp( key, FIELD_NAME_DIRECTION ) )
         {
            rc = bson_append_element( &newObj, NULL, &it ) ;
            if ( rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( newObj ) ;

   rc = _runCommand( cs->_connection, connection->_sock,
                     &connection->_pSendBuffer,
                     &connection->_sendBufferSize,
                     &connection->_pReceiveBuffer,
                     &connection->_receiveBufferSize,
                     connection->_endianConvert,
                     CMD_ADMIN_PREFIX CMD_NAME_POP,
                     &newObj,
                     NULL, NULL, NULL ) ;
   if ( rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDetachNode( sdbReplicaGroupHandle cHandle,
                                const CHAR *hostName,
                                const CHAR *serviceName,
                                const bson *options )
{
   INT32 rc         = SDB_OK ;
   sdbRGStruct *rg = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, rg, SDB_HANDLE_TYPE_REPLICAGROUP ) ;

   if ( NULL == hostName || !*hostName ||
        NULL == serviceName || !*serviceName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_GROUPNAME, rg->_replicaGroupName, string ) ;
   BSON_APPEND( obj, FIELD_NAME_HOST, hostName, string ) ;
   BSON_APPEND( obj, PMD_OPTION_SVCNAME, serviceName, string ) ;
   BSON_APPEND( obj, FIELD_NAME_ONLY_DETACH, 1, bool ) ;
   if ( NULL != options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key,
                              FIELD_NAME_ONLY_DETACH ) )
         {
            rc = bson_append_element( &obj, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( obj ) ;

   rc = _runCommand ( rg->_connection, rg->_sock, &rg->_pSendBuffer,
                      &rg->_sendBufferSize,
                      &rg->_pReceiveBuffer,
                      &rg->_receiveBufferSize,
                      rg->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE, &obj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbAttachNode( sdbReplicaGroupHandle cHandle,
                                const CHAR *hostName,
                                const CHAR *serviceName,
                                const bson *options )
{
   INT32 rc         = SDB_OK ;
   sdbRGStruct *rg = (sdbRGStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, rg, SDB_HANDLE_TYPE_REPLICAGROUP ) ;
   if ( NULL == hostName || !*hostName ||
        NULL == serviceName || !*serviceName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_GROUPNAME, rg->_replicaGroupName, string ) ;
   BSON_APPEND( obj, FIELD_NAME_HOST, hostName, string ) ;
   BSON_APPEND( obj, PMD_OPTION_SVCNAME, serviceName, string ) ;
   BSON_APPEND( obj, FIELD_NAME_ONLY_ATTACH, 1, bool ) ;
   if ( NULL != options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key,
                              FIELD_NAME_ONLY_ATTACH ) )
         {
            rc = bson_append_element( &obj, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( obj ) ;

   rc = _runCommand ( rg->_connection, rg->_sock, &rg->_pSendBuffer,
                      &rg->_sendBufferSize,
                      &rg->_pReceiveBuffer,
                      &rg->_receiveBufferSize,
                      rg->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE,
                      &obj,
                      NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

static INT32 _sdbAlterCollectionInternal ( sdbCollectionHandle cHandle,
                                           const CHAR * taskName,
                                           const bson * options,
                                           BOOLEAN allowNullArgs )
{
   INT32 rc = SDB_OK ;

   sdbCollectionStruct * cl = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;

   HANDLE_CHECK( cHandle, cl, SDB_HANDLE_TYPE_COLLECTION ) ;

   if ( NULL == taskName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = _sdbBuildAlterObject( taskName, options, allowNullArgs, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbAlterCollection( cHandle, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbCreateIdIndex ( sdbCollectionHandle cHandle,
                                    const bson * args )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_CRT_ID_INDEX, args, TRUE ) ;
}

SDB_EXPORT INT32 sdbDropIdIndex( sdbCollectionHandle cHandle )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_DROP_ID_INDEX, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbCreateAutoincrement ( sdbCollectionHandle cHandle,
                                          const bson * args )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_CRT_AUTOINC_FLD, args, FALSE ) ;
}

SDB_EXPORT INT32 sdbDropAutoincrement ( sdbCollectionHandle cHandle,
                                    const bson * args )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_DROP_AUTOINC_FLD, args, TRUE ) ;
}

SDB_EXPORT INT32 sdbEnableSharding ( sdbCollectionHandle cHandle,
                                     const bson * args )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_ENABLE_SHARDING, args, FALSE ) ;
}

SDB_EXPORT INT32 sdbDisableSharding ( sdbCollectionHandle cHandle )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_DISABLE_SHARDING, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbEnableCompression ( sdbCollectionHandle cHandle,
                                        const bson * args )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_ENABLE_COMPRESS, args, TRUE ) ;
}

SDB_EXPORT INT32 sdbDisableCompression ( sdbCollectionHandle cHandle )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_DISABLE_COMPRESS, NULL, TRUE ) ;
}

SDB_EXPORT INT32 sdbCreateAutoIncrement( sdbCollectionHandle cHandle,
                                         const bson * options )
{
   bson obj ;
   INT32 rc =  SDB_OK ;

   bson_init( &obj ) ;
   rc = bson_append_bson( &obj, "AutoIncrement", options ) ;
   if( SDB_OK != rc )
   {
      goto error ;
   }

   rc = bson_append_finish_object( &obj ) ;
   if( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_CRT_AUTOINC_FLD, &obj,
                                     FALSE ) ;
done:
   bson_destroy( &obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDropAutoIncrement( sdbCollectionHandle cHandle,
                                       const bson * options )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_DROP_AUTOINC_FLD,
                                       options, FALSE ) ;
}

SDB_EXPORT INT32 sdbCLSetAttributes( sdbCollectionHandle cHandle,
                                     const bson *options )
{
   return _sdbAlterCollectionInternal( cHandle, SDB_ALTER_CL_SET_ATTR, options, FALSE ) ;
}

SDB_EXPORT INT32 sdbCLGetDetail( sdbCollectionHandle cHandle,
                                 sdbCursorHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson newObj ;

   BSON_INIT( newObj ) ;

   if ( !handle )
   {
      rc = SDB_CLT_INVALID_HANDLE ;
      goto error ;
   }
   *handle = SDB_INVALID_HANDLE ;

   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   /* build collection name */
   BSON_APPEND( newObj, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;
   BSON_FINISH ( newObj ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_CL_DETAIL,
                      0, 0, -1, -1,
                      NULL, NULL, NULL, &newObj,
                      handle ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( newObj ) ;
   return rc ;
error :
   if ( handle && SDB_INVALID_HANDLE != *handle )
   {
      sdbReleaseCursor ( *handle ) ;
      SET_INVALID_HANDLE( handle ) ;
   }
   goto done ;
}

SDB_EXPORT INT32 sdbCLGetCollectionStat( sdbCollectionHandle cHandle,
                                         bson *result )
{
   INT32 rc                          = SDB_OK ;
   sdbCursorHandle cursor            = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection   = NULL ;
   sdbCollectionStruct *cs           = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                  = FALSE ;
   bson hint ;

   BSON_INIT( hint ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !cs->_collectionFullName[0] || !result )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // build hint
   BSON_APPEND( hint, FIELD_NAME_COLLECTION, cs->_collectionFullName, string ) ;
   BSON_FINISH( hint ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_CL_STAT,
                      0, 0, -1, -1,
                      NULL, NULL, NULL, &hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if( SDB_OK != rc )
   {
      goto error ;
   }

   // check return cursor
   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = sdbNext( cursor, result ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( hint ) ;
   if( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}
SDB_EXPORT INT32 sdbCLGetIndexStat( sdbCollectionHandle cHandle,
                                    const CHAR *pIndexName,
                                    bson *result )
{
   return sdbCLGetIndexStat1( cHandle, pIndexName, result, FALSE ) ;
}

SDB_EXPORT INT32 sdbCLGetIndexStat1( sdbCollectionHandle cHandle,
                                     const CHAR *pIndexName,
                                     bson *result,
                                     BOOLEAN detail )
{
   INT32 rc                        = SDB_OK ;
   sdbCursorHandle cursor          = SDB_INVALID_HANDLE ;
   sdbConnectionStruct *connection = NULL ;
   sdbCollectionStruct *cs         = (sdbCollectionStruct*)cHandle ;
   BOOLEAN bsoninit                = FALSE ;
   bson hint ;

   BSON_INIT( hint ) ;
   HANDLE_CHECK( cHandle, cs, SDB_HANDLE_TYPE_COLLECTION ) ;
   connection = (sdbConnectionStruct*)(cs->_connection) ;
   if ( !result )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( !cs->_collectionFullName[0] || !pIndexName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // { Collection: 'cl', Index: 'idx', $Options: { Detail: false } }
   BSON_APPEND( hint, FIELD_NAME_COLLECTION,
                cs->_collectionFullName, string ) ;
   BSON_APPEND( hint, FIELD_NAME_INDEX, pIndexName, string ) ;
   rc = bson_append_start_object( &hint, CMD_ADMIN_PREFIX FIELD_NAME_OPTIONS ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   BSON_APPEND( hint, FIELD_NAME_DETAIL, detail, bool ) ;
   rc = bson_append_finish_object( &hint ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }
   BSON_FINISH ( hint ) ;

   rc = _runCommand2( cs->_connection,
                      &cs->_pSendBuffer, &cs->_sendBufferSize,
                      &cs->_pReceiveBuffer, &cs->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_INDEX_STAT,
                      0, 0, -1, -1,
                      NULL, NULL, NULL, &hint,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = updateCachedObject( rc, connection->_tb, cs->_collectionFullName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   // check return cursor
   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   rc = sdbNext( cursor, result ) ;
   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_IXM_STAT_NOTEXIST ;
      goto error ;
   }
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( hint ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( cursor ) ;
   }
   return rc ;
error :
   goto done ;
}

static INT32 _setDCName ( sdbDCHandle cHandle,
                          const CHAR *pClusterName,
                          const CHAR *pBusinessName )
{
   INT32 rc         = SDB_OK ;
   sdbDCStruct *s   = (sdbDCStruct*)cHandle ;

   HANDLE_CHECK( cHandle, s, SDB_HANDLE_TYPE_DC ) ;
   if ( ossStrlen(pClusterName) + ossStrlen(pBusinessName) + 1 >
        CLIENT_DC_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   ossMemset( s->_name, 0, sizeof(s->_name) ) ;
   ossSnprintf( s->_name, CLIENT_DC_NAMESZ, "%s:%s",
                pClusterName, pBusinessName ) ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetDCName( sdbDCHandle cHandle, CHAR *pBuffer, INT32 size )
{
   INT32 rc                        = SDB_OK ;
   INT32 name_len                  = 0 ;
   sdbDCStruct *dc                 = (sdbDCStruct*)cHandle ;

   HANDLE_CHECK( cHandle, dc, SDB_HANDLE_TYPE_DC ) ;
   if ( NULL == pBuffer )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( size <= 0 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }

   name_len = ossStrlen( dc->_name ) ;
   if ( size < name_len + 1 )
   {
      rc = SDB_INVALIDSIZE ;
      goto error ;
   }
   ossStrncpy( pBuffer, dc->_name, name_len ) ;
   pBuffer[name_len] = 0 ;

done :
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbGetDC( sdbConnectionHandle cHandle, sdbDCHandle *handle )
{
   INT32 rc                        = SDB_OK ;
   sdbDCStruct *dc                 = NULL ;
   const CHAR *pClusterName        = NULL ;
   const CHAR *pBusinessName       = NULL ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   bson_iterator it ;
   bson_iterator subIt ;
   bson retObj ;
   bson subObj ;
   bson_init( &retObj ) ;
   bson_init( &subObj ) ;

   // check
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( NULL == handle  )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   // build dc handle
   ALLOC_HANDLE( dc, sdbDCStruct ) ;
   dc->_handleType    = SDB_HANDLE_TYPE_DC ;
   dc->_connection    = cHandle ;
   dc->_sock          = connection->_sock ;
   dc->_endianConvert = connection->_endianConvert ;

   // get dc name
   rc = sdbGetDCDetail( (sdbDCHandle)dc , &retObj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   if ( BSON_OBJECT == bson_find ( &it, &retObj, FIELD_NAME_DATACENTER ) )
   {
      bson_iterator_subobject( &it, &subObj ) ;
      if ( BSON_STRING == bson_find( &subIt, &subObj, FIELD_NAME_CLUSTERNAME ) )
      {
         pClusterName = bson_iterator_string( &subIt ) ;
      }
      if ( BSON_STRING == bson_find( &subIt, &subObj, FIELD_NAME_BUSINESSNAME ) )
      {
         pBusinessName = bson_iterator_string( &subIt ) ;
      }
   }
   if ( NULL == pClusterName || NULL == pBusinessName )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   rc = _setDCName( (sdbDCHandle)dc, pClusterName, pBusinessName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // register socket
   _regSocket( cHandle, &dc->_sock ) ;
   *handle = (sdbDCHandle)dc ;

done:
   bson_destroy( &retObj ) ;
   bson_destroy( &subObj ) ;
   return rc ;
error:
   if ( NULL != dc )
   {
      SDB_OSS_FREE( dc ) ;
   }
   SET_INVALID_HANDLE( handle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetDCDetail( sdbDCHandle cHandle, bson *retInfo )
{
   INT32 rc               = SDB_OK ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbDCStruct *dc        = (sdbDCStruct*)cHandle ;

   // check
   HANDLE_CHECK( cHandle, dc, SDB_HANDLE_TYPE_DC ) ;
   if ( NULL == dc->_sock )
   {
      rc = SDB_NOT_CONNECTED ;
      goto error ;
   }
   if ( NULL == retInfo  )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   // run command
   rc = _runCommand2( dc->_connection, &dc->_pSendBuffer, &dc->_sendBufferSize,
                      &dc->_pReceiveBuffer, &dc->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_DCINFO,
                      0, 0, -1, -1,
                      NULL, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
   // check
   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   // get dc info
   rc = sdbNext( cursor, retInfo ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor( (sdbCursorHandle)cursor ) ;
   }
   return rc ;
error:
   goto done ;
}

static INT32 _sdbDCCommon( sdbDCHandle cHandle, bson *info, const CHAR *pValue )
{
   INT32 rc                = SDB_OK ;
   const CHAR *pCommand    = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC ;
   sdbDCStruct *dc         = ( sdbDCStruct * )cHandle  ;
   bson newObj ;
   bson_init( &newObj ) ;

   HANDLE_CHECK( cHandle, dc, SDB_HANDLE_TYPE_DC ) ;

   BSON_APPEND( newObj, FIELD_NAME_ACTION, pValue, string ) ;
   if ( NULL != info )
   {
      BSON_APPEND( newObj, FIELD_NAME_OPTIONS, info, bson ) ;
   }
   BSON_FINISH( newObj ) ;

   rc = _runCommand( dc->_connection, dc->_sock,
                     &( dc->_pSendBuffer ),
                     &( dc->_sendBufferSize ),
                     &( dc->_pReceiveBuffer ),
                     &( dc->_receiveBufferSize ),
                     dc->_endianConvert,
                     pCommand, &newObj,
                     NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   bson_destroy( &newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbActivateDC( sdbDCHandle cHandle )
{
   return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_ACTIVATE ) ;
}

SDB_EXPORT INT32 sdbDeactivateDC( sdbDCHandle cHandle )
{
   return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_DEACTIVATE ) ;
}

SDB_EXPORT INT32 sdbEnableReadOnly( sdbDCHandle cHandle, BOOLEAN isReadOnly )
{
   if ( TRUE == isReadOnly )
   {
      return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_ENABLE_READONLY ) ;
   }
   else
   {
      return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_DISABLE_READONLY ) ;
   }
}

SDB_EXPORT INT32 sdbCreateImage( sdbDCHandle cHandle, const CHAR *pCataAddrList )
{
   INT32 rc = SDB_OK ;
   bson options ;
   bson_init( &options ) ;

   if ( NULL == pCataAddrList )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( options, FIELD_NAME_ADDRESS, pCataAddrList, string ) ;
   BSON_FINISH( options ) ;

   rc = _sdbDCCommon( cHandle, &options, CMD_VALUE_NAME_CREATE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   bson_destroy( &options ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbRemoveImage( sdbDCHandle cHandle )
{
   return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_REMOVE ) ;
}

SDB_EXPORT INT32 sdbEnableImage( sdbDCHandle cHandle )
{
   return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_ENABLE ) ;
}

SDB_EXPORT INT32 sdbDisableImage( sdbDCHandle cHandle )
{
   return _sdbDCCommon( cHandle, NULL, CMD_VALUE_NAME_DISABLE ) ;
}

SDB_EXPORT INT32 sdbAttachGroups( sdbDCHandle cHandle, bson *info )
{
   if ( NULL == info  )
   {
      return SDB_INVALIDARG ;
   }
   return _sdbDCCommon( cHandle, info, CMD_VALUE_NAME_ATTACH ) ;
}

SDB_EXPORT INT32 sdbDetachGroups( sdbDCHandle cHandle, bson *info )
{
   if ( NULL == info  )
   {
      return SDB_INVALIDARG ;
   }
   return _sdbDCCommon( cHandle, info, CMD_VALUE_NAME_DETACH ) ;
}

SDB_EXPORT INT32 sdbSyncDB( sdbConnectionHandle cHandle,
                            bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_SYNC_DB,
                      0, 0, -1, -1,
                      options, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbLoadCollectionSpace( sdbConnectionHandle cHandle,
                                         const CHAR *csName,
                                         bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson query ;

   if ( !csName || !*csName || ossStrlen ( csName ) > CLIENT_CS_NAMESZ )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   BSON_APPEND( query, FIELD_NAME_NAME, csName, string ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_NAME ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_LOAD_COLLECTIONSPACE,
                      0, 0, -1, -1,
                      &query, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = insertCachedObject( connection->_tb, csName ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( query ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbUnloadCollectionSpace( sdbConnectionHandle cHandle,
                                           const CHAR *csName,
                                           bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson query ;

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !csName || !*csName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( query, FIELD_NAME_NAME, csName, string ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_NAME ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_UNLOAD_COLLECTIONSPACE,
                      0, 0, -1, -1,
                      &query, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = removeCachedObject( connection->_tb, csName, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }


done:
   BSON_DESTROY( query ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSetPDLevel( sdbConnectionHandle cHandle,
                                INT32 pdLevel,
                                bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson query ;

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   BSON_APPEND( query, FIELD_NAME_PDLEVEL, pdLevel, int ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_PDLEVEL ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_SET_PDLEVEL,
                      0, 0, -1, -1,
                      &query, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( query ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbReloadConfig( sdbConnectionHandle cHandle,
                                  bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_RELOAD_CONFIG,
                      0, 0, -1, -1,
                      options, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbUpdateConfig( sdbConnectionHandle cHandle,
                                  bson *configs, bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND( newObj, NULL, &it, element ) ;
      }
   }
   BSON_APPEND( newObj, FIELD_NAME_CONFIGS, configs, bson ) ;
   BSON_FINISH( newObj ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_UPDATE_CONFIG,
                      0, 0, -1, -1,
                      &newObj, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbDeleteConfig( sdbConnectionHandle cHandle,
                                  bson *configs, bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson_iterator it ;
   bson newObj ;

   BSON_INIT( newObj) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;

   if ( options )
   {
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next ( &it ) )
      {
         BSON_APPEND( newObj, NULL, &it, element ) ;
      }
   }
   BSON_APPEND( newObj, FIELD_NAME_CONFIGS, configs, bson ) ;
   BSON_FINISH( newObj ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_DELETE_CONFIG,
                      0, 0, -1, -1,
                      &newObj, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbRenameCollectionSpace( sdbConnectionHandle cHandle,
                                           const CHAR *pOldName,
                                           const CHAR *pNewName,
                                           bson *options )
{
   INT32 rc         = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson query ;

   BSON_INIT( query ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pOldName || !*pOldName || !pNewName || !*pNewName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( query, FIELD_NAME_OLDNAME, pOldName, string ) ;
   BSON_APPEND( query, FIELD_NAME_NEWNAME, pNewName, string ) ;
   if ( options )
   {
      bson_iterator it ;
      bson_iterator_init ( &it, options ) ;
      while ( BSON_EOO != bson_iterator_next( &it ) )
      {
         const CHAR *key = bson_iterator_key( &it ) ;
         if ( 0 != ossStrcmp( key, FIELD_NAME_OLDNAME ) &&
              0 != ossStrcmp( key, FIELD_NAME_NEWNAME ) )
         {
            rc = bson_append_element( &query, NULL, &it ) ;
            if ( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
      }
   }
   BSON_FINISH( query ) ;

   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTIONSPACE,
                      0, 0, -1, -1,
                      &query, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = removeCachedObject( connection->_tb, pOldName, FALSE ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }


done:
   BSON_DESTROY( query ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbAnalyze( sdbConnectionHandle cHandle,
                             bson *options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;

   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   rc = _runCommand2( cHandle, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_ANALYZE,
                      0, 0, -1, -1,
                      options, NULL, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbGetLastErrorObj( sdbConnectionHandle cHandle, bson *obj )
{
   INT32 rc        = SDB_OK ;
   BOOLEAN hasInit = FALSE ;
   bson result ;
   sdbConnectionStruct *pStruct = _sdbGetConnHandleStruct( cHandle ) ;

   if ( !pStruct || !obj || SDB_HANDLE_TYPE_CONNECTION != pStruct->_handleType )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }
   if ( pStruct->_pErrObjBuf && pStruct->_errObjBufSize >= 5 &&
        *(INT32*)(pStruct->_pErrObjBuf) >= 5 )
   {
      bson_init( &result ) ;
      hasInit = TRUE ;
      rc = bson_init_finished_data( &result, pStruct->_pErrObjBuf ) ;
      if ( rc )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      rc = bson_copy( obj, &result ) ;
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
   }
   else
   {
      rc = SDB_DMS_EOC ;
      goto error ;
   }

done:
   if ( hasInit )
   {
      bson_destroy( &result ) ;
   }
   return rc ;
error:
   goto done ;
}

SDB_EXPORT void sdbCleanLastErrorObj( sdbConnectionHandle cHandle )
{
   sdbConnectionStruct *pStruct = _sdbGetConnHandleStruct( cHandle ) ;
   if ( !pStruct || SDB_HANDLE_TYPE_CONNECTION != pStruct->_handleType )
   {
      return ;
   }
   pStruct->_pErrObjBuf = NULL ;
   pStruct->_errObjBufSize = 0 ;
}

SDB_EXPORT INT32 sdbCreateSequence( sdbConnectionHandle cHandle,
                                    const CHAR *pSequenceName,
                                    const bson *options,
                                    sdbSequenceHandle *sHandle )
{
   INT32 rc                = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   CHAR *pCommand          = CMD_ADMIN_PREFIX CMD_NAME_CREATE_SEQUENCE ;
   sdbSequenceStruct *s    = NULL ;
   BOOLEAN bsoninit        = FALSE ;
   bson newObj ;
   INT32 size              = 0 ;
   const CHAR *key         = NULL ;

   BSON_INIT( newObj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pSequenceName || !*pSequenceName || !sHandle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( newObj, FIELD_NAME_NAME, pSequenceName, string ) ;

   if ( options )
   {
      bson_iterator itr ;
      bson_iterator_init( &itr, options ) ;
      while ( bson_iterator_more( &itr ) )
      {
         bson_iterator_next( &itr ) ;
         key = bson_iterator_key ( &itr );
         if ( 0 == ossStrcmp( key, FIELD_NAME_NAME ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         BSON_APPEND( newObj, NULL, &itr, element ) ;
      }
   }

   BSON_FINISH ( newObj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCommand, &newObj, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   size =  sizeof( sdbSequenceStruct ) + ossStrlen( pSequenceName ) ;
   s = ( sdbSequenceStruct* ) SDB_OSS_MALLOC ( size ) ;
   if ( !s )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset ( s, 0, size ) ;
   s->_handleType    = SDB_HANDLE_TYPE_SEQUENCE ;
   s->_connection    = cHandle ;
   s->_sock          = connection->_sock ;
   ossStrcpy( s->_name, pSequenceName ) ;

   _regSocket( s->_connection, &s->_sock ) ;
   *sHandle = (sdbSequenceHandle) s ;

done:
   BSON_DESTROY( newObj ) ;
   return rc ;
error:
   if ( s )
   {
      SDB_OSS_FREE( s ) ;
   }
   SET_INVALID_HANDLE( sHandle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbGetSequence( sdbConnectionHandle cHandle,
                                 const CHAR *pSequenceName,
                                 sdbSequenceHandle *sHandle )
{
   INT32 rc               = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   sdbSequenceStruct *s   = NULL ;
   BOOLEAN bsoninit       = FALSE ;
   bson condition ;
   bson result ;
   INT32 size = 0 ;

   BSON_INIT( condition ) ;
   BSON_INIT( result ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pSequenceName || !*pSequenceName || !sHandle )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( '$' == pSequenceName[ 0 ] ||
        0 == ossStrncasecmp( "SYS", pSequenceName, 3 ) )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( condition, FIELD_NAME_NAME, pSequenceName, string ) ;
   BSON_FINISH ( condition ) ;

   rc = _sdbGetList ( cHandle, SDB_LIST_SEQUENCES,
                      &condition, NULL, NULL, NULL,
                      0, -1,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbNext ( cursor, &result ) ;
   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_SEQUENCE_NOT_EXIST ;
   }
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   size =  sizeof( sdbSequenceStruct ) + ossStrlen( pSequenceName ) + 1 ;
   s = ( sdbSequenceStruct* ) SDB_OSS_MALLOC ( size ) ;
   if ( !s )
   {
      rc = SDB_OOM ;
      goto error ;
   }
   ossMemset ( s, 0, size ) ;
   s->_handleType    = SDB_HANDLE_TYPE_SEQUENCE ;
   s->_connection    = cHandle ;
   s->_sock          = connection->_sock ;
   ossStrcpy( s->_name, pSequenceName ) ;

   _regSocket( s->_connection, &s->_sock ) ;
   *sHandle = (sdbSequenceHandle) s ;

done :
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }

   BSON_DESTROY( condition ) ;
   BSON_DESTROY( result ) ;
   return rc ;
error :
   if ( s )
   {
      SDB_OSS_FREE( s ) ;
   }
   SET_INVALID_HANDLE( sHandle ) ;
   goto done ;
}

SDB_EXPORT INT32 sdbRenameSequence( sdbConnectionHandle cHandle,
                                    const CHAR *pOldName,
                                    const CHAR *pNewName )
{
   INT32 rc               = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   CHAR *pCommand         = CMD_ADMIN_PREFIX CMD_NAME_ALTER_SEQUENCE ;
   BOOLEAN bsoninit       = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pOldName || !*pOldName || !pNewName || !*pNewName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_ACTION, CMD_VALUE_NAME_RENAME, string ) ;

   rc = bson_append_start_object( &obj, FIELD_NAME_OPTIONS ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_NAME, pOldName, string ) ;
   BSON_APPEND( obj, FIELD_NAME_NEWNAME, pNewName, string ) ;

   rc = bson_append_finish_object( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   BSON_FINISH( obj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCommand, &obj, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done :
   BSON_DESTROY( obj ) ;
   return rc ;
error :
   goto done ;
}

SDB_EXPORT INT32 sdbDropSequence( sdbConnectionHandle cHandle,
                                  const CHAR *pSequenceName )
{
   INT32 rc                = SDB_OK ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)cHandle ;
   CHAR *pCommand          = CMD_ADMIN_PREFIX CMD_NAME_DROP_SEQUENCE ;
   BOOLEAN bsoninit        = FALSE ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( cHandle, connection, SDB_HANDLE_TYPE_CONNECTION ) ;
   if ( !pSequenceName || !*pSequenceName )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_NAME, pSequenceName, string ) ;
   BSON_FINISH ( obj ) ;

   rc = _runCommand ( cHandle, connection->_sock, &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      pCommand, &obj, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT void sdbReleaseSequence( sdbSequenceHandle sHandle )
{
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;

   if ( SDB_INVALID_HANDLE != sHandle && SDB_HANDLE_TYPE_SEQUENCE == s->_handleType )
   {
      _unregSocket( s->_connection, &s->_sock ) ;
      SDB_OSS_FREE ( (sdbSequenceStruct*)sHandle ) ;
   }
}

static INT32 _sdbAlterSequenceInternal ( sdbSequenceHandle sHandle,
                                         const CHAR * actionName,
                                         const bson * options )
{
   INT32 rc = SDB_OK ;
   sdbConnectionStruct *connection = NULL ;
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;
   BOOLEAN bsoninit = FALSE ;
   bson obj ;
   bson_iterator iter ;
   bson_type type ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;
   connection = (sdbConnectionStruct*)(s->_connection) ;
   if ( NULL == options )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_ACTION, actionName, string ) ;

   rc = bson_append_start_object( &obj, FIELD_NAME_OPTIONS ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_NAME, s->_name, string ) ;

   type = bson_find ( &iter, options, FIELD_NAME_NAME );
   if ( BSON_EOO != type )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = bson_append_elements( &obj, options ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   rc = bson_append_finish_object( &obj ) ;
   if ( SDB_OK != rc )
   {
      rc = SDB_DRIVER_BSON_ERROR ;
      goto error ;
   }

   BSON_FINISH( obj ) ;

   rc = _runCommand ( (sdbConnectionHandle) connection,
                      s->_sock,
                      &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      connection->_endianConvert,
                      CMD_ADMIN_PREFIX CMD_NAME_ALTER_SEQUENCE,
                      &obj, NULL, NULL, NULL ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeqSetAttributes( sdbSequenceHandle sHandle,
                                      const bson *options )
{
   return _sdbAlterSequenceInternal( sHandle, CMD_VALUE_NAME_SETATTR, options ) ;
}

SDB_EXPORT INT32 sdbSeqGetNextValue( sdbSequenceHandle sHandle, INT64 *value )
{
   INT32 rc = SDB_OK ;
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;
   INT32 returnNum = 0 ;
   INT32 increment = 0 ;

   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;
   if ( !value )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = sdbSeqFetch( sHandle, 1, value, &returnNum, &increment ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }
done:
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeqGetCurrentValue( sdbSequenceHandle sHandle,
                                        INT64 *value )
{
   INT32 rc                = SDB_OK ;
   BOOLEAN bsoninit        = FALSE ;
   sdbSequenceStruct *s    = (sdbSequenceStruct*)sHandle ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)(s->_connection) ;
   sdbCursorHandle cursor  = SDB_INVALID_HANDLE ;
   bson_iterator it ;
   bson obj ;
   bson retObj ;

   BSON_INIT( obj ) ;
   BSON_INIT( retObj ) ;
   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;
   if ( !value )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( NULL == s->_sock )
   {
      rc = SDB_NOT_CONNECTED ;
      goto error ;
   }

   BSON_APPEND( obj, FIELD_NAME_NAME, s->_name, string ) ;
   BSON_FINISH( obj ) ;

   rc = _runCommand2( (sdbConnectionHandle) connection,
                      &connection->_pSendBuffer,
                      &connection->_sendBufferSize,
                      &connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      CMD_ADMIN_PREFIX CMD_NAME_GET_SEQ_CURR_VAL,
                      0, 0, -1, -1,
                      &obj, NULL, NULL, NULL,
                      &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( SDB_INVALID_HANDLE == cursor )
   {
      rc = SDB_SYS ;
      goto error ;
   }

   rc = sdbNext( cursor, &retObj ) ;
   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_UNEXPECTED_RESULT ;
   }
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( BSON_LONG == bson_find ( &it, &retObj, FIELD_NAME_CURRENT_VALUE) )
   {
      *value = bson_iterator_long ( &it ) ;
   }
   else
   {
      rc = SDB_UNEXPECTED_RESULT ;
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   BSON_DESTROY( retObj ) ;
   if ( SDB_INVALID_HANDLE != cursor )
   {
      sdbReleaseCursor ( cursor ) ;
   }
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeqSetCurrentValue( sdbSequenceHandle sHandle,
                                        const INT64 value )
{
   INT32 rc = SDB_OK ;
   BOOLEAN bsoninit = FALSE ;
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;

   BSON_APPEND( obj, FIELD_NAME_EXPECT_VALUE, value, long ) ;
   BSON_FINISH( obj ) ;

   rc = _sdbAlterSequenceInternal( sHandle, CMD_VALUE_NAME_SET_CURR_VALUE, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeqFetch( sdbSequenceHandle sHandle,
                              const INT32 fetchNum,
                              INT64 *nextValue,
                              INT32 *returnNum,
                              INT32 *increment )
{
   INT32 rc = SDB_OK ;
   BOOLEAN bsoninit = FALSE ;
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;
   sdbConnectionStruct *connection = (sdbConnectionStruct*)(s->_connection) ;
   INT64 contextID = -1 ;
   sdbCursorHandle cursor = SDB_INVALID_HANDLE ;
   bson obj ;
   bson_iterator itr ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;
   if ( !nextValue || !returnNum || !increment )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   if ( fetchNum < 1 )
   {
      rc = SDB_INVALIDARG ;
      goto error ;
   }

   rc = clientBuildSeqFetchMsg( &connection->_pSendBuffer,
                                &connection->_sendBufferSize,
                                s->_name, fetchNum, 0,
                                connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _sendAndRecv( s->_connection, s->_sock,
                      (MsgHeader*)connection->_pSendBuffer,
                      (MsgHeader**)&connection->_pReceiveBuffer,
                      &connection->_receiveBufferSize,
                      TRUE, connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _extract( s->_connection,
                  (MsgHeader *)connection->_pReceiveBuffer,
                  connection->_receiveBufferSize,
                  &contextID,
                  connection->_endianConvert ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = _getRetInfo( s->_connection, &connection->_pReceiveBuffer,
                     &connection->_receiveBufferSize,
                     contextID, &cursor ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   rc = sdbNext ( cursor, &obj ) ;
   if ( SDB_DMS_EOC == rc )
   {
      rc = SDB_UNEXPECTED_RESULT ;
   }
   if ( SDB_OK != rc )
   {
      goto error ;
   }

   if ( BSON_LONG == bson_find ( &itr, &obj, FIELD_NAME_NEXT_VALUE ) )
   {
      *nextValue = bson_iterator_long ( &itr ) ;
   }
   else
   {
      rc = SDB_UNEXPECTED_RESULT ;
      goto error ;
   }

   if ( BSON_INT == bson_find ( &itr, &obj, FIELD_NAME_RETURN_NUM ) )
   {
      *returnNum = bson_iterator_long ( &itr ) ;
   }
   else
   {
      rc = SDB_UNEXPECTED_RESULT ;
      goto error ;
   }

   if ( BSON_INT == bson_find ( &itr, &obj, FIELD_NAME_INCREMENT ) )
   {
      *increment = bson_iterator_long ( &itr ) ;
   }
   else
   {
      rc = SDB_SYS ;
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   if ( cursor != SDB_INVALID_HANDLE )
   {
      sdbReleaseCursor ( cursor ) ;
   }
   return rc ;
error:
   goto done ;
}

SDB_EXPORT INT32 sdbSeqRestart( sdbSequenceHandle sHandle,
                                const INT64 startValue )
{
   INT32 rc = SDB_OK ;
   BOOLEAN bsoninit = FALSE ;
   sdbSequenceStruct *s = (sdbSequenceStruct*)sHandle ;
   bson obj ;

   BSON_INIT( obj ) ;
   HANDLE_CHECK( sHandle, s, SDB_HANDLE_TYPE_SEQUENCE ) ;

   BSON_APPEND( obj, FIELD_NAME_START_VALUE, startValue, long ) ;
   BSON_FINISH( obj ) ;

   rc = _sdbAlterSequenceInternal( sHandle, CMD_VALUE_NAME_RESTART, &obj ) ;
   if ( SDB_OK != rc )
   {
      goto error ;
   }

done:
   BSON_DESTROY( obj ) ;
   return rc ;
error:
   goto done ;
}
