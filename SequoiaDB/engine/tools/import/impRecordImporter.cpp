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

   Source File Name = impRecordImporter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordImporter.hpp"
#include "ossUtil.h"
#include "pd.hpp"
#include "msgDef.h"
#include "../client/client_internal.h"
#include "msg.hpp"
#include "impUtil.hpp"

#if defined( _LINUX ) || defined (_AIX)
#include <arpa/inet.h>
#include <netinet/tcp.h>
#endif

namespace import
{
   #define IMP_MAX_RECORDS_SIZE (SDB_MAX_MSG_LENGTH - 1024 * 1024 * 1)
   #define IMP_DEFAULT_NETWORK_TIMEOUT (-1)
   #define BSON_MIN_SIZE 5
   #define IMP_DUPLICATED_NUMBER "DuplicatedNum" 
   static INT32 defaultVersion = 1 ;
   static INT16 defaultW = 0 ;

   RecordImporter::RecordImporter( const string& hostname,
                                   const string& svcname,
                                   const string& user,
                                   const string& password,
                                   const string& csname,
                                   const string& clname,
                                   BOOLEAN useSSL,
                                   BOOLEAN enableTransaction,
                                   BOOLEAN allowKeyDuplication,
                                   BOOLEAN replaceKeyDuplication,
                                   BOOLEAN allowIDKeyDuplication,
                                   BOOLEAN replaceIDKeyDuplication,
                                   BOOLEAN mustHasIDField,
                                   INT32 batchSize )
         : _insertBufferSize( 0 ),
           _recvBufferSize( 0 ),
           _resultBufferSize( 0 ),
           _useSSL( useSSL ),
           _enableTransaction( enableTransaction ),
           _allowKeyDuplication( allowKeyDuplication ),
           _replaceKeyDuplication( replaceKeyDuplication ),
           _allowIDKeyDuplication( allowIDKeyDuplication),
           _replaceIDKeyDuplication( replaceIDKeyDuplication ),
           _endianConvert( FALSE ),
           _mustHasIDField( mustHasIDField ),
           _connection( SDB_INVALID_HANDLE ),
           _collectionSpace( SDB_INVALID_HANDLE ),
           _collection( SDB_INVALID_HANDLE ),
           _insertMsg( NULL ),
           _insertBuffer( NULL ),
           _recvBuffer( NULL ),
           _resultBuffer( NULL ),
           _hostname( hostname ),
           _svcname( svcname ),
           _user( user ),
           _password( password ),
           _csname( csname ),
           _clname( clname ),
           _batchSize( batchSize ),
           _msgConvertor( NULL ),
           _peerProtocolVersion( SDB_PROTOCOL_VER_INVALID )
   {

   }

   RecordImporter::~RecordImporter()
   {
      disconnect() ;

      SAFE_OSS_FREE( _insertBuffer ) ;
      SAFE_OSS_FREE( _recvBuffer ) ;

      if ( _msgConvertor )
      {
         delete _msgConvertor ;
         _msgConvertor = NULL ;
      }
   }

   INT32 RecordImporter::connect()
   {
      INT32 rc = SDB_OK ;
      sdbConnectionStruct *connection = NULL ;
      CHAR sourceInfo[ IMP_UTIL_SOURCE_INFO_MAX + 1 ] = { 0 } ;
      bson option ;
      bson_init( &option ) ;

      SDB_ASSERT( SDB_INVALID_HANDLE == _connection, "Already connected" ) ;
      SDB_ASSERT( SDB_INVALID_HANDLE == _collectionSpace,
                  "Already get collection space" ) ;
      SDB_ASSERT( SDB_INVALID_HANDLE == _collection, "Already get collection");

      if ( _useSSL )
      {
         rc = sdbSecureConnect( _hostname.c_str(), _svcname.c_str(),
                                _user.c_str(), _password.c_str(),
                                &_connection ) ;
      }
      else
      {
         rc = sdbConnect( _hostname.c_str(), _svcname.c_str(),
                          _user.c_str(), _password.c_str(),
                          &_connection ) ;
      }

      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to connect to database %s:%s, "
                          "rc = %d, usessl=%d",
                 _hostname.c_str(), _svcname.c_str(), rc, _useSSL ) ;
         goto error ;
      }

      // set source info
      genSourceInfo( sourceInfo, IMP_UTIL_SOURCE_INFO_MAX, "sdbimprt" ) ;
      bson_append_string( &option, FIELD_NAME_SOURCE, sourceInfo ) ;
      rc = bson_finish( &option ) ;
      if ( rc )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         PD_LOG( PDERROR, "Failed to build bson, rc = %d", rc ) ;
         goto error ;
      }
      rc = sdbSetSessionAttr( _connection, &option ) ;
      if ( rc )
      {
         PD_LOG( PDWARNING, "Failed to set session attribute, rc = %d. Continue to import", rc ) ;
      }

      connection = (sdbConnectionStruct*)_connection ;
      _endianConvert = connection->_endianConvert ;
      _peerProtocolVersion = connection->_peerProtocolVersion ;
      if ( SDB_PROTOCOL_VER_1 == _peerProtocolVersion )
      {
         _msgConvertor = new(std::nothrow)sdbMsgConvertor() ;
         if ( !_msgConvertor )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to alloc msg convertor, rc = %d", rc ) ;
            goto error ;
         }
      }
      rc = _setSessionAttr() ;
      if ( SDB_OK != rc )
      {
         ossPrintf( "Failed to set session attributes" OSS_NEWLINE ) ;
         PD_LOG( PDERROR, "Failed to set session attributes, rc = %d", rc ) ;
         goto error ;
      }

      rc = sdbGetCollectionSpace( _connection, _csname.c_str(),
                                  &_collectionSpace ) ;
      if ( rc )
      {
         if ( SDB_DMS_CS_NOTEXIST == rc )
         {
            ossPrintf( "Collection space %s does not exist" OSS_NEWLINE,
                       _csname.c_str() ) ;
            PD_LOG( PDERROR, "Collection space %s does not exist, rc = %d",
                    _csname.c_str(), rc ) ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to get collection space %s, rc = %d",
                    _csname.c_str(), rc ) ;
         }

         goto error ;
      }

      rc = sdbGetCollection1( _collectionSpace, _clname.c_str(),
                              &_collection ) ;
      if ( rc )
      {
         if ( SDB_DMS_NOTEXIST == rc )
         {
            ossPrintf( "Collection %s does not exist" OSS_NEWLINE,
                       _clname.c_str() ) ;
            PD_LOG( PDERROR, "Collection %s does not exist, rc = %d",
                    _clname.c_str(), rc ) ;
         }
         else if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get collection %s, rc = %d",
                    _clname.c_str(), rc ) ;
         }

         goto error ;
      }

      rc = _initInsertMsg() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to init insert message, rc=%d", rc ) ;
         goto error ;
      }

   done:
      bson_destroy( &option ) ;
      return rc ;
   error:
      goto done ;
   }

   void RecordImporter::disconnect()
   {
      if ( SDB_INVALID_HANDLE != _collection )
      {
         sdbReleaseCollection( _collection ) ;
         _collection = SDB_INVALID_HANDLE ;
      }

      if ( SDB_INVALID_HANDLE != _collectionSpace )
      {
         sdbReleaseCS( _collectionSpace ) ;
         _collectionSpace = SDB_INVALID_HANDLE ;
      }

      if ( SDB_INVALID_HANDLE != _connection )
      {
         sdbDisconnect( _connection ) ;
         sdbReleaseConnection( _connection ) ;
         _connection = SDB_INVALID_HANDLE ;
      }
   }

   INT32 RecordImporter::import( PageInfo* pageInfo )
   {
      INT32 rc = SDB_OK;
      INT32 flag = 0;

      SDB_ASSERT( NULL != pageInfo, "pageInfo can't be NULL" ) ;

      if ( _enableTransaction )
      {
         rc = sdbTransactionBegin( _connection ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to begin transaction, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( _allowKeyDuplication )
      {
         flag |= FLG_INSERT_CONTONDUP ;
      }
      else if ( _replaceKeyDuplication )
      {
         flag |= FLG_INSERT_REPLACEONDUP ;
      }
      else if ( _allowIDKeyDuplication )
      {
         flag |= FLG_INSERT_CONTONDUP_ID ;
      }
      else if ( _replaceIDKeyDuplication )
      {
         flag |= FLG_INSERT_REPLACEONDUP_ID ;
      }

      // Inform coord or data nodes that the '_id' field is included in records.
      // The '_id' field was added by RecordParser when '_mustHasIDField' is true.
      SDB_ASSERT( _mustHasIDField, "can not be false" ) ;
      if ( _mustHasIDField )
      {
         flag |= FLG_INSERT_HAS_ID_FIELD ;
      }

      rc = _bulkInsert( pageInfo, flag ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to bulk insert, rc=%d", rc ) ;

         if ( _enableTransaction )
         {
            INT32 ret = SDB_OK ;

            ret = sdbTransactionRollback( _connection ) ;
            if ( ret )
            {
               PD_LOG( PDERROR, "Failed to rollback transaction, rc=%d", ret ) ;
               rc = ret ;
               goto error ;
            }
         }

         goto error ;
      }

      if ( _enableTransaction )
      {
         rc = sdbTransactionCommit( _connection ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to commit transaction, rc=%d", rc ) ;
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_getLastResultObj( bson *result )
   {
      INT32 rc = SDB_OK ;

      if ( !result ) 
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _resultBuffer && _resultBufferSize >= BSON_MIN_SIZE && *( INT32* )_resultBuffer >= BSON_MIN_SIZE )
      {
         //Since the result is obtained from the outside, it will be
         //used up immediately, so there is no need to add bson_copy proccessing.
         rc = bson_init_finished_data( result, _resultBuffer ) ;
         if ( rc )
         {
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 RecordImporter::_initInsertMsg()
   {
      INT32 rc = SDB_OK ;
      INT32 nameLength = 0 ;
      sdbCollectionStruct *cls = NULL ;
      const CHAR *clName = NULL ;

      cls = (sdbCollectionStruct*)_collection ;
      clName = cls->_collectionFullName ;

      nameLength = ossStrlen( clName ) ;

      _insertBufferSize = ossRoundUpToMultipleX(
            offsetof( MsgOpInsert, name ) + nameLength + 1, 4 ) ;

      _insertBuffer = (CHAR*)SDB_OSS_MALLOC( _insertBufferSize ) ;
      if ( NULL == _insertBuffer )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc buffer, size=%d",
                 _insertBufferSize ) ;
         goto error ;
      }

      _insertMsg = (MsgOpInsert*)_insertBuffer ;
      _insertMsg->header.requestID     = 0 ;
      _insertMsg->header.opCode        = MSG_BS_INSERT_REQ ;
      _insertMsg->header.eye           = MSG_COMM_EYE_DEFAULT ;
      _insertMsg->header.version       = SDB_PROTOCOL_VER_2 ;
      _insertMsg->header.flags         = 0 ;
      _insertMsg->header.routeID.value = 0 ;
      _insertMsg->header.TID           = ossGetCurrentThreadID() ;
      ossMemset( &(_insertMsg->header.globalID), 0,
                sizeof( _insertMsg->header.globalID ) ) ;
      ossMemset( _insertMsg->header.reserve, 0,
                sizeof( _insertMsg->header.reserve ) ) ;

      ossEndianConvertIf( defaultVersion, _insertMsg->version, _endianConvert );
      ossEndianConvertIf( defaultW, _insertMsg->w, _endianConvert ) ;
      ossEndianConvertIf( nameLength, _insertMsg->nameLength, _endianConvert ) ;

      ossStrncpy( _insertMsg->name, clName, nameLength ) ;
      _insertMsg->name[nameLength] = 0 ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_bulkInsert( PageInfo* pageInfo, SINT32 flag )
   {
      INT32 rc = SDB_OK ;
      INT32 packetLength = _insertBufferSize ;
      BsonPage* pages = pageInfo->pages ;
      bson resExtract ;
      bson_iterator it ;
      bson_type type ;

      bson_init( &resExtract ) ;
      while( pages )
      {
         packetLength += pages->getRecordsSize() ;
         pages = pages->getNext() ;
      }

      flag |= FLG_INSERT_RETURNNUM ;
      _insertMsg->header.messageLength = packetLength ;
      ossEndianConvertIf( flag, _insertMsg->flags, _endianConvert ) ;

      rc = _send( _insertBuffer, _insertBufferSize ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to send header buffer, rc=%d", rc ) ;
         goto error ;
      }

      pages = pageInfo->pages ;
      while( pages )
      {
         rc = _send( pages->getBuffer(), pages->getRecordsSize(), FALSE ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to send bson buffer, rc=%d", rc ) ;
            goto error ;
         }

         pages = pages->getNext() ;
      }

      rc = _recv() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to recv message, rc=%d", rc ) ;
         goto error ;
      }

      rc = _extract() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to extract message, rc=%d", rc ) ;
         goto error ;
      }
      // Since the memory space of the bson resExtract is shared,
      // bson_destory(&resExtract) is not required.	  
      rc = _getLastResultObj( &resExtract ) ;
      if ( SDB_OK == rc )
      {
         type = bson_find( &it, &resExtract, IMP_DUPLICATED_NUMBER ) ;
         if ( BSON_LONG == type )
         {
            pageInfo->duplicatedNum = bson_iterator_long( &it ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Failed to get duplicate count. DuplicatedNum's type is %d , "
                               "but it must be BSON_LONG ", type ) ;
         }
      }
      else
      {
         rc = SDB_OK ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_send( const CHAR *pMsg, INT32 len, BOOLEAN isHeader )
   {
      INT32 rc                 = SDB_OK ;
      INT32 sentSize           = 0 ;
      INT32 totalSentSize      = 0 ;
      sdbCollectionStruct *cls = (sdbCollectionStruct*)_collection ;
      Socket* sock             = cls->_sock ;
      CHAR *pBuffer            = (CHAR*)pMsg ;

      if ( NULL == sock )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( isHeader && _msgConvertor )
      {
         // the output 'len' will be changed
         rc = _msgConvertor->downgradeRequest( pMsg, len, pBuffer, len ) ;
         if ( rc )
         {
            goto error ;
         }
      }

      while( len > totalSentSize )
      {
         rc = clientSend( sock, pBuffer + totalSentSize, len - totalSentSize,
                          &sentSize, IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_recv()
   {
      INT32 rc                 = SDB_OK ;
      INT32 len                = 0 ;
      INT32 realLen            = 0 ;
      INT32 receivedLen        = 0 ;
      INT32 totalReceivedLen   = 0 ;
      _resultBuffer            = NULL ;
      _recvBufferSize          = 0 ;
      sdbCollectionStruct *cls = (sdbCollectionStruct*)_collection ;
      Socket* sock             = cls->_sock ;
      INT32 minReplySize       = ( SDB_PROTOCOL_VER_1 == _peerProtocolVersion ) ?
                                 sizeof(MsgOpReplyV1) : sizeof(MsgOpReply) ;
      CHAR **ppBuffer          = &_recvBuffer ;

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
                           IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
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
            setsockopt( clientGetRawSocket( sock ),
                        IPPROTO_TCP, TCP_QUICKACK,
                        (void*)&i, sizeof(i) ) ;
         }
   #endif
         break ;
      }

      ossEndianConvertIf4 ( len, realLen, _endianConvert ) ;

      if ( realLen < minReplySize )
      {
         rc = SDB_NET_BROKEN_MSG ;
         goto error ;
      }

      if ( _recvBufferSize < realLen )
      {
         _recvBufferSize = realLen ;

         _recvBuffer = (CHAR*)SDB_OSS_REALLOC( _recvBuffer, _recvBufferSize ) ;
         if ( NULL == _recvBuffer )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to malloc buffer, size=%d",
                    _recvBufferSize ) ;
            goto error ;
         }

         ossMemcpy( _recvBuffer, &realLen, sizeof( realLen ) ) ;
      }

      receivedLen = 0 ;
      totalReceivedLen = sizeof( realLen ) ;
      while ( TRUE )
      {
         INT32 recvLen = realLen - totalReceivedLen ;

         rc = clientRecv ( sock, _recvBuffer + totalReceivedLen,
                           recvLen, &receivedLen,
                           IMP_DEFAULT_NETWORK_TIMEOUT ) ;
         totalReceivedLen += receivedLen ;
         if ( SDB_TIMEOUT == rc )
         {
            continue ;
         }
         else if ( rc )
         {
            goto error ;
         }

         if ( realLen == totalReceivedLen )
         {
            break ;
         }
      }
      // upgrade the reply
      if ( _msgConvertor )
      {
         INT32 tmpLen = 0 ;
         CHAR *tmpPtr = NULL ;
         rc = _msgConvertor->upgradeReply( *ppBuffer, tmpPtr, tmpLen ) ;
         if ( rc )
         {
            goto error ;
         }

         SDB_ASSERT( tmpLen == *(INT32 *)tmpPtr,
                     "Converted message length is not as expected" ) ;
         // copy the result back to the received buffer
         if ( tmpLen > realLen )
         {
            rc = reallocBuffer( ppBuffer, &_recvBufferSize, tmpLen ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         ossMemcpy( *ppBuffer, tmpPtr, tmpLen ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 RecordImporter::_extract()
   {
      INT32 rc          = SDB_OK ;
      INT32 replyFlag   = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom   = -1 ;
      SINT64 contextID  = 0 ;

      rc = clientExtractReply( _recvBuffer, &replyFlag, &contextID,
                               &startFrom, &numReturned, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = replyFlag ;
      if ( SDB_OK == replyFlag && 1 == numReturned &&
          ( MSG_BS_INSERT_RES == ( (MsgHeader*)_recvBuffer)->opCode ) )
      {
         INT32 dataOff   = 0;
         INT32 dataSize  = 0;
         MsgOpReply *pTmpReply = ( MsgOpReply* )_recvBuffer ;
         //get the size of the response packet from the first four bytes of _recvBuffer
         dataOff = ossRoundUpToMultipleX( sizeof( MsgOpReply ), 4 ) ;
         dataSize = pTmpReply->header.messageLength - dataOff ;
         //save result info
         if ( dataSize > 0 )
         {
            _resultBuffer = _recvBuffer + dataOff ;
            _resultBufferSize = dataSize ;
            /// should truncate the message size
            pTmpReply->header.messageLength = sizeof( MsgOpReply ) ;
            pTmpReply->numReturned = 0 ;
         }
         else
         {
            _resultBuffer = NULL ;
            _recvBufferSize = 0 ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 RecordImporter::_setSessionAttr()
   {
      INT32 rc = SDB_OK ;
      bson options ;
      BOOLEAN optionsInited = FALSE ;

      if ( _enableTransaction )
      {
         // set TransMaxLockNum to batch size, so parallel transactions won't
         // trigger lock escalation to block each other
         // reserved 10% more in advance
         INT32 lockMoreSize = _batchSize * 10 / 100 ;
         INT32 lockSize = _batchSize + lockMoreSize > 100 ? lockMoreSize : 100 ;

         bson_init( &options ) ;
         optionsInited = TRUE ;

         rc = bson_append_int( &options,
                               FIELD_NAME_TRANS_MAXLOCKNUM,
                               lockSize ) ;
         if ( rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }

         rc = bson_finish( &options ) ;
         if ( rc )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }

         rc = sdbSetSessionAttr( _collection, &options ) ;
         if ( rc )
         {
            if ( SDB_INVALIDARG == rc )
            {
               PD_LOG( PDWARNING, "Server can not recognize field [%s], "
                       "it is an old version server, ignore the field",
                       FIELD_NAME_TRANS_MAXLOCKNUM ) ;
               rc = SDB_OK ;
            }
            goto error ;
         }
      }

   done:
      if ( optionsInited )
      {
         bson_destroy( &options ) ;
      }
      return rc ;
   error:
      goto done ;
   }

} // namespace
