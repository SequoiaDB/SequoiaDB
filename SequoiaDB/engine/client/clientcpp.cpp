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

   Source File Name = clientcpp.cpp

   Descriptive Name = C++ Client Driver

   When/how to use: this program may be used on binary and text-formatted
   versions of C++ Client component. This file contains functions for
   client driver.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  TW  Initial Draft

   Last Changed =

*******************************************************************************/
#include "clientImpl.hpp"
#include "common.h"
#include "ossMem.hpp"
#include "msgCatalogDef.h"
#include "msgDef.h"
#include "pmdOptions.h"
#include "pd.hpp"
#include "fmpDef.hpp"
#include "../bson/lib/md5.hpp"
#include "utilCipher.hpp"
#include "../bson/lib/base64.h"
#include <string>
#include <vector>
#ifdef SDB_SSL
#include "ossSSLWrapper.h"
#endif
#include "msg.hpp"

using namespace std ;
using namespace bson ;

#define SDB_MD5_VALUE_BUF_LEN  (SDB_MD5_DIGEST_LENGTH * 2 + 1)
#define LOB_ALIGNED_LEN 524288
#define CLIENT_SQL_MAX_LEN 127
#define MAX_USERNAME_LENGTH 256
#define MAX_PASSWORD_LENGTH 256

namespace sdbclient
{
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

   static ERROR_ON_REPLY_FUNC _sdbErrorOnReplyCallback = NULL ;
   static BOOLEAN _sdbIsSrand = FALSE ;
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
         _sdbSrand () ;
#if defined (_WINDOWS)
      rand_s ( &randVal ) ;
#elif defined (_LINUX) || defined (_AIX)
      randVal = rand_r ( &_sdbRandSeed ) ;
#endif
      return randVal ;
   }

#define CHECK_RET_MSGHEADER( pSendBuf, pRecvBuf, pConnect )   \
do                                                            \
{                                                             \
   rc = clientCheckRetMsgHeader( pSendBuf, pRecvBuf,          \
                                 pConnect->_endianConvert ) ; \
   if ( SDB_OK != rc )                                        \
   {                                                          \
      if ( SDB_UNEXPECTED_RESULT == rc )                      \
      {                                                       \
         pConnect->disconnect() ;                             \
      }                                                       \
      goto error ;                                            \
   }                                                          \
}while( FALSE )

   static INT32 clientSocketSend ( ossSocket *pSock,
                                   CHAR *pBuffer,
                                   INT32 sendSize )
   {
      INT32 rc = SDB_OK ;
      INT32 sentSize = 0 ;
      INT32 totalSentSize = 0 ;
      while ( sendSize > totalSentSize )
      {
         rc = pSock->send ( &pBuffer[totalSentSize],
                            sendSize - totalSentSize,
                            sentSize,
                            SDB_CLIENT_SOCKET_TIMEOUT_DFT ) ;
         totalSentSize += sentSize ;
         if ( SDB_TIMEOUT == rc )
            continue ;
         if ( rc  )
            goto done ;
      }
   done :
      return rc ;
   }

   static INT32 clientSocketRecv ( ossSocket *pSock,
                                   CHAR *pBuffer,
                                   INT32 receiveSize )
   {
      INT32 rc = SDB_OK ;
      INT32 receivedSize = 0 ;
      INT32 totalReceivedSize = 0 ;
      while ( receiveSize > totalReceivedSize )
      {
         rc = pSock->recv ( &pBuffer[totalReceivedSize],
                            receiveSize - totalReceivedSize,
                            receivedSize,
                            SDB_CLIENT_SOCKET_TIMEOUT_DFT ) ;
         totalReceivedSize += receivedSize ;
         if ( SDB_TIMEOUT == rc )
            continue ;
         if ( rc )
            goto done ;
      }
   done :
      return rc ;
   }

   static INT32 _sdbBuildAlterObject ( const CHAR * taskName,
                                       const BSONObj * options,
                                       BOOLEAN allowNullArgs,
                                       BSONObj & alterObject )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         BSONObjBuilder alterBuilder( builder.subobjStart( FIELD_NAME_ALTER ) ) ;

         alterBuilder.append( FIELD_NAME_NAME, taskName ) ;

         if ( NULL == options )
         {
            if ( allowNullArgs )
            {
               alterBuilder.appendNull( FIELD_NAME_ARGS ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            alterBuilder.append( FIELD_NAME_ARGS, *options ) ;
         }

         alterBuilder.done() ;
         alterObject = builder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   static INT32 extractErrorObj( const CHAR *pErrorBuf,
                                  INT32 *pFlag,
                                  const CHAR **ppErr,
                                  const CHAR **ppDetail )
   {
      INT32 rc = SDB_OK ;
      BSONObj localObj ;

      if ( !pErrorBuf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         localObj.init( pErrorBuf ) ;
      }
      catch( std::exception )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }

      {
         BSONElement errnoEle = localObj.getField( OP_ERRNOFIELD ) ;
         if ( pFlag && NumberInt == errnoEle.type() )
         {
            *pFlag = errnoEle.Int() ;
         }
      }
      {
         BSONElement descEle = localObj.getField( OP_ERRDESP_FIELD ) ;
         if ( ppErr && String == descEle.type() )
         {
            *ppErr = descEle.valuestr() ;
         }
      }
      {
         BSONElement detailEle = localObj.getField( OP_ERR_DETAIL ) ;
         if ( ppDetail && String == detailEle.type() )
         {
            *ppDetail = detailEle.valuestr() ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   SDB_EXPORT void sdbSetErrorOnReplyCallback( ERROR_ON_REPLY_FUNC func )
   {
      _sdbErrorOnReplyCallback = func ;
   }

   /*
    * _sdbBase
    */
   _sdbBase::_sdbBase (CLIENT_CLASS_TYPE type) :
   _type( type ),
   _connection( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 )
   {}

   INT32 _sdbBase::_regHandle( _sdbImpl *connection, ossValuePtr ptr )
   {
      INT32 rc = SDB_OK ;
      if ( connection )
      {
         _connection = connection ;
         rc = _connection->_registerHandle( _type, ptr ) ;
      }
      else
      {
         rc = SDB_INVALIDARG ;
      }
      return rc ;
   }

   void _sdbBase::_unregHandle( ossValuePtr ptr )
   {
      // when we coredump from here, please check wether _connection
      // had been destroyed or not
      if ( _connection )
      {
         _resetErrorAndResultBuf();
         _connection->_unregisterHandle( _type, ptr ) ;
         _connection = NULL ;
      }
   }

   void _sdbBase::_resetErrorAndResultBuf()
   {
      if ( !_connection )
      {
         return ;
      }
      const CHAR *pErrorBuf  = _connection->_pErrorBuf ;
      const CHAR *pResultBuf = _connection->_pResultBuf ;

      if ( pErrorBuf &&
           ( pErrorBuf >= _pReceiveBuffer &&
             pErrorBuf < ( _pReceiveBuffer + _receiveBufferSize ) ) )
      {
         _connection->_setErrorBuffer( NULL, 0 ) ;
      }
      if ( pResultBuf &&
           ( pResultBuf >= _pReceiveBuffer &&
             pResultBuf < ( _pReceiveBuffer + _receiveBufferSize ) ) )
      {
         _connection->_setResultBuffer( NULL, 0 ) ;
      }
   }

   /*
    * sdbCursorImpl
    * Cursor Implementation
    */
   _sdbCursorImpl::_sdbCursorImpl () :
   _sdbBase( CLIENT_CLASS_CURSOR ),
   _contextID ( -1 ),
   _isClosed ( FALSE ),
   _totalRead ( 0 ),
   _offset ( -1 )
   {
   }

   _sdbCursorImpl::~_sdbCursorImpl ()
   {
      if ( _connection )
      {
         if ( !_isClosed )
         {
            if ( -1 != _contextID )
            {
               _killCursor () ;
            }
         }
         // unregister the cursor anyway
         _dropConnection() ;
      }
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
   }


   void _sdbCursorImpl::_close()
   {
      _isClosed   = TRUE ;
      _contextID  = -1 ;
      _offset     = -1 ;
   }

   INT32 _sdbCursorImpl::_killCursor ()
   {
      INT32 rc         = SDB_OK ;

      // check
      if ( -1 == _contextID || !_connection )
      {
         goto done ;
      }

      // build msg
      rc = clientBuildKillContextsMsg ( &_pSendBuffer, &_sendBufferSize, 0,
                                        1, &_contextID,
                                        _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer,
                                      &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::advance( const BSONObj &option, BSONObj *pResult )
   {
      INT32 rc                = SDB_OK ;
      CHAR *pReceiveBuffer    = NULL ;
      INT32 receiveBufferSize = 0 ;

      const CHAR *pBackData   = NULL ;
      INT32 backDataSize      = 0 ;

      // if contextid is not invalid
      if ( -1 == _contextID )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      // check
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( _pReceiveBuffer )
      {
         INT32 tmpOffset = _offset ;
         MsgOpReply *pReply = (MsgOpReply*)_pReceiveBuffer ;

         if ( -1 == tmpOffset )
         {
            tmpOffset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
         }
         else
         {
            // otherwise let's skip the current one
            tmpOffset += ossRoundUpToMultipleX ( *(INT32*)&_pReceiveBuffer[tmpOffset],
                                                 4 ) ;
         }

         if ( tmpOffset < pReply->header.messageLength )
         {
            pBackData = &_pReceiveBuffer[tmpOffset] ;
            backDataSize = pReply->header.messageLength - tmpOffset ;
         }
      }

      // build msg
      rc = clientBuildAdvanceMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     _contextID, 0, option.objdata(),
                                     pBackData, backDataSize,
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer,
                                      &pReceiveBuffer,
                                      &receiveBufferSize ) ;
      if ( rc )
      {
         if ( pResult )
         {
            _connection->getLastErrorObj( *pResult ) ;
            try
            {
               *pResult = pResult->getOwned() ;
            }
            catch( ... )
            {
               /// ignore error
            }
            /// reset result
            _connection->cleanLastErrorObj() ;
         }
         goto error ;
      }

      /// clear buffer
      if ( _pReceiveBuffer )
      {
         MsgOpReply *pReply = (MsgOpReply*)_pReceiveBuffer ;
         pReply->header.messageLength = sizeof( MsgOpReply ) ;
      }

   done :
      if ( pReceiveBuffer )
      {
         SDB_OSS_FREE( pReceiveBuffer ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::_readNextBuffer ()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked  = FALSE ;
      SINT64 contextID = 0 ;

      // if contextid is not invalid
      if ( -1 == _contextID )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      // check
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      // build msg
      rc = clientBuildGetMoreMsg ( &_pSendBuffer, &_sendBufferSize, -1,
                                   _contextID, 0, _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      // receive from engine
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK == rc )
      {
         // check return msg header
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      }
      if ( SDB_DMS_EOC == rc )
      {
         // check return opcode
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
         rc = SDB_DMS_EOC ;
      }
      // when engine return 0 != replyFlag, the context has been kill,
      // just set _contextID to -1 to avoid send kill context to engine
      if ( SDB_OK != rc )
      {
         MsgOpReply *pReply = (MsgOpReply*)_pReceiveBuffer ;
         if ( !pReply )
         {
            goto error ;
         }
         if ( SDB_OK != pReply->flags )
         {
            _contextID = -1 ;
         }
         goto error ;
      }
      else if ( -1 == contextID )
      {
         // early closed
         _contextID = -1 ;
      }

   done :
      if ( locked )
      {
         _connection->unlock () ;
      }
      return rc ;
   error :
      if ( SDB_DMS_EOC != rc )
      {
         _killCursor() ;
      }
      // clean up
      _close() ;
      // unregister
      _dropConnection() ;
      goto done ;
   }

   INT32 _sdbCursorImpl::next ( BSONObj &obj, BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pReply = NULL ;

      // check wether the cursor had been close or not
      if ( _isClosed )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }

      // begin to get next record
      if ( !_pReceiveBuffer && _connection )
      {
         if ( -1 == _offset )
         {
            rc = _readNextBuffer () ;
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }

   retry :
      pReply = (MsgOpReply*)_pReceiveBuffer ;
      if ( !pReply )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // let it jump to next record
      if ( -1 == _offset )
      {
         // if it's the first time we fetch record, let's place offset to very
         // beginning of first record
         _offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
      }
      else
      {
         // otherwise let's skip the current one
         _offset += ossRoundUpToMultipleX ( *(INT32*)&_pReceiveBuffer[_offset],
                                            4 ) ;
      }
      // let's see if we have jump out of bound
      if ( _offset >= pReply->header.messageLength ||
           _offset >= _receiveBufferSize )
      {
         if ( _connection )
         {
            _offset = -1 ;
            rc = _readNextBuffer () ;
            if ( rc )
            {
               goto error ;
            }
            goto retry ;
         }
         else
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }
      // then let's read the object
      obj.init ( &_pReceiveBuffer [ _offset ] ) ;

      if ( TRUE == getOwned )
      {
          obj = obj.copy();
      }
      ++_totalRead ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::current ( BSONObj &obj, BOOLEAN getOwned )
   {
      INT32 rc = SDB_OK ;

      if ( _isClosed )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }

      if ( -1 == _offset )
      {
         rc = next( obj, getOwned ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         if ( !_pReceiveBuffer )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         obj.init ( &_pReceiveBuffer [ _offset ] ) ;

         if ( TRUE == getOwned )
         {
            obj = obj.copy() ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::close()
   {
      INT32 rc = SDB_OK ;

      // check wether the cursor had been close or not
      if ( _isClosed )
      {
         goto done ;
      }
      // when _contextID is -1, the context in engine has been delete,
      // so, no need to send kill context message.
      if ( -1 == _contextID )
      {
         _close() ;
         goto done ;
      }
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      // kill context
      rc = _killCursor() ;
      if ( rc )
      {
         goto error ;
      }

      _close() ;

   done :
      if ( SDB_OK == rc )
      {
         // unregister anyway
         _dropConnection() ;
      }
      return rc ;
   error :
      goto done ;
   }

   /*
    * sdbCollectionImpl
    * Collection Implementation
    */
   _sdbCollectionImpl::_sdbCollectionImpl () :
   _sdbBase( CLIENT_CLASS_CL ),
   _pAppendOIDBuffer ( NULL ),
   _appendOIDBufferSize ( 0 )
   {
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) ) ;
      ossMemset ( _collectionName, 0, sizeof ( _collectionName ) ) ;
      ossMemset ( _collectionFullName, 0, sizeof ( _collectionFullName ) ) ;
   }

   _sdbCollectionImpl::_sdbCollectionImpl ( CHAR *pCollectionFullName ) :
   _sdbBase( CLIENT_CLASS_CL ),
   _pAppendOIDBuffer ( NULL ),
   _appendOIDBufferSize ( 0 ),
   _version ( CATALOG_DEFAULT_VERSION )
   {
      _setName ( pCollectionFullName ) ;
   }

   _sdbCollectionImpl::_sdbCollectionImpl ( CHAR *pCollectionSpaceName,
                                            CHAR *pCollectionName ) :
   _sdbBase( CLIENT_CLASS_CL ),
   _pAppendOIDBuffer ( NULL ),
   _appendOIDBufferSize ( 0 )
   {
      INT32 collectionSpaceLen = ossStrlen ( pCollectionSpaceName ) ;
      INT32 collectionLen      = ossStrlen ( pCollectionName ) ;
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) );
      ossMemset ( _collectionName, 0, sizeof ( _collectionName ) ) ;
      ossMemset ( _collectionFullName, 0, sizeof ( _collectionFullName ) ) ;
      if ( collectionSpaceLen <= CLIENT_CS_NAMESZ &&
           collectionLen <= CLIENT_COLLECTION_NAMESZ )
      {
         ossStrncpy ( _collectionSpaceName, pCollectionSpaceName,
                      collectionSpaceLen ) ;
         ossStrncpy ( _collectionName, pCollectionName, collectionLen ) ;
         ossStrncpy ( _collectionFullName, pCollectionSpaceName,
                      collectionSpaceLen ) ;
         ossStrncat ( _collectionFullName, ".", 1 ) ;
         ossStrncat ( _collectionFullName, pCollectionName, collectionLen ) ;
      }
   }

   INT32 _sdbCollectionImpl::_setName ( const CHAR *pCollectionFullName )
   {
      INT32 rc                 = SDB_OK ;
      CHAR *pDot               = NULL ;
      CHAR *pDot1              = NULL ;
      CHAR collectionFullName[ CLIENT_CL_FULLNAME_SZ + 1 ] = { 0 } ;

      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) );
      ossMemset ( _collectionName, 0, sizeof ( _collectionName ) ) ;
      ossMemset ( _collectionFullName, 0, sizeof ( _collectionFullName ) ) ;

      if ( !pCollectionFullName ||
           ossStrlen ( pCollectionFullName ) > CLIENT_CL_FULLNAME_SZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossStrncpy ( collectionFullName, pCollectionFullName,
                   sizeof( collectionFullName ) - 1 ) ;
      pDot = (CHAR*)ossStrchr ( (CHAR*)collectionFullName, '.' ) ;
      pDot1 = (CHAR*)ossStrrchr ( (CHAR*)collectionFullName, '.' ) ;
      if ( !pDot || (pDot != pDot1) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *pDot = 0 ;
      ++pDot ;

      if ( ossStrlen ( collectionFullName ) <= CLIENT_CS_NAMESZ &&
           ossStrlen ( pDot ) <= CLIENT_COLLECTION_NAMESZ )
      {
         ossStrncpy( _collectionSpaceName, collectionFullName,
                     CLIENT_CS_NAMESZ ) ;
         ossStrncpy( _collectionName, pDot, CLIENT_COLLECTION_NAMESZ ) ;
         ossStrncpy( _collectionFullName, pCollectionFullName,
                     sizeof( _collectionFullName ) - 1 ) ;
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

   _sdbCollectionImpl::~_sdbCollectionImpl ()
   {
      _dropConnection() ;
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
      if ( _pAppendOIDBuffer )
      {
         SDB_OSS_FREE ( _pAppendOIDBuffer ) ;
      }
   }

#pragma pack(1)
   class _IDToInsert
   {
   public :
      CHAR _type ;
      CHAR _id[4] ; // _id + '\0'
      OID _oid ;
      _IDToInsert ()
      {
         _type = (CHAR)jstOID ;
         _id[0] = '_' ;
         _id[1] = 'i' ;
         _id[2] = 'd' ;
         _id[3] = 0 ;
         SDB_ASSERT ( sizeof ( _IDToInsert) == 17,
                      "IDToInsert should be 17 bytes" ) ;
      }
   } ;

   typedef class _IDToInsert _IDToInsert ;
   class _idToInsert : public BSONElement
   {
   public :
      _idToInsert( CHAR* x ) : BSONElement((CHAR*) ( x )){}
   } ;

   typedef class _idToInsert _idToInsert ;

#pragma pack()

   INT32 _sdbCollectionImpl::_appendOID ( const BSONObj &input, BSONObj &output )
   {
      INT32 rc = SDB_OK ;
      _IDToInsert oid ;
      _idToInsert oidEle((CHAR*)(&oid)) ;
      INT32 oidLen = 0 ;

      // if it already has _id, let's jump to done
      if ( !input.getField( CLIENT_RECORD_ID_FIELD ).eoo() )
      {
         output = input ;
         goto done ;
      }
      oid._oid.init() ;
      oidLen = oidEle.size() ;
      // let's make sure the buffer length is sufficient
      if ( _appendOIDBufferSize < input.objsize() + oidLen )
      {
         CHAR *pOld = _pAppendOIDBuffer ;
         INT32 newSize = ossRoundUpToMultipleX ( input.objsize() +
                                                 oidLen,
                                                 SDB_PAGE_SIZE ) ;
         if ( newSize < 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _pAppendOIDBuffer = (CHAR*)SDB_OSS_REALLOC( _pAppendOIDBuffer,
                                                     sizeof(CHAR)*newSize) ;
         if ( !_pAppendOIDBuffer )
         {
            rc = SDB_OOM ;
            _pAppendOIDBuffer = pOld ;
            goto error ;
         }
         _appendOIDBufferSize = newSize ;
      }
      *(INT32*)(_pAppendOIDBuffer) = input.objsize() + oidLen ;
      ossMemcpy ( &_pAppendOIDBuffer[sizeof(INT32)], oidEle.rawdata(),
                  oidEle.size() ) ;
      ossMemcpy ( &_pAppendOIDBuffer[sizeof(INT32)+oidLen],
                  (CHAR*)(input.objdata()+sizeof(INT32)),
                  input.objsize()-sizeof(INT32) ) ;
      output.init ( _pAppendOIDBuffer ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   void* _sdbCollectionImpl::_getConnection ()
   {
      return _connection ;
   }

   INT32 _sdbCollectionImpl::getCount ( SINT64 &count,
                                        const BSONObj &condition,
                                        const BSONObj &hint )
   {
      INT32 rc            = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj newObj ;
      BSONObj countObj ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      try
      {
         BSONObjBuilder newObjBuilder ;
         newObjBuilder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         if( !hint.isEmpty() )
         {
            newObjBuilder.append( FIELD_NAME_HINT, hint ) ;
         }
         newObj = newObjBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_COUNT,
                                     &condition, NULL, NULL, &newObj,
                                     FLG_QUERY_WITH_RETURNDATA, 0, -1, -1,
                                     &pCursor ) ;
      /// udpate and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // check return cursor
      if ( NULL == pCursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // there should only 1 record read
      rc = pCursor->next ( countObj ) ;
      if ( rc )
      {
         // if we didn't read anything, let's return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
            goto error ;
         }
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TOTAL ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         else
         {
            count = ele.numberLong() ;
         }
      }

   done:
      if ( NULL != pCursor )
      {
         delete( pCursor ) ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::_getRetVersion ()
   {
      INT32 version = CATALOG_INVALID_VERSION ;

      if( NULL != _pReceiveBuffer )
      {
         version = ((MsgOpReply*)_pReceiveBuffer)->startFrom ;
      }
      return version ;
   }

   // dml operation can not call this func
   INT32 _sdbCollectionImpl::_runCommand ( const CHAR *pString,
                                           const BSONObj *arg1,
                                           const BSONObj *arg2,
                                           const BSONObj *arg3,
                                           const BSONObj *arg4,
                                           SINT32 flag,
                                           UINT64 reqID,
                                           SINT64 numToSkip,
                                           SINT64 numToReturn,
                                           _sdbCursor **ppCursor )
   {
      INT32 rc = SDB_OK ;
      INT32 version = CATALOG_INVALID_VERSION ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, pString,
                                    flag, reqID, numToSkip, numToReturn,
                                    arg1 ? arg1->objdata() : NULL,
                                    arg2 ? arg2->objdata() : NULL,
                                    arg3 ? arg3->objdata() : NULL,
                                    arg4 ? arg4->objdata() : NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize,
                                      ppCursor, FALSE ) ;

      version = _getRetVersion() ;

      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, version) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // DDL operatrion if return SDB_OK and version is valid then need to set to collection
      if( CATALOG_INVALID_VERSION != version)
      {
         setVersion( version );
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::_insert ( const BSONObj &obj,
                                       const BSONObj &hint,
                                       INT32 flags,
                                       BSONObj &newObj,
                                       BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;

      // make sure the object is initialized
      if ( _collectionFullName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = _appendOID ( obj, newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      // Inform other nodes that the '_id' field is included in the record.
      OSS_BIT_SET( flags, FLG_INSERT_HAS_ID_FIELD ) ;

      // Check strictly that the hint is used with FLG_INSERT_HASHINT together.
      if ( hint.isEmpty() )
      {
         OSS_BIT_CLEAR( flags, FLG_INSERT_HASHINT ) ;
      }
      else
      {
         OSS_BIT_SET( flags, FLG_INSERT_HASHINT ) ;
      }

      OSS_BIT_SET( flags, FLG_INSERT_RETURNNUM ) ;

      rc = clientBuildInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, flags, 0,
                                     newObj.objdata(),
                                     hint.isEmpty() ? NULL : hint.objdata(),
                                     _connection->_endianConvert ) ;

      if ( rc )
      {
         goto error ;
      }

      ossEndianConvertIf ( _version,((MsgOpInsert*)_pSendBuffer)->version,
                           _connection->_endianConvert ) ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      if ( pResult )
      {
         _connection->getLastResultObj( *pResult, FALSE ) ;
      }

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update, and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( const BSONObj &obj, OID *id )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( NULL == id )
      {
         rc = _insert( obj, _sdbStaticObject, 0, newObj ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         rc = _insert( obj, _sdbStaticObject, FLG_INSERT_RETURN_OID, newObj ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( jstOID == newObj.getField( CLIENT_RECORD_ID_FIELD ).type() )
         {
            *id = newObj.getField ( CLIENT_RECORD_ID_FIELD ).__oid();
         }
         else
         {
            id->clear() ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( const bson::BSONObj &obj,
                                      INT32 flags,
                                      bson::BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;
      rc = insert( obj, _sdbStaticObject, flags, pResult ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( const bson::BSONObj &obj,
                                      const bson::BSONObj &hint,
                                      INT32 flags,
                                      bson::BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      rc = _insert( obj, hint, flags, newObj, pResult ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
      {
         BSONObjBuilder bob ;
         bob.append( newObj.getField ( CLIENT_RECORD_ID_FIELD ) ) ;
         bob.appendElements( *pResult ) ;
         *pResult = bob.obj() ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( std::vector<bson::BSONObj> &objs,
                                      INT32 flags,
                                      bson::BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;
      rc = insert( objs, _sdbStaticObject, flags, pResult ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( std::vector<bson::BSONObj> &objs,
                                      const bson::BSONObj &hint,
                                      INT32 flags,
                                      bson::BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;
      SINT32 num = objs.size() ;

      BSONObj newObj ;
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( CLIENT_RECORD_ID_FIELD ) ) ;

      if ( _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( num <= 0 )
      {
         // in this case, prevent use '_pSendBuffer' to send anything to engine
         goto done ;
      }

      if ( hint.isEmpty() )
      {
         OSS_BIT_CLEAR( flags, FLG_INSERT_HASHINT ) ;
      }
      else
      {
         OSS_BIT_SET( flags, FLG_INSERT_HASHINT ) ;
      }

      OSS_BIT_SET( flags, FLG_INSERT_RETURNNUM ) ;

      // Inform other nodes that the '_id' field is included in records.
      OSS_BIT_SET( flags, FLG_INSERT_HAS_ID_FIELD ) ;

      for ( SINT32 count = 0 ; count < num ; ++count )
      {
         rc = _appendOID ( objs[count], newObj ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
         {
            try
            {
               sub.append( newObj.getField( CLIENT_RECORD_ID_FIELD ) ) ;
            }
            catch ( std::exception )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }

         if ( 0 == count )
         {
            rc = clientBuildInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                           _collectionFullName, flags, 0,
                                           newObj.objdata(), NULL,
                                           _connection->_endianConvert ) ;
            if ( rc )
            {
               goto error ;
            }
            ossEndianConvertIf ( _version,((MsgOpInsert*)_pSendBuffer)->version,
                                 _connection->_endianConvert ) ;
         }
         else
         {
            rc = clientAppendInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                            newObj.objdata(),
                                            _connection->_endianConvert ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

      // Append hint to the end of the message.
      if ( !hint.isEmpty() )
      {
         rc = clientAppendHint2InsertMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                             hint.objdata(),
                                             _connection->_endianConvert ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      sub.done() ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      if ( pResult )
      {
         _connection->getLastResultObj( *pResult, FALSE ) ;
      }

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // return output object
      if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
      {
         builder.appendElements( *pResult ) ;
         *pResult = builder.obj() ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( const bson::BSONObj objs[],
                                      INT32 size,
                                      INT32 flags,
                                      bson::BSONObj *pResult )
   {
      INT32 rc         = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder builder ;
      BSONArrayBuilder sub( builder.subarrayStart( CLIENT_RECORD_ID_FIELD ) ) ;

      if ( _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( size < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      else if ( 0 == size )
      {
         goto done ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      OSS_BIT_CLEAR( flags, FLG_INSERT_HASHINT ) ;
      OSS_BIT_SET( flags, FLG_INSERT_RETURNNUM );

      for ( SINT32 count = 0; count < size; ++count )
      {
         rc = _appendOID ( objs[count], newObj ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
         {
            try
            {
               sub.append( newObj.getField ( CLIENT_RECORD_ID_FIELD ) ) ;
            }
            catch ( std::exception )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
         if ( 0 == count )
         {
            rc = clientBuildInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                           _collectionFullName, flags, 0,
                                           newObj.objdata(), NULL,
                                           _connection->_endianConvert ) ;
            if ( rc )
            {
                goto error ;
            }

            ossEndianConvertIf ( _version,((MsgOpInsert*)_pSendBuffer)->version,
                                 _connection->_endianConvert ) ;
         }
         else
         {
            rc = clientAppendInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                            newObj.objdata(),
                                            _connection->_endianConvert ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

      sub.done() ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      if ( pResult )
      {
         _connection->getLastResultObj( *pResult, FALSE ) ;
      }

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);

      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // return output object
      if ( pResult && ( flags & FLG_INSERT_RETURN_OID ) )
      {
         builder.appendElements( *pResult ) ;
         *pResult = builder.obj() ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::bulkInsert ( SINT32 flags,
                                          vector<BSONObj> &obj )
   {
      INT32 rc = SDB_OK ;
      flags &= ~FLG_INSERT_RETURN_OID ;

      rc = insert( obj, flags, NULL ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::update ( const BSONObj &rule,
                                      const BSONObj &condition,
                                      const BSONObj &hint,
                                      INT32 flag,
                                      BSONObj *pResult )
   {
      return _update ( rule, condition, hint, flag, pResult ) ;
   }

   INT32 _sdbCollectionImpl::upsert ( const BSONObj &rule,
                                      const BSONObj &condition,
                                      const BSONObj &hint,
                                      const BSONObj &setOnInsert,
                                      INT32 flag,
                                      BSONObj *pResult )
   {
      BSONObj newHint ;
      INT32 rc = SDB_OK ;

      try
      {
         if ( !setOnInsert.isEmpty() )
         {
            BSONObjBuilder newHintBuilder ;

            if ( !hint.isEmpty() )
            {
               newHintBuilder.appendElements( hint ) ;
            }
            newHintBuilder.append( FIELD_NAME_SET_ON_INSERT, setOnInsert ) ;
            newHint = newHintBuilder.obj() ;
         }
         else
         {
            newHint = hint ;
         }
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _update ( rule, condition, newHint, flag | FLG_UPDATE_UPSERT,
                     pResult ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_update ( const BSONObj &rule,
                                       const BSONObj &condition,
                                       const BSONObj &hint,
                                       INT32 flag,
                                       BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;

      if ( _collectionFullName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      flag |= FLG_UPDATE_RETURNNUM ;

      rc = clientBuildUpdateMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, flag, 0,
                                     condition.objdata(),
                                     rule.objdata(),
                                     hint.objdata(),
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      ossEndianConvertIf ( _version,((MsgOpUpdate*)_pSendBuffer)->version,
                          _connection->_endianConvert ) ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      /// get result
      if ( pResult )
      {
         _connection->getLastResultObj( *pResult, FALSE ) ;
      }

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);
      if ( SDB_OK != rc )

      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::del ( const BSONObj &condition,
                                   const BSONObj &hint,
                                   INT32 flag,
                                   BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;

      if ( _collectionFullName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
     if ( !_connection )
     {
        rc = SDB_NOT_CONNECTED ;
        goto error ;
     }

     flag |= FLG_DELETE_RETURNNUM ;

      rc = clientBuildDeleteMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, flag, 0,
                                     condition.objdata(),
                                     hint.objdata(),
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      ossEndianConvertIf ( _version, ((MsgOpDelete*)_pSendBuffer)->version,
                          _connection->_endianConvert ) ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      if ( pResult )
      {
         _connection->getLastResultObj( *pResult, FALSE ) ;
      }

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::pop( const BSONObj &option )
   {
      INT32 rc = SDB_OK ;
      BSONElement lidEle ;
      BSONObj cmdOption ;
      BSONObjBuilder builder ;

      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( option.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // Append all the options except the collection name, if any.
         // We should use the collection by which this function is called.
         builder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         BSONObjIterator itr( option ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( FIELD_NAME_COLLECTION, ele.fieldName() ) )
            {
               continue ;
            }

            builder.append( ele ) ;
         }

         cmdOption = builder.done() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_POP,
                                     &cmdOption ) ;
      /// ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_query ( _sdbCursor **cursor,
                                      const BSONObj &condition,
                                      const BSONObj &selected,
                                      const BSONObj &orderBy,
                                      const BSONObj &hint,
                                      INT64 numToSkip,
                                      INT64 numToReturn,
                                      INT32 flags )
   {
      INT32 rc              = SDB_OK ;
      INT32 newFlags        = 0 ;
      _sdbCursor *pCursor   = NULL ;

      // check
      if ( _collectionFullName [0] == '\0' || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newFlags = regulateQueryFlags( flags ) ;

      // try to set flag to be find one
      newFlags |= FLG_QUERY_WITH_RETURNDATA ;
      newFlags |= QUERY_PREPARE_MORE ;
      newFlags |= FLG_QUERY_CLOSE_EOF_CTX ;

      // run command
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, _collectionFullName,
                                    newFlags, 0, numToSkip, numToReturn,
                                    condition.objdata(),
                                    selected.objdata(),
                                    orderBy.objdata(),
                                    hint.objdata(),
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      ossEndianConvertIf ( _version, ((MsgOpQuery*)_pSendBuffer)->version,
                           _connection->_endianConvert ) ;

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize,
                                      &pCursor, FALSE ) ;

      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update, and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);

      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // return cursor
      *cursor = pCursor ;
   done :
      return rc ;
   error :
      if ( NULL != pCursor )
      {
         delete pCursor ;
         pCursor = NULL ;
      }
      goto done;
   }

   INT32 _sdbCollectionImpl::query ( _sdbCursor **cursor,
                                     const BSONObj &condition,
                                     const BSONObj &selected,
                                     const BSONObj &orderBy,
                                     const BSONObj &hint,
                                     INT64 numToSkip,
                                     INT64 numToReturn,
                                     INT32 flag )
   {
      // remove "query plan" and "query and modify" flag
      if ( 0 != flag )
      {
         flag = eraseSingleFlag( flag, FLG_QUERY_EXPLAIN ) ;
         flag = eraseSingleFlag( flag, FLG_QUERY_MODIFY ) ;
      }
      return _query( cursor, condition, selected, orderBy, hint,
                     numToSkip, numToReturn, flag ) ;
   }

   INT32 _sdbCollectionImpl::queryOne( bson::BSONObj &obj,
                                       const bson::BSONObj &condition,
                                       const bson::BSONObj &selected,
                                       const bson::BSONObj &orderBy,
                                       const bson::BSONObj &hint,
                                       INT64 numToSkip,
                                       INT32 flag )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      flag |= FLG_QUERY_WITH_RETURNDATA ;
      rc = query( cursor,
                  condition,
                  selected,
                  orderBy,
                  hint,
                  numToSkip,
                  1,
                  flag ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = cursor.next( obj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_queryAndModify  ( _sdbCursor **cursor,
                                                const BSONObj &condition,
                                                const BSONObj &selected,
                                                const BSONObj &orderBy,
                                                const BSONObj &hint,
                                                const BSONObj &update,
                                                INT64 numToSkip,
                                                INT64 numToReturn,
                                                INT32 flag,
                                                BOOLEAN isUpdate,
                                                BOOLEAN returnNew )
   {
      INT32 rc = SDB_OK ;
      BSONObj newHint ;

      try
      {
         BSONObjBuilder newHintBuilder ;
         BSONObjBuilder modifyBuilder ;
         BSONObj modify ;

         // create $Modify
         if ( isUpdate )
         {
            if ( update.isEmpty() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            modifyBuilder.append( FIELD_NAME_OP, FIELD_OP_VALUE_UPDATE ) ;
            modifyBuilder.appendObject( FIELD_NAME_OP_UPDATE, update.objdata() ) ;
            modifyBuilder.appendBool( FIELD_NAME_RETURNNEW, returnNew ) ;
         }
         else
         {
            modifyBuilder.append( FIELD_NAME_OP, FIELD_OP_VALUE_REMOVE ) ;
            modifyBuilder.appendBool( FIELD_NAME_OP_REMOVE, TRUE ) ;
         }
         modify = modifyBuilder.obj() ;

         // create new hint with $Modify
         if ( !hint.isEmpty() )
         {
            newHintBuilder.appendElements( hint ) ;
         }
         newHintBuilder.appendObject( FIELD_NAME_MODIFY, modify.objdata() ) ;
         newHint = newHintBuilder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      if ( 0 != flag )
      {
         flag = eraseSingleFlag( flag, FLG_QUERY_EXPLAIN ) ;
      }
      flag |= FLG_QUERY_MODIFY ;
      rc = _query( cursor, condition, selected, orderBy, newHint,
                   numToSkip, numToReturn, flag ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::getQueryMeta ( _sdbCursor **cursor,
                                            const BSONObj &condition,
                                            const BSONObj &orderBy,
                                            const BSONObj &hint,
                                            INT64 numToSkip,
                                            INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bo ;
      BSONObj newHint ;

      if ( _collectionFullName [0] == '\0' || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      bo.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
      if ( hint.isEmpty() )
      {
         BSONObj emptyObj ;
         bo.append( FIELD_NAME_HINT, emptyObj ) ;
      }
      else
      {
         bo.append( FIELD_NAME_HINT, hint ) ;
      }
      newHint = bo.obj() ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_QUERYMETA,
                                     &condition, NULL, &orderBy, &newHint,
                                     0, 0, numToSkip, numToReturn,
                                     cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::_createIndex ( const BSONObj &indexDef,
                                            const CHAR *pIndexName,
                                            BOOLEAN isUnique,
                                            BOOLEAN isEnforced,
                                            INT32 sortBufferSize,
                                            SINT64 *pTaskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher, hint, indexObj, countObj ;
      // this cursor no need to register to collection, for it only
      // use temporarily
      _sdbCursor *cursor = NULL ;

      if ( _collectionFullName [0] == '\0' ||
           !pIndexName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( sortBufferSize < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }


      indexObj = BSON( IXM_FIELD_NAME_KEY << indexDef <<
                       IXM_FIELD_NAME_NAME << pIndexName <<
                       IXM_FIELD_NAME_UNIQUE << (isUnique ? true : false) <<
                       IXM_FIELD_NAME_ENFORCED << (isEnforced ? true : false)
                     ) ;

      matcher = BSON( FIELD_NAME_COLLECTION << _collectionFullName <<
                      FIELD_NAME_INDEX << indexObj <<
                      IXM_FIELD_NAME_SORT_BUFFER_SIZE << sortBufferSize <<
                      FIELD_NAME_ASYNC << ( pTaskID ? true : false ) ) ;

      // For Compatibility with older engine( version <3.4 ), keep sort buffer
      // size in hint. After several versions, we can delete it.
      hint = BSON( IXM_FIELD_NAME_SORT_BUFFER_SIZE << sortBufferSize ) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX,
                                     &matcher, NULL, NULL, &hint,
                                     0, 0, 0, -1, &cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get task ID
      if ( NULL == pTaskID )
      {
         goto done ;
      }

      if ( NULL == cursor )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }

      rc = cursor->next( countObj ) ; // there should only 1 record read
      if ( rc )
      {
         // if we didn't read anything, let't return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         goto error ;
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
            goto error ;
         }
         *pTaskID = ele.numberLong () ;
      }

   done :
      if ( NULL != cursor )
      {
         delete cursor ;
         cursor = NULL ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::_createIndex ( const BSONObj &indexDef,
                                            const CHAR *pIndexName,
                                            const BSONObj &indexAttr,
                                            const BSONObj &option,
                                            SINT64 *pTaskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj matcher, hint, countObj ;
      BSONObjBuilder indexBuild, matchBuilder, hintBuilder ;
      BOOLEAN foundOutSort = FALSE ;
      // no need to register in collection
      _sdbCursor *cursor = NULL ;

      if ( _collectionFullName [0] == '\0' ||
           !pIndexName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // for example, build below message:
      // macher: { Collection: "foo.bar",
      //           Index:{ key: {a:1}, name: 'aIdx', Unique: true,
      //                   Enforced: true, NotNull: true },
      //           SortBufferSize: 1024, Async: true }
      // hint: { SortBufferSize: 1024 }
      // For Compatibility with older engine( version <3.4 ), keep sort buffer
      // size in hint. After several versions, we can delete it.

      indexBuild.append( IXM_FIELD_NAME_KEY, indexDef ) ;
      indexBuild.append( IXM_FIELD_NAME_NAME, pIndexName ) ;

      {
         BSONObjIterator it( indexAttr ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 == ossStrcmp( e.fieldName(),
                                 IXM_FIELD_NAME_SORT_BUFFER_SIZE ) )
            {
               foundOutSort = TRUE ;
               matchBuilder.append( e ) ;
               hintBuilder.append( e ) ;
            }
            else
            {
               indexBuild.append( e ) ;
            }
         }
      }
      if ( !foundOutSort )
      {
         BSONElement e = option.getField( IXM_FIELD_NAME_SORT_BUFFER_SIZE ) ;
         if ( !e.eoo() )
         {
            hintBuilder.append( e ) ;
         }
      }

      matchBuilder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
      matchBuilder.append( FIELD_NAME_INDEX, indexBuild.obj() ) ;
      matchBuilder.appendElements( option );
      matchBuilder.append( FIELD_NAME_ASYNC, ( pTaskID ? true : false ) ) ;
      matcher = matchBuilder.obj() ;

      hint = hintBuilder.obj() ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX,
                                     &matcher, NULL, NULL, &hint,
                                     0, 0, 0, -1, &cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get task ID
      if ( NULL == pTaskID )
      {
         goto done ;
      }

      if ( NULL == cursor )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }

      rc = cursor->next( countObj ) ; // there should only 1 record read
      if ( rc )
      {
         // if we didn't read anything, let't return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         goto error ;
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
            goto error ;
         }
         *pTaskID = ele.numberLong () ;
      }

   done :
      if ( NULL != cursor )
      {
         delete cursor ;
         cursor = NULL ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::createIndex ( const BSONObj &indexDef,
                                           const CHAR *pIndexName,
                                           BOOLEAN isUnique,
                                           BOOLEAN isEnforced,
                                           INT32 sortBufferSize )
   {
      return _createIndex ( indexDef, pIndexName, isUnique,
                            isEnforced, sortBufferSize ) ;
   }

   INT32 _sdbCollectionImpl::createIndex ( const BSONObj &indexDef,
                                           const CHAR *pIndexName,
                                           const BSONObj &indexAttr,
                                           const BSONObj &option )
   {
      return _createIndex( indexDef, pIndexName, indexAttr, option ) ;
   }

   INT32 _sdbCollectionImpl::createIndexAsync ( SINT64 &taskID,
                                                const BSONObj &indexDef,
                                                const CHAR *pIndexName,
                                                const BSONObj &indexAttr,
                                                const BSONObj &option )
   {
      return _createIndex( indexDef, pIndexName, indexAttr, option, &taskID ) ;
   }

   INT32 _sdbCollectionImpl::snapshotIndexes ( _sdbCursor **cursor,
                                               const BSONObj &condition,
                                               const BSONObj &selector,
                                               const BSONObj &orderby,
                                               const BSONObj &hint,
                                               INT64 numToSkip,
                                               INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder hintBuilder ;

      if ( _collectionFullName [0] == '\0' || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      {
         BSONObjIterator it( hint ) ;
         while( it.more() )
         {
            BSONElement e = it.next() ;
            if ( 0 != ossStrcmp( e.fieldName(), FIELD_NAME_COLLECTION ) )
            {
               hintBuilder.append( e ) ;
            }
         }
         hintBuilder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
      }

      rc = _connection->getSnapshot( cursor, SDB_SNAP_INDEXES,
                                     condition, selector, orderby,
                                     hintBuilder.done(),
                                     numToSkip, numToReturn ) ;

      /// ignore update result
      updateCachedObject( rc, _connection->_getCachedContainer(),
                          _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::getIndexes ( _sdbCursor **cursor,
                                          const CHAR *pIndexName )
   {
      INT32 rc = SDB_OK ;
      BSONObj queryCond ;
      BSONObj newObj ;

      if ( _collectionFullName [0] == '\0' || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      /* build query condition */
      if ( pIndexName )
      {
         queryCond = BSON ( IXM_FIELD_NAME_INDEX_DEF "." IXM_FIELD_NAME_NAME <<
                            pIndexName ) ;
      }
      /* build collection name */
      newObj = BSON ( FIELD_NAME_COLLECTION << _collectionFullName ) ;
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES,
                                     &queryCond, NULL, NULL, &newObj,
                                     0, 0, 0, -1,
                                     cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::getIndexes ( std::vector<bson::BSONObj> &infos )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;
      BSONObj obj ;

      rc = getIndexes( cursor, NULL ) ;
      if ( rc )
      {
         goto error ;
      }

      infos.clear() ;
      while( SDB_DMS_EOC != ( rc = cursor.next( obj ) ) )
      {
         infos.push_back( obj ) ;
      }
      rc = SDB_OK ;

   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::getIndex ( const CHAR *pIndexName, bson::BSONObj &info )
   {
      INT32 rc = SDB_OK ;
      sdbCursor cursor ;

      if ( !pIndexName || !*pIndexName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = getIndexes( cursor, pIndexName ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cursor.next( info ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_NOTEXIST ;
         goto error ;
      }
      else if ( rc )
      {
         goto error ;
      }

   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_dropIndex ( const CHAR *pIndexName,
                                          SINT64 *pTaskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj countObj, matchObj ;
      _sdbCursor *cursor = NULL ;

      if ( _collectionFullName [0] == '\0' ||
           !pIndexName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      matchObj = BSON( FIELD_NAME_COLLECTION << _collectionFullName <<
                       FIELD_NAME_INDEX << BSON ( "" << pIndexName ) <<
                       FIELD_NAME_ASYNC << ( pTaskID ? true : false ) ) ;

      /// send message
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                                     &matchObj, NULL, NULL, NULL,
                                     0, 0, 0, -1, &cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get task ID
      if ( !pTaskID )
      {
         goto done ;
      }

      if ( NULL == cursor )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }

      rc = cursor->next( countObj ) ; // there should only 1 record read
      if ( rc )
      {
         // if we didn't read anything, let't return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         goto error ;
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
            goto error ;
         }
         *pTaskID = ele.numberLong() ;
      }

   done :
      if ( NULL != cursor )
      {
         delete cursor ;
         cursor = NULL ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::dropIndex ( const CHAR *pIndexName )
   {
      return _dropIndex( pIndexName ) ;
   }

   INT32 _sdbCollectionImpl::dropIndexAsync ( SINT64 &taskID,
                                              const CHAR *pIndexName )
   {
      return _dropIndex( pIndexName, &taskID ) ;
   }

   INT32 _sdbCollectionImpl::_copyIndex ( const CHAR *subClFullName,
                                          const CHAR *pIndexName,
                                          SINT64 *pTaskID )
   {
      INT32 rc = SDB_OK ;
      BSONObj countObj, matchObj ;
      BSONObjBuilder builder ;
      _sdbCursor *cursor = NULL ;

      if ( _collectionFullName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_NAME, _collectionFullName ) ;
         builder.append( FIELD_NAME_ASYNC, ( pTaskID ? true : false ) ) ;
         if ( subClFullName )
         {
            builder.append( FIELD_NAME_SUBCLNAME, subClFullName ) ;
         }
         if ( pIndexName )
         {
            builder.append( FIELD_NAME_INDEXNAME, pIndexName ) ;
         }
         matchObj = builder.obj() ;

         /// send message
         rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_COPY_INDEX,
                                        &matchObj, NULL, NULL, NULL,
                                        0, 0, 0, -1, &cursor ) ;
         updateCachedObject( rc, _connection->_getCachedContainer(),
                             _collectionFullName ) ; // ignore update result
         if ( SDB_OK != rc )
         {
            goto error ;
         }

         // get task ID
         if ( !pTaskID )
         {
            goto done ;
         }

         rc = cursor->next( countObj ) ; // there should only 1 record read
         if ( rc )
         {
            // if we didn't read anything, let't return unexpected
            if ( SDB_DMS_EOC == rc )
            {
               rc = SDB_UNEXPECTED_RESULT ;
            }
            goto error ;
         }
         else
         {
            BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
            if ( ele.type() != NumberLong )
            {
               rc = SDB_UNEXPECTED_RESULT ;
               goto error ;
            }
            *pTaskID = ele.numberLong() ;
         }

      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         goto error ;
      }

   done :
      if ( NULL != cursor )
      {
         delete cursor ;
         cursor = NULL ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::copyIndex ( const CHAR *subClFullName,
                                         const CHAR *pIndexName )
   {
      return _copyIndex( subClFullName, pIndexName ) ;
   }

   INT32 _sdbCollectionImpl::copyIndexAsync ( SINT64 &taskID,
                                              const CHAR *subClFullName,
                                              const CHAR *pIndexName )
   {
      return _copyIndex( subClFullName, pIndexName, &taskID ) ;
   }

   INT32 _sdbCollectionImpl::create()
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionFullName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( FIELD_NAME_NAME << _collectionFullName ) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                                     &newObj ) ;
      /// ignore update result
      insertCachedObject( _connection->_getCachedContainer(),
                          _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::drop ()
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( FIELD_NAME_NAME << _collectionFullName ) ;
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION,
                                     &newObj ) ;
      /// ignore the result
      removeCachedObject( _connection->_getCachedContainer(),
                          _collectionFullName, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::split ( const CHAR *pSourceGroupName,
                                     const CHAR *pTargetGroupName,
                                     const BSONObj &splitCondition,
                                     const BSONObj &splitEndCondition)
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionFullName [0] == '\0' ||
           !pSourceGroupName || !*pSourceGroupName ||
           !pTargetGroupName || !*pTargetGroupName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( CAT_COLLECTION_NAME << _collectionFullName <<
                      CAT_SOURCE_NAME << pSourceGroupName <<
                      CAT_TARGET_NAME << pTargetGroupName <<
                      CAT_SPLITQUERY_NAME << splitCondition <<
                      CAT_SPLITENDQUERY_NAME << splitEndCondition) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                     &newObj ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::split ( const CHAR *pSourceGroupName,
                                     const CHAR *pTargetGroupName,
                                     double percent )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionFullName [0] == '\0' ||
          !pSourceGroupName || !*pSourceGroupName ||
          !pTargetGroupName || !*pTargetGroupName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( percent <= 0.0 || percent > 100.0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( CAT_COLLECTION_NAME << _collectionFullName <<
                      CAT_SOURCE_NAME << pSourceGroupName <<
                      CAT_TARGET_NAME << pTargetGroupName <<
                      CAT_SPLITPERCENT_NAME << percent ) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                     &newObj ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::splitAsync ( SINT64 &taskID,
                                          const CHAR *pSourceGroupName,
                                          const CHAR *pTargetGroupName,
                                          const bson::BSONObj &splitCondition,
                                          const bson::BSONObj &splitEndCondition )
   {
      INT32 rc                 = SDB_OK ;
      _sdbCursor *cursor       = NULL ;
      BSONObj newObj  ;
      BSONObj countObj ;

      if ( _collectionFullName[0] == '\0' ||
           !pSourceGroupName || !*pSourceGroupName ||
           !pTargetGroupName || !*pTargetGroupName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder bob ;
         bob.append ( CAT_COLLECTION_NAME, _collectionFullName ) ;
         bob.append ( CAT_SOURCE_NAME, pSourceGroupName ) ;
         bob.append ( CAT_TARGET_NAME, pTargetGroupName ) ;
         bob.append ( CAT_SPLITQUERY_NAME, splitCondition ) ;
         bob.append ( CAT_SPLITENDQUERY_NAME, splitEndCondition ) ;
         bob.appendBool ( FIELD_NAME_ASYNC, TRUE ) ;
         newObj = bob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                     &newObj, NULL, NULL, NULL,
                                     0, 0, 0, -1,
                                     &cursor ) ;
      /// ingore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // there should only 1 record read
      rc = cursor->next( countObj ) ;
      if ( rc )
      {
         // if we didn't read anything, let't return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         else
         {
            taskID = ele.numberLong () ;
         }
      }

   done :
      if ( cursor )
      {
         delete cursor ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::splitAsync ( const CHAR *pSourceGroupName,
                                          const CHAR *pTargetGroupName,
                                          FLOAT64 percent,
                                          SINT64 &taskID )
   {
      INT32 rc                 = SDB_OK ;
      _sdbCursor *cursor       = NULL ;
      BSONObj newObj ;
      BSONObj countObj ;

      if ( _collectionFullName [0] == '\0' ||
           !pSourceGroupName || !*pSourceGroupName ||
           !pTargetGroupName || !*pTargetGroupName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( percent <= 0.0 || percent > 100.0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder bob ;
         bob.append ( CAT_COLLECTION_NAME, _collectionFullName ) ;
         bob.append ( CAT_SOURCE_NAME, pSourceGroupName ) ;
         bob.append ( CAT_TARGET_NAME, pTargetGroupName ) ;
         bob.append ( CAT_SPLITPERCENT_NAME, percent ) ;
         bob.appendBool ( FIELD_NAME_ASYNC, TRUE ) ;
         newObj = bob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                     &newObj, NULL, NULL, NULL,
                                     0, 0, 0, -1,
                                     &cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // there should only 1 record read
      rc = cursor->next ( countObj ) ;
      if ( rc )
      {
         // if we didn't read anything, let't return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
      }
      else
      {
         BSONElement ele = countObj.getField ( FIELD_NAME_TASKID ) ;
         if ( ele.type() != NumberLong )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         else
         {
            taskID = ele.numberLong () ;
         }
      }

   done :
      if ( cursor )
      {
         delete cursor ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::aggregate ( _sdbCursor **cursor,
                                         std::vector<bson::BSONObj> &obj )
   {
      INT32 rc = SDB_OK ;
      SINT32 num = obj.size() ;

      if ( _collectionFullName[0] == '\0' || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      for ( SINT32 count = 0; count < num; ++count )
      {
         if ( 0 == count )
         {
            rc = clientBuildAggrRequestCpp( &_pSendBuffer, &_sendBufferSize,
                                            _collectionFullName,
                                            obj[count].objdata(),
                                            _connection->_endianConvert ) ;
            if ( rc )
            {
               goto error ;
            }

            ossEndianConvertIf ( _version,((MsgOpAggregate*)_pSendBuffer)->version,
                                 _connection->_endianConvert ) ;
         }
         else
         {
            rc = clientAppendAggrRequestCpp( &_pSendBuffer, &_sendBufferSize,
                                             obj[count].objdata(),
                                             _connection->_endianConvert ) ;
         }
         if ( rc )
         {
            goto error ;
         }
      }

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize,
                                      cursor ) ;
      if( SDB_CLIENT_CATA_VER_OLD == rc &&
          CATALOG_INVALID_VERSION != _getRetVersion())
      {
         setVersion( _getRetVersion() );
      }
      /// update and ignore the update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version);
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::detachCollection ( const CHAR *subClFullName )
   {
      INT32 rc         = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      // check argument
      if ( !subClFullName ||
            ossStrlen ( subClFullName) > CLIENT_COLLECTION_NAMESZ ||
           _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // main collection fullname is required
      ob.append ( FIELD_NAME_NAME, _collectionFullName ) ;
      // sub collection fullname is required
      ob.append ( FIELD_NAME_SUBCLNAME, subClFullName ) ;
      newObj = ob.obj() ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL,
                        &newObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::attachCollection ( const CHAR *subClFullName,
                                                const bson::BSONObj &options)
   {
      INT32 rc         = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      // check argument
      if ( !subClFullName ||
           ossStrlen ( subClFullName) > CLIENT_COLLECTION_NAMESZ ||
           _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // main collection fullname is required
      ob.append ( FIELD_NAME_NAME, _collectionFullName ) ;
      // sub collection fullname is required
      ob.append ( FIELD_NAME_SUBCLNAME, subClFullName ) ;
      ob.appendElementsUnique( options ) ;
      newObj = ob.obj() ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_LINK_CL,
                        &newObj ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_alterCollection2 ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder bob ;
      BSONElement ele ;
      BSONObj newObj ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         bob.append( FIELD_NAME_ALTER_TYPE, SDB_CATALOG_CL ) ;
         bob.append( FIELD_NAME_VERSION, SDB_ALTER_VERSION ) ;
         bob.append( FIELD_NAME_NAME, _collectionFullName ) ;

         ele = options.getField( FIELD_NAME_ALTER ) ;
         if ( Object == ele.type() || Array == ele.type() )
         {
            bob.append( ele ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = options.getField( FIELD_NAME_OPTIONS ) ;
         if ( Object == ele.type() )
         {
            bob.append( ele ) ;
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION,
                         &newObj ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::_alterCollection1 ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         bob.append ( FIELD_NAME_NAME, _collectionFullName ) ;
         bob.append ( FIELD_NAME_OPTIONS, options ) ;
         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      // run command
      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION,
                         &newObj ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::_alterInternal ( const CHAR * taskName,
                                              const BSONObj * arguments,
                                              BOOLEAN allowNullArgs )
   {
      INT32 rc = SDB_OK ;

      BSONObj alterObject ;

      rc = _sdbBuildAlterObject( taskName, arguments,
                                 allowNullArgs, alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = alterCollection( alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;

   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::alterCollection ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BSONElement ele ;

      ele = options.getField( FIELD_NAME_ALTER ) ;
      if ( EOO == ele.type() )
      {
         rc = _alterCollection1( options ) ;
      }
      else
      {
         rc = _alterCollection2( options ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;

   }

   INT32 _sdbCollectionImpl::explain ( _sdbCursor **cursor,
                                       const bson::BSONObj &condition,
                                       const bson::BSONObj &select,
                                       const bson::BSONObj &orderBy,
                                       const bson::BSONObj &hint,
                                       INT64 numToSkip,
                                       INT64 numToReturn,
                                       INT32 flag,
                                       const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // append info
      try
      {
         bob.append( FIELD_NAME_HINT, hint ) ;
         bob.append( FIELD_NAME_OPTIONS, options ) ;
         newObj= bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      if ( 0 != flag )
      {
         flag = eraseSingleFlag( flag, FLG_QUERY_MODIFY ) ;
      }
      // get query explain
      rc = _query( cursor, condition, select, orderBy, newObj,
                   numToSkip, numToReturn, flag | FLG_QUERY_EXPLAIN ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_getRetLobInfo( CHAR **ppBuffer,
                                             INT32 *pBufSize,
                                             const OID *oid,
                                             INT32 mode,
                                             SINT64 contextID,
                                             _sdbLob **lob )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONElement ele ;
      MsgHeader *pReply = ( MsgHeader* )(*ppBuffer) ;
      INT32 tupleOffset = 0 ;

      if ( !lob || !pReply ||
           pReply->messageLength <= (INT32)sizeof( MsgOpReply ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         obj = BSONObj( *ppBuffer + sizeof( MsgOpReply ) ) ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // build _sdbLob object
      if ( *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }

      *lob = (_sdbLob*)( new(std::nothrow) _sdbLobImpl() ) ;
      if ( !(*lob) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      // set attribute of the newly created _sdbLob object
      if ( NULL != oid )
      {
         ((_sdbLobImpl*)*lob)->_oid = *oid ;
      }
      ((_sdbLobImpl*)*lob)->_contextID = contextID ;
      ((_sdbLobImpl*)*lob)->_isOpen = TRUE ;
      ((_sdbLobImpl*)*lob)->_mode = mode ;
      ((_sdbLobImpl*)*lob)->_lobSize = 0 ;
      ((_sdbLobImpl*)*lob)->_createTime = 0 ;
      ((_sdbLobImpl*)*lob)->_modificationTime = 0 ;

      // setting connection should behind setting _contextID,
      // for if setting failed, we need to close the lob
      rc = ((_sdbLobImpl*)*lob)->_setConnection( _connection ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_LOB_OID ) ;
      if ( !ele.eoo() )
      {
         if ( jstOID == ele.type() )
         {
            ((_sdbLobImpl*)*lob)->_oid = ele.OID() ;
         }
         else
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }

      ele = obj.getField( FIELD_NAME_LOB_SIZE ) ;
      if ( NumberInt == ele.type() || NumberLong == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_lobSize = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // createTime
      ele = obj.getField( FIELD_NAME_LOB_CREATETIME ) ;
      if ( NumberLong == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_createTime = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // modificationTime
      ele = obj.getField( FIELD_NAME_LOB_MODIFICATION_TIME ) ;
      if ( NumberLong == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_modificationTime = ele.numberLong() ;
      }
      else
      {
         ((_sdbLobImpl*)*lob)->_modificationTime =
            ((_sdbLobImpl*)*lob)->_createTime ;
      }

      // lob pageSize
      ele = obj.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
      if ( NumberInt == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_pageSize =  ele.numberInt() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_LOB_FLAG ) ;
      if ( NumberInt == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_flag = (UINT32)ele.numberInt() ;
      }
      ele = obj.getField( FIELD_NAME_LOB_PIECESINFONUM ) ;
      if ( NumberInt == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_piecesInfoNum = ele.numberInt() ;
      }
      ele = obj.getField( FIELD_NAME_LOB_PIECESINFO ) ;
      if ( Array == ele.type() )
      {
         ((_sdbLobImpl*)*lob)->_piecesInfo = BSONArray( ele.embeddedObject() ) ;
      }

      tupleOffset = ossRoundUpToMultipleX( sizeof( MsgOpReply ) +
                                           obj.objsize(), 4 ) ;

      /// prepare cache from return data
      if ( pReply->messageLength > tupleOffset )
      {
         // the return message format is as below:
         // "MsgOpReply  |  Meta Object  |  _MsgLobTuple   | Data"
         const MsgLobTuple *tuple = NULL ;
         const CHAR *body = NULL ;
         INT32 retMsgLen = pReply->messageLength ;

         // let lob's receive buffer pointer points to the cl's receive buffer
         ((_sdbLobImpl*)*lob)->_pReceiveBuffer = *ppBuffer ;
         *ppBuffer = NULL ;
         ((_sdbLobImpl*)*lob)->_receiveBufferSize = *pBufSize ;
         *pBufSize = 0 ;

         // initialize lob with the return data
         tuple = (MsgLobTuple *)(((_sdbLobImpl*)*lob)->_pReceiveBuffer +
                                  tupleOffset) ;
         // "tuple->columns.offset" is the offset of lob content
         // in engine, at the very beginning, the offset must be 0,
         // for we have not read anything yet
         if ( 0 != tuple->columns.offset )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         else if ( retMsgLen < (INT32)( tupleOffset + sizeof( MsgLobTuple ) +
                                        tuple->columns.len ) )
         {
            rc = SDB_SYS ;
            goto error ;
         }

         body = (const CHAR*)tuple + sizeof( MsgLobTuple ) ;
         ((_sdbLobImpl*)*lob)->_currentOffset = 0 ;
         ((_sdbLobImpl*)*lob)->_cachedOffset = 0 ;
         // "tuple->columns.len" is the length of lob content return
         // by engine
         ((_sdbLobImpl*)*lob)->_cachedSize = tuple->columns.len ;
         ((_sdbLobImpl*)*lob)->_dataCache = body ;
      }

   done:
      return rc ;
   error:
      if ( lob && *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::createLob( _sdbLob **lob, const bson::OID *oid )
   {
      INT32 rc = SDB_OK ;
      OID oidObj ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // If user does not offer oid, we won't create one here.
      // Because, we don't known whether the engine is new or old(
      // when the engine is older than v3.2.4, it's an old engine
      // which is not support sub cl for lob).
      // When oid is null, the old engine will return -6, but the
      // new engine will create one by using the new rule.
      if ( NULL != oid )
      {
         oidObj = *oid ;
      }

      if ( !_connection->_getIsOldVersionLobServer() )
      {
         BOOLEAN isOldVersionLobServer = FALSE ;
         rc = _createLob( lob, &oidObj, &isOldVersionLobServer ) ;
         if ( isOldVersionLobServer )
         {
            // deal with old version server. oid should be generate in client side
            oidObj = OID::gen() ;
            rc = _createLob( lob, &oidObj ) ;
            if ( SDB_OK == rc )
            {
               _connection->_setIsOldVersionLobServer( TRUE ) ;
            }
         }
      }
      else
      {
         // deal with old version server. oid should be generate in client side
         if ( !oidObj.isSet() )
         {
            oidObj = OID::gen() ;
         }

         rc = _createLob( lob, &oidObj ) ;
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

   INT32 _sdbCollectionImpl::_createLob( _sdbLob **lob, const bson::OID *oid,
                                         BOOLEAN *isOldVersionLobServer )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      SINT64 contextID = -1 ;
      BOOLEAN locked = FALSE ;

      if ( !lob )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // append info
      try
      {
         BSONObjBuilder bob ;
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         if( NULL != oid && oid->isSet() )
         {
            bob.appendOID( FIELD_NAME_LOB_OID, (bson::OID *)oid ) ;
         }

         bob.append( FIELD_NAME_LOB_OPEN_MODE, SDB_LOB_CREATEONLY ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // build msg
      rc = clientBuildOpenLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     obj.objdata(), 0, 1, 0,
                                     _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      _connection->unlock() ;
      locked = FALSE ;

      if ( SDB_INVALIDARG == rc && (NULL == oid || !oid->isSet()) )
      {
         if ( NULL != isOldVersionLobServer )
         {
            *isOldVersionLobServer = TRUE ;
         }
      }

      if ( SDB_OK == rc )
      {
         // check return msg header
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      }
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _getRetLobInfo( &_pReceiveBuffer, &_receiveBufferSize, oid,
                           SDB_LOB_CREATEONLY, contextID, lob ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( locked )
      {
         _connection->unlock () ;
      }
      return rc ;
   error:
      if ( lob && *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }

      goto done ;
   }

   INT32 _sdbCollectionImpl::removeLob( const bson::OID &oid )
   {
      INT32 rc = SDB_OK ;
      BSONObj meta ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // append info
      try
      {
         BSONObjBuilder bob ;
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (OID *)(&oid) ) ;
         meta = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      // build msg
      rc = clientBuildRemoveLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                       meta.objdata(), 0, 1, 0,
                                       _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::truncateLob( const bson::OID &oid, INT64 length )
   {
      INT32 rc = SDB_OK ;
      BSONObj meta ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( length < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // append info
      try
      {
         BSONObjBuilder bob ;
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (OID *)(&oid) ) ;
         bob.append( FIELD_NAME_LOB_LENGTH, length ) ;
         meta = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      // build msg
      rc = clientBuildTruncateLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                         meta.objdata(), 0, 1, 0,
                                         _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::openLob( _sdbLob **lob, const bson::OID &oid,
                                      INT32 mode )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObj obj ;
      SINT64 contextID = -1 ;
      BOOLEAN locked = FALSE ;

      // check
      if ( '\0' == _collectionFullName[0] || !lob )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( SDB_LOB_CREATEONLY != mode && !isLobReadOnlyMode( mode ) &&
           !hasLobWriteMode( mode) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_LOB_READ == mode )
      {
         flag |= FLG_LOBOPEN_WITH_RETURNDATA ;
      }

      // append info
      try
      {
         BSONObjBuilder bob ;
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (bson::OID *)&oid ) ;
         bob.append( FIELD_NAME_LOB_OPEN_MODE, mode ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      // build msg
      rc = clientBuildOpenLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     obj.objdata(), flag,
                                     1, 0, _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      _connection->unlock() ;
      locked = FALSE ;
      if ( SDB_OK == rc )
      {
         // check return msg header
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      }
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _getRetLobInfo( &_pReceiveBuffer, &_receiveBufferSize, &oid,
                           mode, contextID, lob ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      if ( locked )
      {
         _connection->unlock () ;
      }
      return rc ;
   error:
      if ( lob && *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::listLobs( _sdbCursor **cursor,
                                       const bson::BSONObj &condition,
                                       const bson::BSONObj &selected,
                                       const bson::BSONObj &orderBy,
                                       const bson::BSONObj &hint,
                                       INT64 numToSkip,
                                       INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newHint ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // append info
      try
      {
         bob.appendElements( hint ) ;
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         newHint = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      if ( !_connection->_getIsOldVersionLobServer() )
      {
         // run command with new version format
         rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, &condition,
                            &selected, &orderBy, &newHint, numToSkip,
                            numToReturn, cursor ) ;
         if ( SDB_INVALIDARG == rc )
         {
            if ( !condition.isEmpty() || !selected.isEmpty()
                 || !orderBy.isEmpty() || !hint.isEmpty()
                 || 0 != numToSkip || -1 != numToReturn )
            {
               // recheck remote server is old or not
               _sdbCursor *tmpCursor = NULL ;
               BSONObj tmpHint = BSON( FIELD_NAME_COLLECTION << _collectionFullName ) ;
               // run command with new version format(But without condition etc.)
               rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, NULL,
                                  NULL, NULL, &tmpHint, 0, -1, &tmpCursor ) ;
               if ( SDB_OK == rc )
               {
                  //Now we are sure that remote server is new version.
                  tmpCursor->close() ;
                  RELEASE_INNER_HANDLE( tmpCursor ) ;
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }

            //otherwise remote server may be old version
            if ( SDB_INVALIDARG == rc )
            {
               // deal with old version server. clName is in the query field
               rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                                  &newHint, NULL, NULL, NULL, 0, -1,
                                  cursor ) ;
               if ( SDB_OK == rc )
               {
                  _connection->_setIsOldVersionLobServer( TRUE ) ;
               }
            }
         }
      }
      else
      {
         // deal with old version server. clName is in the query field
         rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, &newHint, NULL,
                            NULL, NULL, 0, -1, cursor ) ;
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

   INT32 _sdbCollectionImpl::listLobPieces( _sdbCursor **cursor,
                                            const bson::BSONObj &condition,
                                            const bson::BSONObj &selected,
                                            const bson::BSONObj &orderBy,
                                            const bson::BSONObj &hint,
                                            INT64 numToSkip,
                                            INT64 numToReturn )
   {
      INT32 rc = SDB_OK ;
      BSONObj newHint ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.appendElements( hint ) ;
         queryBuilder.append( FIELD_NAME_COLLECTION, this->getFullName() ) ;
         queryBuilder.appendBool( FIELD_NAME_LOB_LIST_PIECES_MODE, TRUE ) ;
         newHint = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      if ( !_connection->_getIsOldVersionLobServer() )
      {
         rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, &condition,
                            &selected, &orderBy, &newHint, numToSkip,
                            numToReturn, cursor ) ;
         if ( SDB_INVALIDARG == rc )
         {
            // deal with old version server. clName is in the query field
            rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, &newHint,
                               NULL, NULL, NULL, 0, -1, cursor ) ;
            if ( SDB_OK == rc )
            {
               _connection->_setIsOldVersionLobServer( TRUE ) ;
            }
         }
      }
      else
      {
         // deal with old version server. clName is in the query field
         rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, &newHint, NULL,
                            NULL, NULL, 0, -1, cursor ) ;
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

   INT32 _sdbCollectionImpl::createLobID( bson::OID &oid,
                                          const CHAR *pTimeStamp )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BOOLEAN locked = FALSE ;
      const MsgOpReply* reply = NULL ;
      BSONObj dateInfo ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( NULL != pTimeStamp )
      {
         try
         {
            dateInfo = BSON( FIELD_NAME_LOB_CREATETIME << pTimeStamp ) ;
         }
         catch ( std::exception )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }

      // build msg
      rc = clientBuildCreateLobIDMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                         dateInfo.objdata(), 0, 1, 0,
                                         _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg from engine
      rc = _connection->_recvExtract( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;

      reply = ( const MsgOpReply * )( _pReceiveBuffer ) ;
      if ( reply->numReturned > 0 &&
           (UINT32)reply->header.messageLength >
           ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
      {
         // get reply bson from received msg
         const CHAR* bsonBuf = _pReceiveBuffer + sizeof( MsgOpReply ) ;
         try
         {
            BSONObj obj = BSONObj( bsonBuf ) ;
            BSONElement ele = obj.getField( FIELD_NAME_LOB_OID ) ;
            if ( jstOID != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            oid = ele.OID() ;
         }
         catch ( std::exception )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_runCmdOfLob ( const CHAR *cmd,
                                            const BSONObj *query,
                                            const BSONObj *selector,
                                            const BSONObj *orderBy,
                                            const BSONObj *hint,
                                            INT64 skip,
                                            INT64 returnNum,
                                            _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;

      // check
      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = _connection->_runCommand( cmd, query, selector, orderBy, hint,
                                     0, 0, skip, returnNum, cursor ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::truncate( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         BSONObjIterator iterOptions( options ) ;
         while ( iterOptions.more() )
         {
            BSONElement ele = iterOptions.next() ;
            // skip collection field
            if ( 0 != ossStrcmp( FIELD_NAME_COLLECTION, ele.fieldName() ) )
            {
               builder.append( ele ) ;
            }
         }
         builder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         obj = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE,
                                     &obj ) ;
      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::createIdIndex( const bson::BSONObj &options )
   {
      return _alterInternal( SDB_ALTER_CL_CRT_ID_INDEX, &options, FALSE ) ;
   }

   INT32 _sdbCollectionImpl::dropIdIndex()
   {
      return _alterInternal( SDB_ALTER_CL_DROP_ID_INDEX, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionImpl::createAutoIncrement( const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      if( options.nFields() <= 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      obj = BSON( FIELD_NAME_AUTOINCREMENT << options );
      rc = _alterInternal( SDB_ALTER_CL_CRT_AUTOINC_FLD, &obj, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::createAutoIncrement( const std::vector<bson::BSONObj> &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder builder ;
      BSONArrayBuilder autoincBuilder( builder.subarrayStart( FIELD_NAME_AUTOINCREMENT )) ;

      if( !options.size() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         for( UINT32 i = 0; i < options.size(); i++ )
         {
            autoincBuilder.append( options[i] ) ;
         }
         autoincBuilder.done() ;
         obj = builder.obj() ;
         rc = _alterInternal( SDB_ALTER_CL_CRT_AUTOINC_FLD, &obj, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::dropAutoIncrement( const CHAR* fieldName )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder builder ;

      if( !fieldName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      builder.append( FIELD_NAME_AUTOINC_FIELD, fieldName );
      builder.done() ;
      obj = builder.obj() ;

      rc = _alterInternal( SDB_ALTER_CL_DROP_AUTOINC_FLD, &obj, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::dropAutoIncrement( const std::vector<const CHAR*> &fieldNames )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder builder ;
      BSONArrayBuilder autoincBuilder( builder.subarrayStart( FIELD_NAME_AUTOINC_FIELD )) ;

      if( !fieldNames.size() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         for( UINT32 i = 0; i < fieldNames.size(); i++ )
         {
            if( !fieldNames[i] )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "The field[%d] name is null", i ) ;
               goto error ;
            }
            autoincBuilder.append( fieldNames[i] ) ;
         }
         autoincBuilder.done() ;
         obj = builder.obj() ;
         rc = _alterInternal( SDB_ALTER_CL_DROP_AUTOINC_FLD, &obj, FALSE ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 _sdbCollectionImpl::enableSharding ( const bson::BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_CL_ENABLE_SHARDING, &options, FALSE ) ;
   }

   INT32 _sdbCollectionImpl::disableSharding ()
   {
      return _alterInternal( SDB_ALTER_CL_DISABLE_SHARDING, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionImpl::enableCompression ( const bson::BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_CL_ENABLE_COMPRESS, &options, TRUE ) ;
   }

   INT32 _sdbCollectionImpl::disableCompression ()
   {
      return _alterInternal( SDB_ALTER_CL_DISABLE_COMPRESS, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionImpl::setAttributes ( const bson::BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_CL_SET_ATTR, &options, FALSE ) ;
   }

   INT32 _sdbCollectionImpl::getDetail ( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;

      if ( '\0' == _collectionFullName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      obj = BSON( FIELD_NAME_COLLECTION << _collectionFullName ) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_CL_DETAIL,
                                     NULL, NULL, NULL, &obj,
                                     0, 0, 0, -1,
                                     cursor ) ;

      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::getCollectionStat ( bson::BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder ob ;
      BSONObj obj ;
      sdbCursor cursor ;

      if ( '\0' == _collectionFullName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
      obj = ob.obj() ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_CL_STAT,
                                     NULL, NULL, NULL, &obj,
                                     0, 0, 0, -1,
                                     &cursor.pCursor ) ;

      updateCachedVersion( rc, _connection->_getCachedContainer(),
                           _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cursor.next( result, TRUE ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::getIndexStat ( const CHAR *pIndexName,
                                            bson::BSONObj &result,
                                            BOOLEAN detail )
   {
      INT32 rc = SDB_OK ;
      BSONObj hint ;
      sdbCursor cursor ;

      if ( '\0' == _collectionFullName[0] || !pIndexName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // { Collection: 'cl', Index: 'idx', $Options: { Detail: false } }
      {
         BSONObjBuilder builder( 128 ) ;
         builder.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         builder.append( FIELD_NAME_INDEX, pIndexName ) ;
         BSONObjBuilder subBuilder(
               builder.subobjStart( CMD_ADMIN_PREFIX FIELD_NAME_OPTIONS ) ) ;
         subBuilder.appendBool( FIELD_NAME_DETAIL, detail ) ;
         subBuilder.done() ;
         hint = builder.obj() ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_INDEX_STAT,
                                     NULL, NULL, NULL, &hint,
                                     0, 0, 0, -1,
                                     &cursor.pCursor ) ;

      /// ignore update result
      updateCachedVersion( rc, _connection->_getCachedContainer(),
                          _collectionFullName, _version ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = cursor.next( result, TRUE ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_IXM_STAT_NOTEXIST ;
         goto error ;
      }
      if ( rc )
      {
         goto error ;
      }
   done:
      cursor.close() ;
      return rc ;
   error:
      goto done ;
   }

   void _sdbCollectionImpl::setVersion ( INT32 clVersion )
   {
      _version = clVersion;
   }

   INT32 _sdbCollectionImpl::getVersion ()
   {
      return _version;
   }

   INT32 _sdbCollectionImpl::setConsistencyStrategy( INT32 value )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_CONSISTENCY_STRATEGY, value ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _alterInternal( SDB_ALTER_CL_SET_ATTR, &query, FALSE ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
    * _sdbNodeImpl
    * Sdb Node Implementation
    */
   _sdbNodeImpl::_sdbNodeImpl () :
   _sdbBase( CLIENT_CLASS_NODE )
   {
      ossMemset ( _hostName, 0, sizeof(_hostName) ) ;
      ossMemset ( _serviceName, 0, sizeof(_serviceName) ) ;
      ossMemset ( _nodeName, 0, sizeof(_nodeName) ) ;
      _nodeID = SDB_NODE_INVALID_NODEID ;
   }

   _sdbNodeImpl::~_sdbNodeImpl ()
   {
      _dropConnection() ;
   }

   INT32 _sdbNodeImpl::connect ( _sdb **dbConn )
   {
      INT32 rc = SDB_OK ;
      _sdbImpl *connection = NULL ;

      if ( !dbConn || _hostName[0] == '\0' || _serviceName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      connection = new (std::nothrow) _sdbImpl () ;
      if ( !connection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = connection->connect ( _hostName, _serviceName ) ;
      if ( rc )
      {
         goto error ;
      }
      *dbConn = connection ;
   done :
      return rc ;
   error :
      if ( connection )
      {
         delete connection ;
      }
      goto done ;
   }

   sdbNodeStatus _sdbNodeImpl::getStatus ( )
   {
      sdbNodeStatus sns     = SDB_NODE_UNKNOWN ;
      INT32 rc              = SDB_OK ;

      _sdbCursor *pCursor   = NULL ;
      BSONObjBuilder builder ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObj obj ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      builder.append( FIELD_NAME_HOST, _hostName ) ;
      builder.append( FIELD_NAME_SERVICE_NAME, _serviceName ) ;
      builder.appendBool( FIELD_NAME_RAWDATA, TRUE ) ;
      condition = builder.obj() ;

      selector = BSON( FIELD_NAME_STATUS << 1 ) ;

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                                     &condition, &selector, NULL, NULL,
                                     0, 0, 0, 1,
                                     &pCursor ) ;
      if ( rc )
      {
         goto error ;
      }

      /// get cursor
      rc = pCursor->next( obj ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( !obj.getField( FIELD_NAME_STATUS ).eoo() )
      {
         sns = SDB_NODE_ACTIVE ;
      }
      else
      {
         sns = SDB_NODE_INACTIVE ;
      }

   done :
      if ( pCursor )
      {
         delete pCursor ;
      }
      return sns ;
   error :
      goto done ;
   }

   INT32 _sdbNodeImpl::_stopStart ( BOOLEAN start )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;

      if ( _hostName[0] == '\0' || _serviceName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      configuration = BSON ( CAT_HOST_FIELD_NAME << _hostName <<
                             PMD_OPTION_SVCNAME << _serviceName ) ;
      rc = _connection->_runCommand ( start?
                                      (CMD_ADMIN_PREFIX CMD_NAME_STARTUP_NODE) :
                                      (CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_NODE),
                                      &configuration ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbNodeImpl::_innerAlter( const CHAR* taskName, const BSONObj* options )
   {
      INT32 rc = SDB_OK ;

      const CHAR *command = CMD_ADMIN_PREFIX CMD_NAME_ALTER_NODE ;
      BSONObj queryObj, hintObj ;

      try
      {
         BSONObjBuilder queryBuilder, hintBuilder ;
         
         // Build Action and Options
         queryBuilder.append( FIELD_NAME_ACTION, taskName ) ;

         if ( NULL == options )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            queryBuilder.append( FIELD_NAME_OPTIONS, *options ) ;
         }

         queryObj = queryBuilder.obj() ;

         // Build GroupID and NodeID
         hintBuilder.append( FIELD_NAME_GROUPID, _replicaGroupID ) ;
         hintBuilder.append( FIELD_NAME_NODEID, _nodeID ) ;

         hintObj = hintBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // check connection
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( command, &queryObj, NULL, NULL, &hintObj,
                                     0, 0, 0, -1, NULL ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbNodeImpl::setLocation ( const CHAR *pLocation )
   {
      INT32 rc = SDB_OK ;

      BSONObjBuilder builder ;
      BSONObj option ;

      if ( NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // build { Location: pLocation }
         builder.append( FIELD_NAME_NODE_LOCATION, pLocation ) ;
         option = builder.obj() ;
      }
      catch( const std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( SDB_ALTER_NODE_SET_LOCATION, &option ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbNodeImpl::setAttributes ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_NODE_SET_ATTR, &options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   /*
      _sdbReplicaGroupImpl implement
   */
   _sdbReplicaGroupImpl::_sdbReplicaGroupImpl () :
   _sdbBase( CLIENT_CLASS_RG )
   {
      ossMemset ( _replicaGroupName, 0, sizeof(_replicaGroupName) ) ;
      _isCatalog = FALSE ;
      _replicaGroupID = 0 ;
   }

   _sdbReplicaGroupImpl::~_sdbReplicaGroupImpl ()
   {
      _dropConnection() ;
   }

   INT32 _sdbReplicaGroupImpl::getNodeNum ( sdbNodeStatus status, INT32 *num )
   {
      INT32 rc = SDB_OK ;
      BSONObj result ;
      BSONElement ele ;
      INT32 totalCount = 0 ;

      if ( !num )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = getDetail ( result ) ;
      if ( rc )
      {
         goto error ;
      }
      ele = result.getField ( CAT_GROUP_NAME ) ;
      // get total number of nodes
      if ( ele.type() == Array )
      {
         BSONObjIterator it ( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            it.next () ;
            ++totalCount ;
         }
      }
      *num = totalCount ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::createNode ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const CHAR *pDatabasePath,
                                            const bson::BSONObj &options,
                                            _sdbNode **ppNode )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObjBuilder ob ;
      _sdbCursor *cursor = NULL;
      BSONObj nodeObj;

      if ( _replicaGroupName[0] == '\0' ||
           !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // GroupName is required
      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;

      // HostName is required
      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;

      // service name is required
      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

      // database path is required
      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;

      // append all other parameters into configuration
      ob.appendElementsUnique( options ) ;

      configuration = ob.obj () ;

      // run command
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE, &configuration, NULL,
                                     NULL, NULL, 0, 0, 0, -1, &cursor ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( ppNode )
      {
         rc = cursor->next( nodeObj );
         if ( SDB_DMS_EOC == rc )
         {
            rc = getNode( pHostName, pServiceName, ppNode );
            if ( rc )
            {
               goto error ;
            }
         }
         if ( rc )
         {
            goto error;
         }
         else
         {
            rc = _extractNode( ppNode, nodeObj.objdata());
            if ( rc )
            {
               goto error ;
            }
            // if no match, let's clear
            if ( ossStrcmp( ( (_sdbNodeImpl *)( *ppNode ) )->_hostName, pHostName ) != 0 ||
                 ossStrcmp( ( (_sdbNodeImpl *)( *ppNode ) )->_serviceName, pServiceName ) != 0 )
            {
               SDB_OSS_DEL( ( *ppNode ) );
               *ppNode = NULL;
               goto error;
            }
         }
      }

   done:
      SAFE_OSS_DELETE( cursor ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::createNode ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const CHAR *pDatabasePath,
                                            map<string,string> &config,
                                            _sdbNode **ppNode )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObjBuilder ob ;
      map<string,string>::iterator it ;
      _sdbCursor *cursor = NULL;
      BSONObj nodeObj;

      if ( _replicaGroupName[0] == '\0' ||
           !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // GroupName is required
      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;
      config.erase ( CAT_GROUPNAME_NAME ) ;

      // HostName is required
      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;
      config.erase ( CAT_HOST_FIELD_NAME ) ;

      // service name is required
      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;
      config.erase ( PMD_OPTION_SVCNAME ) ;

      // database path is required
      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;
      config.erase ( PMD_OPTION_DBPATH ) ;

      // append all other parameters into configuration
      for ( it = config.begin(); it != config.end(); ++it )
      {
         ob.append ( it->first.c_str(),
                     it->second.c_str() ) ;
      }
      configuration = ob.obj () ;

      // run command
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE, &configuration, NULL,
                                     NULL, NULL, 0, 0, 0, -1, &cursor ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( ppNode )
      {
         rc = cursor->next( nodeObj );
         if ( SDB_DMS_EOC == rc )
         {
            rc = getNode( pHostName, pServiceName, ppNode );
            if ( rc )
            {
               goto error ;
            }
         }
         else
         {
            rc = _extractNode( ppNode, nodeObj.objdata());
            if ( rc )
            {
               goto error ;
            }
            // if no match, let's clear
            if ( ossStrcmp( ( (_sdbNodeImpl *)( *ppNode ) )->_hostName, pHostName ) != 0 ||
                 ossStrcmp( ( (_sdbNodeImpl *)( *ppNode ) )->_serviceName, pServiceName ) != 0 )
            {
               SDB_OSS_DEL( ( *ppNode ) );
               *ppNode = NULL;
               goto error;
            }
         }
      }

   done:
      SAFE_OSS_DELETE( cursor ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::removeNode ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const BSONObj &configure )
   {
      INT32 rc = SDB_OK ;
      BSONObj removeInfo ;
      BSONObjBuilder ob ;
      const CHAR *pRemoveNode = CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE ;

      if ( !pHostName || !pServiceName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // GroupName is required
      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;
      // HostName is required
      ob.append ( FIELD_NAME_HOST, pHostName ) ;

      // ServiceName is required
      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

      // append all other parameters
      {
         BSONObjIterator it ( configure ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next () ;
            const CHAR *key = ele.fieldName() ;
            if ( ossStrcmp ( key, CAT_GROUPNAME_NAME ) == 0 ||
                 ossStrcmp ( key, FIELD_NAME_HOST ) == 0  ||
                 ossStrcmp ( key, PMD_OPTION_SVCNAME ) == 0 )
            {
               // skip the ones we already created
               continue ;
            }
            ob.append ( ele ) ;
         } // while
      } // if ( configure )
      removeInfo = ob.obj () ;

      rc = _connection->_runCommand ( pRemoveNode, &removeInfo ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::getDetail ( BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;
      sdbCursor cursor ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( CAT_GROUPNAME_NAME << _replicaGroupName ) ;
      rc = _connection->getList ( &cursor.pCursor, SDB_LIST_GROUPS, newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = cursor.next ( result ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::_extractNode ( _sdbNode **node,
                                              const CHAR *primaryData )
   {
      INT32 rc = SDB_OK ;
      _sdbNodeImpl *pNode = NULL ;
      // primaryData is pointed to bson data, it's not a normal
      // string, so we can't use '!*primaryData' to check it is
      // an empty string or not
      if ( !node || !primaryData )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // create new node object
      pNode = new ( std::nothrow) _sdbNodeImpl () ;
      if ( !pNode )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      pNode->_replicaGroupID = _replicaGroupID ;
      // setup connection
      rc = pNode->_setConnection( this->_connection ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = clientReplicaGroupExtractNode ( primaryData,
                                           pNode->_hostName,
                                           OSS_MAX_HOSTNAME,
                                           pNode->_serviceName,
                                           OSS_MAX_SERVICENAME,
                                           &pNode->_nodeID ) ;
      if ( rc )
      {
         goto error ;
      }
      ossStrcpy ( pNode->_nodeName, pNode->_hostName ) ;
      ossStrncat ( pNode->_nodeName, NODE_NAME_SERVICE_SEP, 1 ) ;
      ossStrncat ( pNode->_nodeName, pNode->_serviceName,
                   OSS_MAX_SERVICENAME ) ;
      *node = pNode ;
   done :
      return rc ;
   error :
      if ( pNode )
      {
         delete pNode ;
         pNode = NULL ;
      }
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::getMaster ( _sdbNode **node )
   {
      INT32 rc = SDB_OK ;
      INT32 primaryNode = -1 ;
      const CHAR *primaryData = NULL ;
      BSONObj result ;
      BSONElement ele ;

      if ( !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // get information of nodes from catalog
      rc = getDetail ( result ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
         goto error ;
      }
      // check the nodes in current group
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
         // the replica group is not array
         rc = SDB_SYS ;
         goto error ;
      }
      {
         BSONObjIterator it ( ele.embeddedObject() ) ;
         if ( !it.more() )
         {
            rc = SDB_CLS_EMPTY_GROUP ;
            goto error ;
         }
      }
      // check have primary or not
      ele = result.getField ( CAT_PRIMARY_NAME ) ;
      if ( ele.type() == EOO )
      {
         // cannot find primary
         rc = SDB_RTN_NO_PRIMARY_FOUND ;
         goto error ;
      }
      if ( ele.type() != NumberInt )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      primaryNode = ele.numberInt () ;
      if ( -1 == primaryNode )
      {
         // cannot find primary
         rc = SDB_RTN_NO_PRIMARY_FOUND ;
         goto error ;
      }
      // walk through the replica group and find out the NodeID
      {
         ele = result.getField ( CAT_GROUP_NAME ) ;
         BSONObjIterator it ( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONObj embObj ;
            BSONElement embEle = it.next() ;
            // make sure each element is object and construct intObj object
            // bson_init_finished_data does not accept const CHAR*,
            // however since we are NOT going to perform any change, it's afe to
            // cast const CHAR* to CHAR* here
            if ( Object == embEle.type() )
            {
               embObj = embEle.embeddedObject() ;
               // look for "NodeID" in each object
               BSONElement embEle1 = embObj.getField ( CAT_NODEID_NAME ) ;
               if ( embEle1.type() != NumberInt )
               {
                  rc = SDB_SYS ;
                  goto error ;
               }
               // if we find the master, let's record the pointer and jump out
               if ( primaryNode == embEle1.numberInt() )
               {
                  primaryData = embObj.objdata() ;
                  break ;
               }
            }
            else
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
      if ( primaryData )
      {
         rc = _extractNode ( node, primaryData ) ;
         if ( rc )
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
      return rc ;
   error :
      goto done ;
   }

   // attempt to get slave, if no slave exist, return primary
   INT32 _sdbReplicaGroupImpl::getSlave ( _sdbNode **node,
                                          const vector<INT32>& positions )
   {
      INT32 rc = SDB_OK ;
      BSONObj result ;
      BSONElement ele ;
      vector<const CHAR*> nodeDatas ;
      vector<INT32>::const_iterator it ;
      vector<INT32> validPositions ;
      INT32 nodeCount = 0 ;
      INT32 primaryNodeId = -1 ;
      INT32 primaryNodePosition = 0 ;
      BOOLEAN hasPrimary = TRUE ;
      BOOLEAN needGeneratePosition = FALSE ;

      // check arguments
      if ( !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      for ( it = positions.begin(); it != positions.end(); it++ )
      {
         vector<INT32>::iterator it_inner = validPositions.begin() ;
         BOOLEAN hasContained = FALSE ;
         if ( *it < 1 || *it > 7 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         for ( ; it_inner != validPositions.end(); it_inner++ )
         {
            if ( *it == *it_inner )
            {
               hasContained = TRUE ;
               break ;
            }
         }
         if ( !hasContained )
         {
            validPositions.push_back( *it ) ;
         }
      }
      // when user do not specify any positions,
      // we add 2 to select a slave node
      if ( validPositions.size() == 0 )
      {
         needGeneratePosition = TRUE ;
      }
      // get detail of group from catalog
      rc = getDetail ( result ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
         goto error ;
      }
      // check the nodes in current group
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
         // the replica group is not array
         rc = SDB_SYS ;
         goto error ;
      }
      {
         BSONObjIterator it ( ele.embeddedObject() ) ;
         if ( !it.more() )
         {
            rc = SDB_CLS_EMPTY_GROUP ;
            goto error ;
         }
      }
      ele = result.getField ( CAT_PRIMARY_NAME ) ;
      if ( ele.type() == EOO )
      {
         hasPrimary = FALSE ;
      }
      else if ( ele.type() != NumberInt )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      else
      {
         // get the primary node and skip it later
         primaryNodeId = ele.numberInt () ;
         if ( -1 == primaryNodeId )
         {
            hasPrimary = FALSE ;
         }
      }
      // walk through replica group and skip primary node, and pick up a random one
      {
         ele = result.getField ( CAT_GROUP_NAME ) ;
         BSONObj objReplicaGroupList = ele.embeddedObject() ;
         BSONObjIterator it ( objReplicaGroupList ) ;
         // loop for all elements in the replica group
         INT32 counter = 0 ;
         while ( it.more() )
         {
            ++counter ;
            BSONObj embObj ;
            BSONElement embEle ;
            // make sure each element is object and construct intObj object
            // bson_init_finished_data does not accept const CHAR*,
            // however since we are NOT going to perform any change, it's safe
            // to cast const CHAR* to CHAR* here
            embEle = it.next() ;
            if ( embEle.type() == Object )
            {
               embObj = embEle.embeddedObject() ;
               BSONElement embEle1 = embObj.getField ( CAT_NODEID_NAME ) ;
               // look for "NodeID" in each object
               if ( embEle1.type() != NumberInt )
               {
                  rc = SDB_SYS ;
                  goto error ;
               }
               nodeDatas.push_back ( embObj.objdata() ) ;
               if ( hasPrimary && primaryNodeId == embEle1.numberInt() )
               {
                  primaryNodePosition = counter ;
               }
            }
            else
            {
               rc = SDB_SYS ;
               goto error ;
            }
         }
      }
      // check
      if ( hasPrimary && 0 == primaryNodePosition )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      // try to generate slave node's positions
      nodeCount = nodeDatas.size() ;
      if ( needGeneratePosition )
      {
         INT32 i = 0 ;
         for ( ; i < nodeCount ; i++ )
         {
            if ( hasPrimary && primaryNodePosition == i + 1 )
            {
               continue ;
            }
            validPositions.push_back( i + 1 ) ;
         }
      }
      // Build sdbNode based on hostname and service name
      if ( nodeCount == 1 )
      {
         rc = _extractNode ( node, nodeDatas[0] ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else if ( validPositions.size() == 1 )
      {
         INT32 idx = ( validPositions[0] - 1 ) % nodeCount ;
         rc = _extractNode ( node, nodeDatas[idx] ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      else
      {
         INT32 position = 0 ;
         INT32 nodeIndex = -1 ;
         INT32 flags[7] = { 0 } ;
         INT32 rand = _sdbRand() ;
         vector<INT32> includePrimaryPositions ;
         vector<INT32> excludePrimaryPositions ;
         vector<INT32>::iterator it = validPositions.begin() ;
         for ( ; it != validPositions.end() ; it++ )
         {
            INT32 pos = *it ;
            if ( pos <= nodeCount )
            {
               nodeIndex = pos - 1 ;
               if ( flags[nodeIndex] == 0 )
               {
                  flags[nodeIndex] = 1 ;
                  includePrimaryPositions.push_back( pos ) ;
                  if ( hasPrimary && primaryNodePosition != pos )
                  {
                     excludePrimaryPositions.push_back( pos ) ;
                  }
               }
            }
            else
            {
               nodeIndex = ( pos - 1 ) % nodeCount ;
               if ( flags[nodeIndex] == 0 )
               {
                  flags[nodeIndex] = 1 ;
                  includePrimaryPositions.push_back( pos ) ;
                  if ( hasPrimary && primaryNodePosition != nodeIndex + 1 )
                  {
                     excludePrimaryPositions.push_back( pos ) ;
                  }
               }
            }
         }
         // after removing, let's select a slave node
         if ( excludePrimaryPositions.size() > 0 )
         {
            position = rand % excludePrimaryPositions.size() ;
            position = excludePrimaryPositions[position] ;
         }
         else
         {
            position = rand % includePrimaryPositions.size() ;
            position = includePrimaryPositions[position] ;
            if ( needGeneratePosition )
            {
               position += 1 ;
            }
         }
         nodeIndex = ( position - 1 ) % nodeCount ;
         rc = _extractNode( node, nodeDatas[nodeIndex] ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::getNode ( const CHAR *pHostName,
                                         const CHAR *pServiceName,
                                         _sdbNode **node )
   {
      INT32 rc = SDB_OK ;
      BSONObj result ;
      BSONElement ele ;
      if ( !pHostName || !pServiceName || !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      *node = NULL ;
      // get detail of the current node's replica group
      rc = getDetail ( result ) ;
      if ( rc )
      {
         goto error ;
      }
      // find the replica group field
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      {
         BSONObjIterator it ( ele.embeddedObject() ) ;
         // iterate all members in the replica group
         while ( it.more() )
         {
            BSONElement embEle ;
            BSONObj embObj ;
            embEle = it.next() ;
            if ( embEle.type() == Object )
            {
               embObj = embEle.embeddedObject() ;
               // build a temp node
               rc = _extractNode ( node, embObj.objdata() ) ;
               if ( rc )
               {
                  goto error ;
               }
               // if we get a match
               if ( ossStrcmp ( ((_sdbNodeImpl*)(*node))->_hostName,
                                pHostName ) == 0 &&
                    ossStrcmp ( ((_sdbNodeImpl*)(*node))->_serviceName,
                                pServiceName ) == 0 )
                  break ;
               // if no match, let's clear
               SDB_OSS_DEL ( (*node ) ) ;
               *node = NULL ;
            }
         }
      }
      // if we didn't find anything, return not exist
      if ( !(*node) )
      {
         rc = SDB_CLS_NODE_NOT_EXIST ;
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( node && *node )
      {
         SDB_OSS_FREE ( (*node) ) ;
         *node = NULL ;
      }
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::getNode ( const CHAR *pNodeName,
                                         _sdbNode **node )
   {
      INT32 rc = SDB_OK ;
      CHAR *pHostName = NULL ;
      CHAR *pServiceName = NULL ;
      INT32 nodeNameLen = 0 ;
      if ( !pNodeName || !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      nodeNameLen = ossStrlen ( pNodeName ) ;
      pHostName = (CHAR*)SDB_OSS_MALLOC ( nodeNameLen +1 ) ;
      if ( !pHostName )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ossMemset ( pHostName, 0, nodeNameLen + 1 ) ;
      ossStrncpy ( pHostName, pNodeName, nodeNameLen + 1 ) ;
      pServiceName = ossStrchr ( pHostName, NODE_NAME_SERVICE_SEPCHAR ) ;
      if ( !pServiceName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *pServiceName = '\0' ;
      pServiceName ++ ;
      rc = getNode ( pHostName, pServiceName, node ) ;
      if ( rc )
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
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::start ()
   {
      INT32 rc = SDB_OK ;
      BSONObj replicaGroupName ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << _replicaGroupName ) ;
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP,
                                      &replicaGroupName ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::stop ()
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;

      if ( ossStrlen ( _replicaGroupName ) == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      configuration = BSON ( CAT_GROUPNAME_NAME << _replicaGroupName ) ;
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_GROUP,
                                      &configuration );
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::attachNode( const CHAR *pHostName,
                                           const CHAR *pSvcName,
                                           const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      // check
      if ( _replicaGroupName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      if ( NULL == pHostName || !*pHostName ||
           NULL == pSvcName || !*pSvcName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build obj
      bob.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
      bob.append( FIELD_NAME_HOST, pHostName ) ;
      bob.append( PMD_OPTION_SVCNAME, pSvcName ) ;
      bob.appendBool( FIELD_NAME_ONLY_ATTACH, 1 ) ;
      bob.appendElementsUnique( options ) ;
      newObj = bob.obj() ;

      // run command
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::detachNode( const CHAR *pHostName,
                                           const CHAR *pSvcName,
                                           const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      // check
      if ( _replicaGroupName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL == pHostName || !*pHostName ||
           NULL == pSvcName || !*pSvcName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build obj
      bob.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
      bob.append( FIELD_NAME_HOST, pHostName ) ;
      bob.append( PMD_OPTION_SVCNAME, pSvcName ) ;
      bob.appendBool( FIELD_NAME_ONLY_DETACH, 1 ) ;
      bob.appendElementsUnique( options ) ;
      newObj = bob.obj() ;

      // run command
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::reelect( const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      // check
      if ( _replicaGroupName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_REELECT,
                                     &query, NULL, NULL, NULL, 0, 0, 0, -1 ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::reelectLocation( const CHAR* pLocation,
                                                const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      // check
      if ( _replicaGroupName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
         queryBuilder.append( FIELD_NAME_LOCATION, pLocation ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_REELECT_LOCATION,
                                     &query, NULL, NULL, NULL, 0, 0, 0, -1 ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::_innerAlter( const CHAR* taskName,
                                            const BSONObj* pOptions,
                                            BOOLEAN allowNullArgs )
   {
      INT32 rc = SDB_OK ;

      const CHAR *command = CMD_ADMIN_PREFIX CMD_NAME_ALTER_GROUP ;
      BSONObj queryObj, hintObj ;

      try
      {
         BSONObjBuilder queryBuilder, hintBuilder ;

         // Build Action and Options
         queryBuilder.append( FIELD_NAME_ACTION, taskName ) ;

         if ( NULL == pOptions )
         {
            if ( allowNullArgs )
            {
               queryBuilder.appendNull( FIELD_NAME_OPTIONS ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }
         else
         {
            queryBuilder.append( FIELD_NAME_OPTIONS, *pOptions ) ;
         }

         queryObj = queryBuilder.obj() ;

         // Build GroupID
         hintBuilder.append( FIELD_NAME_GROUPID, _replicaGroupID ) ;

         hintObj = hintBuilder.obj() ;
      }
      catch( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // check connection
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( command, &queryObj, NULL, NULL, &hintObj,
                                     0, 0, 0, -1, NULL ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::setActiveLocation( const CHAR* pLocation )
   {
      INT32 rc = SDB_OK ;

      BSONObjBuilder builder ;
      BSONObj option ;

      if ( NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // build { ActiveLocation: pLocation }
         builder.append( FIELD_NAME_GROUP_ACTIVE_LOCATION, pLocation ) ;
         option = builder.obj() ;
      }
      catch( const std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( SDB_ALTER_GROUP_SET_ACTIVE_LOCATION, &option, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbReplicaGroupImpl::setAttributes ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_GROUP_SET_ATTR, &options, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbReplicaGroupImpl::startCriticalMode ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_GROUP_START_CRITICAL_MODE, &options, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbReplicaGroupImpl::stopCriticalMode ()
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_GROUP_STOP_CRITICAL_MODE, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbReplicaGroupImpl::startMaintenanceMode ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_GROUP_START_MAINTENANCE_MODE, &options, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbReplicaGroupImpl::stopMaintenanceMode ( const BSONObj & options )
   {
      INT32 rc = SDB_OK ;

      rc = _innerAlter( SDB_ALTER_GROUP_STOP_MAINTENANCE_MODE, &options, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      done:
         return rc ;
      error:
         goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::_getRetVersion ()
   {
      INT32 version = CATALOG_INVALID_VERSION ;

      if( NULL != _pReceiveBuffer )
      {
         version = ((MsgOpReply*)_pReceiveBuffer)->startFrom ;
      }
      return version ;
   }

   INT32 _sdbCollectionSpaceImpl::_runCommand ( const CHAR *pString,
                                                const BSONObj *arg1,
                                                const BSONObj *arg2,
                                                const BSONObj *arg3,
                                                const BSONObj *arg4,
                                                SINT32 flag,
                                                UINT64 reqID,
                                                SINT64 numToSkip,
                                                SINT64 numToReturn,
                                                _sdbCursor **ppCursor )
   {
      INT32 rc = SDB_OK ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, pString,
                                    flag, reqID, numToSkip, numToReturn,
                                    arg1 ? arg1->objdata() : NULL,
                                    arg2 ? arg2->objdata() : NULL,
                                    arg3 ? arg3->objdata() : NULL,
                                    arg4 ? arg4->objdata() : NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                                      &_receiveBufferSize,
                                      ppCursor, FALSE ) ;

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::_setName ( const CHAR *pCollectionSpaceName )
   {
      INT32 rc = SDB_OK ;
      if ( ossStrlen ( pCollectionSpaceName ) > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) ) ;
      ossStrncpy( _collectionSpaceName, pCollectionSpaceName,
                  CLIENT_CS_NAMESZ );
   exit:
      return rc ;
   }

   /*
    * sdbCollectionSpaceImpl
    * Collection Space Implementation
    */
   _sdbCollectionSpaceImpl::_sdbCollectionSpaceImpl () :
   _sdbBase( CLIENT_CLASS_CS )
   {
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) ) ;
   }

   _sdbCollectionSpaceImpl::_sdbCollectionSpaceImpl( CHAR *pCollectionSpaceName )
   : _sdbBase( CLIENT_CLASS_CS )
   {
      _setName ( pCollectionSpaceName ) ;
   }

   _sdbCollectionSpaceImpl::~_sdbCollectionSpaceImpl ()
   {
      _dropConnection() ;
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
   }

   INT32 _sdbCollectionSpaceImpl::getCollection ( const CHAR *pCollectionName,
                                                  _sdbCollection **collection,
                                                  BOOLEAN checkExist )
   {
      INT32 rc            = SDB_OK ;
      INT32 version       = CATALOG_INVALID_VERSION ;
      CHAR clFullName[ CLIENT_CL_FULLNAME_SZ + 1 ] = { 0 } ;
      BSONObjBuilder builder ;
      BSONObj query ;
      BSONObj selector ;

      if ( !pCollectionName ||
           ossStrlen ( pCollectionName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' || !collection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                   _collectionSpaceName, pCollectionName ) ;

      if ( fetchCachedVersion( _connection->_getCachedContainer(),
                               clFullName, &version ) )
      {
         // DO NOTHING
      }
      else
      {
         if ( checkExist )
         {
            query     = BSON ( FIELD_NAME_NAME << clFullName ) ;
            selector  = builder.appendNull( FIELD_NAME_VERSION ).obj() ;
            rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION,
                            &query , &selector ) ;
            if ( rc )
            {
               goto error ;
            }
            version = _getRetVersion();

            rc = insertCachedVersion( _connection->_getCachedContainer(),
                                      clFullName, version ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            version = CATALOG_DEFAULT_VERSION ;
         }
      }
      if ( NULL != (*collection) )
      {
         delete *collection ;
         *collection = NULL ;
      }

      *collection = (_sdbCollection*)( new(std::nothrow) sdbCollectionImpl () ) ;
      if ( !*collection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionImpl*)*collection)->_setName ( clFullName ) ;
      ((sdbCollectionImpl*)*collection)->setVersion( version );
      rc = ((sdbCollectionImpl*)*collection)->_setConnection ( _connection ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( collection && NULL != *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::createCollection ( const CHAR *pCollectionName,
                                                     _sdbCollection **collection )
   {
      return createCollection ( pCollectionName, _sdbStaticObject, collection ) ;
   }

   INT32 _sdbCollectionSpaceImpl::createCollection ( const CHAR *pCollectionName,
                                                     const BSONObj &options,
                                                     _sdbCollection **collection )
   {
      INT32 rc            = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      CHAR clFullName[ CLIENT_CL_FULLNAME_SZ + 1 ] = { 0 } ;

      if ( !pCollectionName || !*pCollectionName ||
           ossStrlen ( pCollectionName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' || !collection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                   _collectionSpaceName, pCollectionName ) ;

      ob.append ( FIELD_NAME_NAME, clFullName ) ;
      ob.appendElementsUnique( options ) ;
      newObj = ob.obj () ;

      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }
      *collection = (_sdbCollection*)( new(std::nothrow) sdbCollectionImpl () ) ;
      if ( !*collection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionImpl*)*collection)->_setName ( clFullName ) ;
      ((sdbCollectionImpl*)*collection)->setVersion( _getRetVersion() ) ;
      rc  = ((sdbCollectionImpl*)*collection)->_setConnection ( _connection ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      /// ignore the result
      insertCachedVersion( _connection->_getCachedContainer(),
                           clFullName, _getRetVersion() ) ;

   done :
      return rc ;
   error :
      if ( collection && NULL != *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::dropCollection( const CHAR *pCollectionName,
                                                  const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      BSONObj newObj ;

      CHAR clFullName[ CLIENT_CL_FULLNAME_SZ + 1 ] = { 0 } ;

      if ( !pCollectionName ||
           ossStrlen ( pCollectionName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                   _collectionSpaceName, pCollectionName ) ;

      try
      {
         BSONObjBuilder builder ;
         BSONObjIterator iterOptions( options ) ;
         while ( iterOptions.more() )
         {
            BSONElement ele = iterOptions.next() ;
            // skip name field
            if ( 0 != ossStrcmp( FIELD_NAME_NAME, ele.fieldName() ) )
            {
               builder.append( ele ) ;
            }
         }
         builder.append( FIELD_NAME_NAME, clFullName ) ;
         newObj = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      /// ignore the result
      removeCachedObject( _connection->_getCachedContainer(),
                          clFullName, FALSE ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::listCollections ( _sdbCursor **cursor )
   {
      INT32   rc = SDB_OK ;
      BSONObj condition ;
      CHAR    lowBound[ CLIENT_CS_NAMESZ + 1 + 1 ] = { 0 } ;
      CHAR    upBound[ CLIENT_CS_NAMESZ + 1 + 1 ] = { 0 } ;

      if ( '\0' == _collectionSpaceName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ossStrncpy( lowBound, _collectionSpaceName, CLIENT_CS_NAMESZ ) ;
      ossStrncat( lowBound, ".", 1 ) ;
      ossStrncpy( upBound, _collectionSpaceName, CLIENT_CS_NAMESZ ) ;
      ossStrncat( upBound, "/", 1 ) ;

      // build condition bson
      try
      {
         BSONObjBuilder builder ;
         BSONObjBuilder subBuilder ( builder.subobjStart( FIELD_NAME_NAME ) ) ;
         subBuilder.append( "$gt", lowBound ) ;
         subBuilder.append( "$lt", upBound ) ;
         subBuilder.doneFast() ;
         condition = builder.obj() ;
      }
      catch( const std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }


      rc = _connection->getList( cursor, SDB_LIST_COLLECTIONS, condition ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::create ()
   {
      INT32 rc            = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionSpaceName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( FIELD_NAME_NAME << _collectionSpaceName ) ;

      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      /// ignore the result
      insertCachedObject( _connection->_getCachedContainer(),
                          _collectionSpaceName ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::drop ()
   {
      INT32 rc            = SDB_OK ;
      BSONObj newObj ;

      if ( _collectionSpaceName [0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      newObj = BSON ( FIELD_NAME_NAME << _collectionSpaceName ) ;
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      /// ignore the result
      removeCachedObject( _connection->_getCachedContainer(),
                          _collectionSpaceName, TRUE ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::renameCollection( const CHAR * oldName,
                                                    const CHAR * newName,
                                                    const BSONObj & options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      CHAR clFullName[ CLIENT_CL_FULLNAME_SZ + 1 ] = { 0 } ;

      if( NULL == oldName || NULL == newName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_COLLECTIONSPACE, this->getCSName() ) ;
         queryBuilder.append( FIELD_NAME_OLDNAME, oldName ) ;
         queryBuilder.append( FIELD_NAME_NEWNAME, newName ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      ossSnprintf( clFullName, sizeof( clFullName ), "%s.%s",
                   _collectionSpaceName, oldName ) ;

      rc = _connection->_runCommand( (CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTION),
                                     &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      /// ignore the result
      removeCachedObject( _connection->_getCachedContainer(),
                          clFullName, FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::alterCollectionSpace ( const BSONObj & options )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder bob ;
      BSONElement ele ;
      BSONObj newObj ;

      // check
      if ( '\0' == _collectionSpaceName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         if ( !options.hasField( FIELD_NAME_ALTER ) )
         {
            bob.append( FIELD_NAME_NAME, _collectionSpaceName ) ;
            bob.append( FIELD_NAME_OPTIONS, options ) ;
         }
         else
         {
            bob.append( FIELD_NAME_ALTER_TYPE, SDB_CATALOG_CS ) ;
            bob.append( FIELD_NAME_VERSION, SDB_ALTER_VERSION ) ;
            bob.append( FIELD_NAME_NAME, _collectionSpaceName ) ;

            ele = options.getField( FIELD_NAME_ALTER ) ;
            if ( Object == ele.type() || Array == ele.type() )
            {
               bob.append( ele ) ;
            }
            else
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            ele = options.getField( FIELD_NAME_OPTIONS ) ;
            if ( Object == ele.type() )
            {
               bob.append( ele ) ;
            }
            else if ( EOO != ele.type() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION_SPACE,
                                      &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::setDomain ( const BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_CS_SET_DOMAIN, &options, FALSE ) ;
   }

   INT32 _sdbCollectionSpaceImpl::getDomainName ( CHAR *result,
                                                  INT32 resultLen )
   {
      INT32 rc = SDB_OK ;
      CHAR sql[ CLIENT_SQL_MAX_LEN + CLIENT_CS_NAMESZ + 1 ] = { 0 } ;
      sdbCursor cursor ;
      BSONObj   tempObj ;

      if ( '\0' == _collectionSpaceName[0] || 0 >= resultLen || NULL == result )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      ossMemset( result, 0, resultLen ) ;
      // build sql
      ossSnprintf( sql, CLIENT_SQL_MAX_LEN + CLIENT_CS_NAMESZ,
                   "select Domain from $LIST_CS where Name = '%s'",
                   _collectionSpaceName ) ;

      rc = _connection->exec( sql, cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = cursor.next( tempObj ) ;
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

      if ( jstNULL != tempObj.firstElement().type() )
      {
         ossStrncpy( result,
                     tempObj.getStringField( "Domain" ),
                     resultLen - 1 ) ;
      }
      else
      {
         result[0] = '\0' ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::removeDomain ()
   {
      return _alterInternal( SDB_ALTER_CS_REMOVE_DOMAIN, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionSpaceImpl::enableCapped ()
   {
      return _alterInternal( SDB_ALTER_CS_ENABLE_CAPPED, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionSpaceImpl::disableCapped ()
   {
      return _alterInternal( SDB_ALTER_CS_DISABLE_CAPPED, NULL, TRUE ) ;
   }

   INT32 _sdbCollectionSpaceImpl::setAttributes ( const BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_CS_SET_ATTR, &options, FALSE ) ;
   }

   INT32 _sdbCollectionSpaceImpl::_alterInternal ( const CHAR * taskName,
                                                   const BSONObj * arguments,
                                                   BOOLEAN allowNullArgs )
   {
      INT32 rc = SDB_OK ;

      BSONObj alterObject ;

      rc = _sdbBuildAlterObject( taskName, arguments, allowNullArgs, alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = alterCollectionSpace( alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   /*
    * sdbDomainImpl
    * SequoiaDB Domain Implementation
    */
   _sdbDomainImpl::_sdbDomainImpl () :
   _sdbBase( CLIENT_CLASS_DOMAIN )
   {
      ossMemset( _domainName, 0, sizeof ( _domainName ) ) ;
   }

   _sdbDomainImpl::_sdbDomainImpl ( const CHAR *pDomainName ) :
   _sdbBase( CLIENT_CLASS_DOMAIN )
   {
      _setName( pDomainName ) ;
   }

   _sdbDomainImpl::~_sdbDomainImpl ()
   {
      _dropConnection() ;
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
   }

   INT32 _sdbDomainImpl::_setName ( const CHAR *pDomainName )
   {
      INT32 rc = SDB_OK ;
      if ( ossStrlen ( pDomainName ) > CLIENT_DOMAIN_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ossMemset ( _domainName, 0, sizeof(_domainName) ) ;
      ossStrncpy( _domainName, pDomainName, CLIENT_DOMAIN_NAMESZ ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::alterDomain ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BSONElement ele ;

      ele = options.getField( FIELD_NAME_ALTER ) ;
      if ( EOO == ele.type() )
      {
         rc = _alterDomainV1( options ) ;
      }
      else
      {
         rc = _alterDomainV2( options ) ;
      }

      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::_alterDomainV2 ( const bson::BSONObj & options )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder bob ;
      BSONElement ele ;
      BSONObj newObj ;

      // check
      if ( '\0' == _domainName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         bob.append( FIELD_NAME_ALTER_TYPE, SDB_CATALOG_DOMAIN ) ;
         bob.append( FIELD_NAME_VERSION, SDB_ALTER_VERSION ) ;
         bob.append( FIELD_NAME_NAME, _domainName ) ;

         ele = options.getField( FIELD_NAME_ALTER ) ;
         if ( Object == ele.type() || Array == ele.type() )
         {
            bob.append( ele ) ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         ele = options.getField( FIELD_NAME_OPTIONS ) ;
         if ( Object == ele.type() )
         {
            bob.append( ele ) ;
         }
         else if ( EOO != ele.type() )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }

         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN,
                                     &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::_alterDomainV1 ( const bson::BSONObj & options )
   {
      INT32 rc       = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      if ( '\0' == _domainName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         ob.append ( FIELD_NAME_NAME, _domainName ) ;
         ob.append ( FIELD_NAME_OPTIONS, options ) ;
         newObj = ob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN,
                                     &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::listCollectionSpacesInDomain ( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObjBuilder ob1 ;
      BSONObjBuilder ob2 ;

      if ( '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         ob1.append ( FIELD_NAME_DOMAIN, _domainName ) ;
         condition = ob1.obj () ;
         ob2.appendNull ( FIELD_NAME_NAME ) ;
         selector = ob2.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->getList( cursor, SDB_LIST_CS_IN_DOMAIN,
                                 condition, selector ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::listCollectionsInDomain ( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      BSONObj selector ;
      BSONObjBuilder ob1 ;
      BSONObjBuilder ob2 ;

      if ( '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build bson
      try
      {
         ob1.append ( FIELD_NAME_DOMAIN, _domainName ) ;
         condition = ob1.obj () ;
         ob2.appendNull ( FIELD_NAME_NAME ) ;
         selector = ob2.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->getList( cursor, SDB_LIST_CL_IN_DOMAIN,
                                 condition, selector ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::listReplicaGroupInDomain( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition;
      if ( '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         condition = BSON( FIELD_NAME_NAME << this->getName() ) ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->getList( cursor, SDB_LIST_DOMAINS, condition ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDomainImpl::addGroups ( const BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_DOMAIN_ADD_GROUPS, &options, FALSE ) ;
   }

   INT32 _sdbDomainImpl::setGroups ( const BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_DOMAIN_SET_GROUPS, &options, FALSE ) ;
   }

   INT32 _sdbDomainImpl::removeGroups ( const BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_DOMAIN_REMOVE_GROUPS, &options, FALSE ) ;
   }

   INT32 _sdbDomainImpl::setActiveLocation ( const CHAR* pLocation )
   {
      INT32 rc = SDB_OK ;
      BSONObj option ;

      try
      {
         // build { ActiveLocation: pLocation }
         option = BSON( FIELD_NAME_GROUP_ACTIVE_LOCATION << pLocation ) ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _alterInternal( SDB_ALTER_DOMAIN_SET_ACTIVE_LOCATION, &option, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::setLocation( const CHAR * pHostName, const CHAR * pLocation )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj option ;

      if ( NULL == pHostName || NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // build { HostName: pHostName, Location: pLocation }
         builder.append( FIELD_NAME_HOST, pHostName ) ;
         builder.append( FIELD_NAME_NODE_LOCATION, pLocation ) ;
         option = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _alterInternal( SDB_ALTER_DOMAIN_SET_LOCATION, &option, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDomainImpl::setAttributes ( const bson::BSONObj & options )
   {
      return _alterInternal( SDB_ALTER_DOMAIN_SET_ATTR, &options, FALSE ) ;
   }

   INT32 _sdbDomainImpl::_alterInternal ( const CHAR * taskName,
                                          const BSONObj * arguments,
                                          BOOLEAN allowNullArgs )
   {
      INT32 rc = SDB_OK ;

      BSONObj alterObject ;

      rc = _sdbBuildAlterObject( taskName, arguments,
                                 allowNullArgs, alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = alterDomain( alterObject ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   /*
    * sdbDataCenterImpl
    * SequoiaDB Data Center Implementation
    */
   _sdbDataCenterImpl::_sdbDataCenterImpl () :
   _sdbBase( CLIENT_CLASS_DC )
   {
      ossMemset( _dcName, 0, sizeof ( _dcName ) ) ;
   }

   _sdbDataCenterImpl::~_sdbDataCenterImpl ()
   {
      _dropConnection() ;
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
   }

   INT32 _sdbDataCenterImpl::_setName ( const CHAR *pClusterName,
                                        const CHAR *pBusinessName )
   {
      INT32 rc = SDB_OK ;
                          ;
      if ( ossStrlen( pClusterName ) + ossStrlen( pBusinessName ) + 1 >
           CLIENT_DC_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemset ( _dcName, 0, sizeof(_dcName) ) ;
      ossSnprintf( _dcName, CLIENT_DC_NAMESZ, "%s:%s",
                   pClusterName, pBusinessName ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDataCenterImpl::_innerAlter( const CHAR *pValue,
                                          const bson::BSONObj *pInfo )
   {
      INT32 rc            = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      // check
      if ( NULL == pValue )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // build obj to send
      bob.append( FIELD_NAME_ACTION, pValue ) ;
      if ( NULL != pInfo )
      {
         bob.append( FIELD_NAME_OPTIONS, *pInfo ) ;
      }
      newObj = bob.obj() ;

      // run command
      rc = _connection->_runCommand( pCommand, &newObj, NULL, NULL, NULL,
                                     0, 0, 0, -1, NULL ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDataCenterImpl::getDetail( bson::BSONObj &retInfo )
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pCommand      = CMD_ADMIN_PREFIX CMD_NAME_GET_DCINFO ;
      _sdbCursor *retInfoCursor = NULL ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( pCommand, NULL, NULL, NULL, NULL,
                                     0, 0, 0, -1, &retInfoCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get dc detail
      rc = retInfoCursor->next( retInfo ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( NULL != retInfoCursor )
      {
         delete retInfoCursor ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbDataCenterImpl::activateDC()
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_ACTIVATE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::deactivateDC()
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_DEACTIVATE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::enableReadOnly( BOOLEAN isReadOnly )
   {
      INT32 rc = SDB_OK ;
      if ( TRUE == isReadOnly )
      {
         rc = _innerAlter( CMD_VALUE_NAME_ENABLE_READONLY, NULL ) ;
      }
      else
      {
         rc = _innerAlter( CMD_VALUE_NAME_DISABLE_READONLY, NULL ) ;
      }
      return rc ;
   }

   INT32 _sdbDataCenterImpl::createImage( const CHAR *pCataAddrList )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      // check
      if ( NULL == pCataAddrList )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build obj
      newObj = BSON( FIELD_NAME_ADDRESS << pCataAddrList ) ;

      // run command
      rc = _innerAlter( CMD_VALUE_NAME_CREATE, &newObj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDataCenterImpl::removeImage()
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_REMOVE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::enableImage()
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_ENABLE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::disableImage()
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_DISABLE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::attachGroups( const bson::BSONObj &info )
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_ATTACH, &info ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::detachGroups( const bson::BSONObj &info )
   {
      INT32 rc = _innerAlter( CMD_VALUE_NAME_DETACH, &info ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::setActiveLocation( const CHAR *pLocation )
   {
      INT32 rc = SDB_OK ;

      BSONObjBuilder builder ;
      BSONObj option ;

      if ( NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // build { ActiveLocation: pLocation }
         builder.append( FIELD_NAME_GROUP_ACTIVE_LOCATION, pLocation ) ;
         option = builder.obj() ;
      }
      catch( const std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( CMD_VALUE_NAME_SET_ACTIVE_LOCATION, &option ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDataCenterImpl::setLocation( const CHAR * pHostName, const CHAR * pLocation )
   {
      INT32 rc = SDB_OK ;

      BSONObjBuilder builder ;
      BSONObj option ;

      if ( NULL == pHostName || NULL == pLocation )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         // build { HostName: pHostName, Location: pLocation }
         builder.append( FIELD_NAME_HOST, pHostName ) ;
         builder.append( FIELD_NAME_NODE_LOCATION, pLocation ) ;
         option = builder.obj() ;
      }
      catch( const std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( CMD_VALUE_NAME_SET_LOCATION, &option ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDataCenterImpl::startMaintenanceMode( const BSONObj & options )
   {
      return _innerAlter( CMD_VALUE_NAME_START_MAINTENANCE_MODE, &options ) ;
   }

   INT32 _sdbDataCenterImpl::stopMaintenanceMode( const BSONObj & options )
   {
      return _innerAlter( CMD_VALUE_NAME_STOP_MAINTENANCE_MODE, &options ) ;
   }

   /*
    * _sdbRecycleBinImpl
    * SequoiaDB Recycle Bin Implementation
    */
   _sdbRecycleBinImpl::_sdbRecycleBinImpl()
   : _sdbBase( CLIENT_CLASS_RB )
   {
   }

   _sdbRecycleBinImpl::~_sdbRecycleBinImpl()
   {
      _dropConnection() ;
   }

   INT32 _sdbRecycleBinImpl::_innerAlter( const BSONObj &options )
   {
      return _innerCMD( CMD_ADMIN_PREFIX CMD_NAME_ALTER_RECYCLEBIN, options ) ;
   }

   INT32 _sdbRecycleBinImpl::_innerCMD( const CHAR *command,
                                        const BSONObj &options,
                                        _sdbCursor **cursor )
   {
      INT32 rc             = SDB_OK ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( command, &options, NULL, NULL, NULL,
                                     0, 0, 0, -1, cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::getDetail( BSONObj &retInfo )
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pCommand      =
            CMD_ADMIN_PREFIX CMD_NAME_GET_RECYCLEBIN_DETAIL ;
      _sdbCursor *retInfoCursor = NULL ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( pCommand, NULL, NULL, NULL, NULL,
                                     0, 0, 0, -1, &retInfoCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get dc detail
      rc = retInfoCursor->next( retInfo ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      if ( NULL != retInfoCursor )
      {
         delete retInfoCursor ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::enable()
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ACTION,
                         CMD_VALUE_NAME_RECYCLEBIN_ENABLE ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( cmdOptions ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::disable()
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ACTION,
                         CMD_VALUE_NAME_RECYCLEBIN_DISABLE ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( cmdOptions ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::setAttributes( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_ACTION,
                         CMD_VALUE_NAME_RECYCLEBIN_SETATTR ) ;
         builder.append( FIELD_NAME_OPTIONS, options ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerAlter( cmdOptions ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::alter( const BSONObj &options )
   {
      return setAttributes( options ) ;
   }

   INT32 _sdbRecycleBinImpl::returnItem( const CHAR *recycleName,
                                         const BSONObj &options,
                                         BSONObj *result )
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;
      _sdbCursor *resultCursor = NULL ;

      if ( NULL == recycleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         // skip recycle name field
         BSONObjIterator iterOptions( options ) ;
         while ( iterOptions.more() )
         {
            BSONElement ele = iterOptions.next() ;
            if ( 0 != ossStrcmp( FIELD_NAME_RECYCLE_NAME,
                                 ele.fieldName() ) )
            {
               builder.append( ele ) ;
            }
         }
         builder.append( FIELD_NAME_RECYCLE_NAME, recycleName ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerCMD( CMD_ADMIN_PREFIX CMD_NAME_RETURN_RECYCLEBIN_ITEM,
                      cmdOptions, &resultCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL != result )
      {
         rc = resultCursor->next( *result ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      if ( NULL != resultCursor )
      {
         delete resultCursor ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::returnItemToName( const CHAR *recycleName,
                                               const CHAR *returnName,
                                               const BSONObj &options,
                                               BSONObj *result )
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;
      _sdbCursor *resultCursor = NULL ;

      if ( NULL == recycleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL == returnName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         // skip recycle name fields
         BSONObjIterator iterOptions( options ) ;
         while ( iterOptions.more() )
         {
            BSONElement ele = iterOptions.next() ;
            const CHAR *fieldName = ele.fieldName() ;
            if ( ( 0 != ossStrcmp( FIELD_NAME_RECYCLE_NAME, fieldName ) ) &&
                 ( 0 != ossStrcmp( FIELD_NAME_RETURN_NAME, fieldName ) ) )
            {
               builder.append( ele ) ;
            }
         }
         builder.append( FIELD_NAME_RECYCLE_NAME, recycleName ) ;
         builder.append( FIELD_NAME_RETURN_NAME, returnName ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerCMD( CMD_ADMIN_PREFIX CMD_NAME_RETURN_RECYCLEBIN_ITEM_TO_NAME,
                      cmdOptions, &resultCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL != result )
      {
         rc = resultCursor->next( *result ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      if ( NULL != resultCursor )
      {
         delete resultCursor ;
      }
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::dropItem( const CHAR *recycleName,
                                       const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;

      if ( NULL == recycleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         if ( !options.isEmpty() )
         {
            // skip recycle name field
            BSONObjIterator iterOptions( options ) ;
            while ( iterOptions.more() )
            {
               BSONElement ele = iterOptions.next() ;
               if ( 0 != ossStrcmp( FIELD_NAME_RECYCLE_NAME,
                                    ele.fieldName() ) )
               {
                  builder.append( ele ) ;
               }
            }
         }
         builder.append( FIELD_NAME_RECYCLE_NAME, recycleName ) ;
         cmdOptions = builder.obj() ;
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerCMD( CMD_ADMIN_PREFIX CMD_NAME_DROP_RECYCLEBIN_ITEM,
                      cmdOptions ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::dropAll( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      BSONObj cmdOptions ;

      try
      {
         if ( !options.isEmpty() )
         {
            BSONObjBuilder builder ;
            // skip recycle name field
            BSONObjIterator iterOptions( options ) ;
            while ( iterOptions.more() )
            {
               BSONElement ele = iterOptions.next() ;
               if ( 0 != ossStrcmp( FIELD_NAME_RECYCLE_NAME,
                                    ele.fieldName() ) )
               {
                  builder.append( ele ) ;
               }
            }
            cmdOptions = builder.obj() ;
         }
      }
      catch ( exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _innerCMD( CMD_ADMIN_PREFIX CMD_NAME_DROP_RECYCLEBIN_ALL,
                      cmdOptions ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::list( _sdbCursor **cursor,
                                   const BSONObj &condition,
                                   const BSONObj &selector,
                                   const BSONObj &orderBy,
                                   const BSONObj &hint,
                                   INT64 numToSkip,
                                   INT64 numToReturn )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_LIST_RECYCLEBIN ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( pCommand,
                                     &condition,
                                     &selector,
                                     &orderBy,
                                     &hint,
                                     0,
                                     0,
                                     numToSkip,
                                     numToReturn,
                                     cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::snapshot( _sdbCursor **cursor,
                                       const BSONObj &condition,
                                       const BSONObj &selector,
                                       const BSONObj &orderBy,
                                       const BSONObj &hint,
                                       INT64 numToSkip,
                                       INT64 numToReturn )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RECYCLEBIN ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( pCommand,
                                     &condition,
                                     &selector,
                                     &orderBy,
                                     &hint,
                                     0,
                                     0,
                                     numToSkip,
                                     numToReturn,
                                     cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;

   error:
      goto done ;
   }

   INT32 _sdbRecycleBinImpl::getCount( INT64 &count,
                                       const BSONObj &condition )
   {
      INT32 rc             = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_GET_RECYCLEBIN_COUNT ;
      BSONObj countObj ;
      _sdbCursor *cursor = NULL ;

      // check
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand( pCommand,
                                     &condition,
                                     NULL,
                                     NULL,
                                     NULL,
                                     0,
                                     0,
                                     0,
                                     -1,
                                     &cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // check return cursor
      if ( NULL == cursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // there should only 1 record read
      rc = cursor->next( countObj ) ;
      if ( SDB_OK != rc )
      {
         // if we didn't read anything, let's return unexpected
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_UNEXPECTED_RESULT ;
         }
         goto error ;
      }
      else
      {
         try
         {
            BSONElement ele = countObj.getField( FIELD_NAME_TOTAL ) ;
            if ( ele.type() != NumberLong )
            {
               rc = SDB_UNEXPECTED_RESULT ;
            }
            else
            {
               count = ele.numberLong() ;
            }
         }
         catch ( exception &e )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }

   done:
      if ( NULL != cursor )
      {
         delete( cursor ) ;
      }
      return rc ;

   error:
      goto done ;
   }

   /*
    * sdbLobImpl
    * SequoiaDB large object Implementation
    */
   _sdbLobImpl::_sdbLobImpl () :
   _sdbBase( CLIENT_CLASS_LOB ),
   _isOpen( FALSE ),
   _contextID ( -1 ),
   _mode( -1 ),
   _createTime( -1 ),
   _modificationTime( -1 ),
   _lobSize( -1 ),
   _currentOffset( 0 ),
   _cachedOffset( 0 ),
   _cachedSize( 0 ),
   _pageSize( 0 ),
   _flag( 0 ),
   _piecesInfoNum( 0 ),
   _dataCache( NULL )
   {
      _oid = OID() ;
   }

   _sdbLobImpl::~_sdbLobImpl ()
   {
      if ( _connection )
      {
         if ( _isOpen )
         {
            close() ;
         }
         _dropConnection() ;
      }
      if ( _pSendBuffer )
      {
         SAFE_OSS_FREE ( _pSendBuffer ) ;
         _sendBufferSize = 0 ;
      }
      if ( _pReceiveBuffer )
      {
         SAFE_OSS_FREE ( _pReceiveBuffer ) ;
         _receiveBufferSize = 0 ;
      }
   }

   void _sdbLobImpl::_close()
   {
      // 1. we are not going to release send/receive buffer,
      // let destructor do it
      // 2. we are not set _connection to be null here, let destructor
      // do it, for we still need to use the lock which is kept in _connection
      _isOpen = FALSE ;
      _contextID = -1 ;
      _mode = -1 ;
      _cachedOffset = 0 ;
      _cachedSize = 0 ;
      _dataCache = NULL ;
   }

   BOOLEAN _sdbLobImpl::_dataCached()
   {
      // "lob->_currentOffset" may be changed by seek(),
      // so take care of it
      return ( NULL != _dataCache && 0 < _cachedSize &&
               0 <= _cachedOffset &&
               _cachedOffset <= _currentOffset &&
               _currentOffset < ( _cachedOffset + _cachedSize ) ) ;
   }

   void _sdbLobImpl::_readInCache( void *buf, UINT32 len, UINT32 *read )
   {
      const CHAR *cache = NULL ;
      UINT32 readInCache = _cachedOffset + _cachedSize - _currentOffset ;
      readInCache = readInCache <= len ?
                    readInCache : len ;
      cache = _dataCache + _currentOffset - _cachedOffset ;
      ossMemcpy( buf, cache, readInCache ) ;
      _cachedSize -= readInCache + cache - _dataCache ;

      if ( 0 == _cachedSize )
      {
         _dataCache = NULL ;
         _cachedOffset = -1 ;
      }
      else
      {
         _dataCache = cache + readInCache ;
         _cachedOffset = readInCache + _currentOffset ;
      }

      *read = readInCache ;
      return ;
   }

   UINT32 _sdbLobImpl::_reviseReadLen( UINT32 needLen )
   {
      UINT32 pageSize = _pageSize ;
      UINT32 mod = _currentOffset & ( pageSize - 1 ) ;
      UINT32 alignedLen = ossRoundUpToMultipleX( needLen,
                                                 LOB_ALIGNED_LEN ) ;
      alignedLen -= mod ;
      if ( alignedLen < LOB_ALIGNED_LEN )
      {
         alignedLen += LOB_ALIGNED_LEN ;
      }
      return alignedLen ;
   }

   INT32 _sdbLobImpl::_onceRead( CHAR *buf, UINT32 len, UINT32 *read )
   {
      INT32 rc                 = SDB_OK ;
      BOOLEAN locked           = FALSE ;
      UINT32 needRead          = len ;
      UINT32 totalRead         = 0 ;
      CHAR *localBuf           = buf ;
      UINT32 onceRead          = 0 ;
      const MsgOpReply *reply  = NULL ;
      const MsgLobTuple *tuple = NULL ;
      const CHAR *body         = NULL ;
      UINT32 alignedLen        = 0 ;
      SINT64 contextID         = -1 ;

      if ( _dataCached() )
      {
         _readInCache( localBuf, needRead, &onceRead ) ;

         totalRead += onceRead ;
         needRead -= onceRead ;
         _currentOffset += onceRead ;
         localBuf += onceRead ;
         *read = totalRead ;
         goto done ;
      }

      _cachedOffset = -1 ;
      _cachedSize = 0 ;
      _dataCache = NULL ;

      alignedLen = _reviseReadLen( needRead ) ;

      rc = clientBuildReadLobMsg( &_pSendBuffer, &_sendBufferSize,
                                  alignedLen, _currentOffset,
                                  0, _contextID, 0,
                                  _connection->_endianConvert ) ;
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      reply = ( const MsgOpReply * )( _pReceiveBuffer ) ;
      if ( ( UINT32 )( reply->header.messageLength ) <
           ( sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) ) )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      tuple = ( const MsgLobTuple *)
              ( _pReceiveBuffer + sizeof( MsgOpReply ) ) ;
      if ( _currentOffset != tuple->columns.offset )
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

      body = _pReceiveBuffer + sizeof( MsgOpReply ) + sizeof( MsgLobTuple ) ;

      if ( needRead < tuple->columns.len )
      {
         ossMemcpy( localBuf, body, needRead ) ;
         totalRead += needRead ;
         _currentOffset += needRead ;
         _cachedOffset = _currentOffset ;
         _cachedSize = tuple->columns.len - needRead ;
         _dataCache = body + needRead ;
      }
      else
      {
         ossMemcpy( localBuf, body, tuple->columns.len ) ;
         totalRead += tuple->columns.len ;
         _currentOffset += tuple->columns.len ;
         _cachedOffset = -1 ;
         _cachedSize = 0 ;
         _dataCache = NULL ;
      }

      *read = totalRead ;
   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbLobImpl::close ()
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BOOLEAN locked = FALSE ;
      const MsgOpReply* reply = NULL ;

      // check wether the lob had been close or not
      if ( !_isOpen || -1 == _contextID )
      {
         goto done ;
      }
      // check
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      // build msg
      rc = clientBuildCloseLobMsg( &_pSendBuffer, &_sendBufferSize,
                                   0, 1, _contextID, 0,
                                   _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg from engine
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      // release the resource hold in _sdbLobImpl
      // and cleanup data member of this object then
      // set this lob to be close
      _close() ;

      reply = ( const MsgOpReply * )( _pReceiveBuffer ) ;
      if ( reply->numReturned > 0 &&
           (UINT32)reply->header.messageLength >
           ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
      {
         // get reply bson from received msg
         const CHAR* bsonBuf = _pReceiveBuffer + sizeof( MsgOpReply ) ;
         try
         {
            BSONObj obj = BSONObj( bsonBuf ) ;
            BSONElement ele = obj.getField( FIELD_NAME_LOB_MODIFICATION_TIME ) ;
            if ( NumberLong == ele.type() )
            {
               _modificationTime = (UINT64) ele.numberLong() ;
            }
         }
         catch ( std::exception )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      if ( SDB_OK == rc )
      {
         // unregister anyway
         _dropConnection() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbLobImpl::read ( UINT32 len, CHAR *buf, UINT32 *read )
   {
      INT32 rc = SDB_OK ;
      UINT32 needRead = len ;
      CHAR *localBuf = buf ;
      UINT32 onceRead = 0 ;
      UINT32 totalRead = 0 ;

      // check
      if ( !_connection && !_isOpen )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      if ( !_isOpen )
      {
         rc = SDB_LOB_NOT_OPEN ;
         goto error ;
      }
      // check argument
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( (!isLobReadOnlyMode(_mode) && !isLobReadWriteMode(_mode)) || -1 == _contextID)
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 == len )
      {
         *read = 0 ;
         goto done ;
      }

      if ( _currentOffset == _lobSize )
      {
         rc = SDB_EOF ;
         goto error ;
      }

      while ( 0 < needRead && _currentOffset < _lobSize )
      {
         rc = _onceRead( localBuf, needRead, &onceRead ) ;
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

   INT32 _sdbLobImpl::write ( const CHAR *buf, UINT32 len )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BOOLEAN locked = FALSE ;
      UINT32 totalLen = 0 ;
      const UINT32 maxSendLen = 2 * 1024 * 1024 ;

      // check
      if ( !_connection && !_isOpen )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      if ( !_isOpen )
      {
         rc = SDB_LOB_NOT_OPEN ;
         goto error ;
      }
      if ( -1 == _contextID )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // check argument
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( SDB_LOB_CREATEONLY != _mode && !hasLobWriteMode( _mode ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if (isLobReadWriteMode( _mode ) )
      {
         //clean the read cache
         _cachedOffset = -1 ;
         _cachedSize = 0 ;
         _dataCache = NULL ;
      }

      if ( 0 == len )
      {
         goto done ;
      }
      // build msg
      do
      {
         INT64 offset = _currentOffset ;
         UINT32 sendLen = maxSendLen <= len - totalLen ?
                          maxSendLen : len - totalLen ;
         rc = clientBuildWriteLobMsg( &_pSendBuffer, &_sendBufferSize,
                                      buf + totalLen, sendLen, offset, 0, 1,
                                      _contextID, 0,
                                      _connection->_endianConvert ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         _connection->lock() ;
         locked = TRUE ;
         // send msg
         rc = _connection->_send ( _pSendBuffer ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         // receive and extract msg from engine
         rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                          contextID ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         // check return msg header
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
         locked = FALSE ;
         _connection->unlock() ;

         totalLen += sendLen ;

         _currentOffset += sendLen ;
         _lobSize = OSS_MAX( _lobSize, _currentOffset ) ;
      } while ( totalLen < len ) ;

   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbLobImpl::seek ( SINT64 size, SDB_LOB_SEEK whence )
   {
      INT32 rc = SDB_OK ;

      // check
      if ( !_connection && !_isOpen )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if (  !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      if ( !_isOpen )
      {
         rc = SDB_LOB_NOT_OPEN ;
         goto error ;
      }
      if ( -1 == _contextID )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !isLobReadOnlyMode( _mode) &&
           SDB_LOB_CREATEONLY != _mode &&
           !hasLobWriteMode( _mode ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // set seek point
      if ( SDB_LOB_SEEK_SET == whence )
      {
         if ( size < 0 || ( _lobSize < size && SDB_LOB_READ == _mode ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _currentOffset = size ;
      }
      else if ( SDB_LOB_SEEK_CUR == whence )
      {
         if ( ( _lobSize < size + _currentOffset && SDB_LOB_READ == _mode ) ||
              size + _currentOffset < 0 )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _currentOffset += size ;
      }
      else if ( SDB_LOB_SEEK_END == whence )
      {
         if ( size < 0 || ( _lobSize < size && SDB_LOB_READ == _mode ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         _currentOffset = _lobSize - size ;
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

   INT32 _sdbLobImpl::lock( INT64 offset, INT64 length )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      INT64 contextID = -1 ;

      if ( !_connection && !_isOpen )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if (  !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      if ( !_isOpen )
      {
         rc = SDB_LOB_NOT_OPEN ;
         goto error ;
      }
      if ( -1 == _contextID )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( offset < 0 || length < -1 || length == 0)
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( !hasLobWriteMode( _mode ) && SDB_LOB_SHAREREAD != _mode )
      {
         goto done ;
      }

      rc = clientBuildLockLobMsg( &_pSendBuffer, &_sendBufferSize,
                                  offset, length, 0, 1,
                                  _contextID, 0,
                                  _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg from engine
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      _connection->unlock() ;
      locked = FALSE ;

   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbLobImpl::lockAndSeek( INT64 offset, INT64 length )
   {
      INT32 rc = SDB_OK ;

      rc = lock( offset, length ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = seek( offset, SDB_LOB_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbLobImpl::isClosed( BOOLEAN &flag )
   {
      flag = isClosed();
      return SDB_OK ;
   }

   INT32 _sdbLobImpl::getOid( bson::OID &oid )
   {
      oid = getOid() ;
      return SDB_OK ;
   }

   INT32 _sdbLobImpl::getSize( SINT64 *size )
   {
      if ( NULL == size )
      {
         return SDB_INVALIDARG ;
      }
      *size = getSize() ;
      return SDB_OK ;
   }

   INT32 _sdbLobImpl::getCreateTime ( UINT64 *millis )
   {
      if ( NULL == millis )
      {
         return SDB_INVALIDARG ;
      }
      *millis = getCreateTime() ;
      return SDB_OK ;
   }

   BOOLEAN _sdbLobImpl::isClosed()
   {
      return !_isOpen ;
   }

   bson::OID _sdbLobImpl::getOid()
   {
      return _oid ;
   }

   SINT64 _sdbLobImpl::getSize()
   {
      return _lobSize ;
   }

   UINT64 _sdbLobImpl::getCreateTime ()
   {
      return _createTime ;
   }

   UINT64 _sdbLobImpl::getModificationTime()
   {
      return _modificationTime ;
   }

   INT32 _sdbLobImpl::getPiecesInfoNum()
   {
      return _piecesInfoNum ;
   }

   bson::BSONArray _sdbLobImpl::getPiecesInfo()
   {
      return _piecesInfo ;
   }

   BOOLEAN _sdbLobImpl::isEof()
   {
      return _currentOffset >= _lobSize ;
   }

   INT32 _sdbLobImpl::getRunTimeDetail( bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      SINT64 contextID = -1 ;

      // check
      if ( !_connection && !_isOpen )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      if ( !_isOpen )
      {
         rc = SDB_LOB_NOT_OPEN ;
         goto error ;
      }
      if ( -1 == _contextID )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build msg
      rc = clientBuildGetLobRTimeMsg( &_pSendBuffer, &_sendBufferSize,
                                      0, 1, _contextID, 0,
                                      _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      // send msg
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // receive and extract msg from engine
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      locked = FALSE ;
      _connection->unlock() ;

      try
      {
         detail = BSONObj( _pReceiveBuffer + sizeof( MsgOpReply ) ) ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

   done:
      if ( locked )
      {
         _connection->unlock() ;
      }
      return rc ;
   error:
      goto done ;
   }

   /*
    * sdbSequenceImpl
    * Collection Space Implementation
    */
   _sdbSequenceImpl::_sdbSequenceImpl () :
   _sdbBase( CLIENT_CLASS_SQ ),
   _pSequenceName ( NULL )
   {
   }

   _sdbSequenceImpl::~_sdbSequenceImpl ()
   {
      _dropConnection() ;
      if ( _pSequenceName )
      {
         SDB_OSS_FREE ( _pSequenceName ) ;
      }
   }

   INT32 _sdbSequenceImpl::_setName ( const CHAR *pSequenceName )
   {
      INT32 rc       = SDB_OK ;
      UINT32 nameLen = 0 ;

      if ( !pSequenceName || !pSequenceName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _pSequenceName )
      {
         SDB_OSS_FREE ( _pSequenceName ) ;
      }

      nameLen = ossStrlen ( pSequenceName ) ;
      _pSequenceName = ( CHAR * ) SDB_OSS_MALLOC ( nameLen + 1 ) ;
      if ( NULL == _pSequenceName )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      ossStrncpy( _pSequenceName, pSequenceName, nameLen + 1 );
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbSequenceImpl::setAttributes ( const bson::BSONObj &options )
   {
      return _alterInternal ( CMD_VALUE_NAME_SETATTR, options ) ;
   }

   INT32 _sdbSequenceImpl::_alterInternal ( const CHAR *actionName,
                                            const BSONObj &arguments )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         bob.append( FIELD_NAME_ACTION, actionName ) ;
         BSONObjBuilder subBob( bob.subobjStart( FIELD_NAME_OPTIONS ) ) ;
         subBob.append( FIELD_NAME_NAME, _pSequenceName ) ;
         if ( arguments.hasField( FIELD_NAME_NAME ) )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         subBob.appendElements( arguments ) ;
         subBob.done() ;
         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      // run command
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_SEQUENCE,
                                      &newObj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
     return rc ;
   error:
     goto done ;
   }

   INT32 _sdbSequenceImpl::getNextValue ( INT64 &value )
   {
      INT32 rc = SDB_OK ;
      INT32 returnNum = 0 ;
      INT32 increment = 0 ;

      rc = fetch( 1, value, returnNum, increment ) ;
      if ( rc != SDB_OK )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbSequenceImpl::getCurrentValue ( INT64 &value )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;
      _sdbCursor *pCursor = NULL ;
      BSONObj result ;
      BSONElement ele ;

      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         newObj = BSON( FIELD_NAME_NAME << _pSequenceName ) ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_GET_SEQ_CURR_VAL,
                                      &newObj, NULL, NULL, NULL,
                                      0, 0, 0, -1, &pCursor ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( NULL == pCursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // there should only 1 record read
      rc = pCursor->next( result ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_UNEXPECTED_RESULT ;
      }
      if ( rc )
      {
         goto error ;
      }

      ele = result.getField( FIELD_NAME_CURRENT_VALUE ) ;
      if ( !ele.isNumber() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }
      value = ele.numberLong() ;

   done:
      if ( NULL != pCursor )
      {
         delete( pCursor ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbSequenceImpl::setCurrentValue ( const INT64 value )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder bob ;

      try
      {
         bob.append( FIELD_NAME_EXPECT_VALUE, value ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _alterInternal ( CMD_VALUE_NAME_SET_CURR_VALUE, obj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbSequenceImpl::fetch( const INT32 fetchNum, INT64 &nextValue,
                                  INT32 &returnNum, INT32 &increment )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj obj ;
      BSONElement ele ;

      if ( fetchNum < 1 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = clientBuildSeqFetchMsgCpp( &_connection->_pSendBuffer,
                                      &_connection->_sendBufferSize,
                                      _pSequenceName, fetchNum, 0,
                                      _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _connection->_sendAndRecv( _connection->_pSendBuffer,
                                      &_connection->_pReceiveBuffer,
                                      &_connection->_receiveBufferSize,
                                      &pCursor ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( NULL == pCursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      // there should only 1 record read
      rc = pCursor->next( obj ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_UNEXPECTED_RESULT ;
      }
      if ( rc )
      {
         goto error ;
      }

      ele = obj.getField( FIELD_NAME_NEXT_VALUE ) ;
      if ( !ele.isNumber() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }
      nextValue = ele.numberLong() ;

      ele = obj.getField( FIELD_NAME_RETURN_NUM ) ;
      if ( !ele.isNumber() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }
      returnNum = ele.numberInt() ;

      ele = obj.getField( FIELD_NAME_INCREMENT ) ;
      if ( !ele.isNumber() )
      {
         rc = SDB_UNEXPECTED_RESULT ;
         goto error ;
      }
      increment = ele.numberInt() ;

   done:
      if ( NULL != pCursor )
      {
         delete( pCursor ) ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbSequenceImpl::restart ( const INT64 startValue )
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder bob ;

      try
      {
         bob.append( FIELD_NAME_START_VALUE, startValue ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _alterInternal ( CMD_VALUE_NAME_RESTART, obj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   _sdbDataSourceImpl::_sdbDataSourceImpl()
   : _sdbBase( CLIENT_CLASS_DS )
   {
      ossMemset( _dataSourceName, 0, sizeof( _dataSourceName ) ) ;
   }

   _sdbDataSourceImpl::_sdbDataSourceImpl( const CHAR *pDataSourceName )
   : _sdbBase( CLIENT_CLASS_DS )
   {
      _setName( pDataSourceName ) ;
   }

   _sdbDataSourceImpl::~_sdbDataSourceImpl()
   {
      _dropConnection() ;
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE( _pReceiveBuffer ) ;
      }
   }

   INT32 _sdbDataSourceImpl::_setName( const CHAR *pDataSourceName )
   {
      INT32 rc = SDB_OK ;
      if ( ossStrlen( pDataSourceName ) > CLIENT_DATASOURCE_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ossMemset( _dataSourceName, 0, sizeof( _dataSourceName ) ) ;
      ossStrncpy( _dataSourceName, pDataSourceName, CLIENT_DATASOURCE_NAMESZ ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDataSourceImpl::_appendOptions( BSONObjBuilder &builder,
                                             const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjIterator itr( options ) ;
         BSONObjBuilder
            subBuilder( builder.subobjStart( FIELD_NAME_OPTIONS ) ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next() ;
            if ( 0 == ossStrcmp( ele.fieldName(), FIELD_NAME_PASSWD ) )
            {
               CHAR md5[ SDB_MD5_VALUE_BUF_LEN ] = { 0 } ;
               rc = md5Encrypt( ele.valuestrsafe(), md5,
                                SDB_MD5_VALUE_BUF_LEN ) ;
               if ( rc )
               {
                  goto error ;
               }
               subBuilder.append( FIELD_NAME_PASSWD, md5 ) ;
            }
            else
            {
               subBuilder.append( ele ) ;
            }
         }
         subBuilder.done() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbDataSourceImpl::alterDataSource( const BSONObj& options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONElement ele ;
      BSONObj newObj ;
      const CHAR *newName = NULL ;

      if ( 0 == ossStrlen( _dataSourceName ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      try
      {
         // If the user change the name of the data source, the name in this
         // object needs to be updated.
         BSONElement nameEle = options.getField( FIELD_NAME_NAME ) ;
         if ( !nameEle.eoo() )
         {
            if ( String != nameEle.type() )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
            newName = nameEle.valuestr() ;
            if ( 0 == ossStrlen( newName ) )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }
         }

         bob.append( FIELD_NAME_NAME, _dataSourceName ) ;
         rc = _appendOptions( bob, options ) ;
         if ( rc )
         {
            goto error ;
         }
         newObj = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_ALTER_DATASOURCE,
                                     &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( newName && ( 0 != ossStrcmp( _dataSourceName, newName ) ) )
      {
         ossStrncpy( _dataSourceName, newName, CLIENT_DATASOURCE_NAMESZ ) ;
         _dataSourceName[ ossStrlen(newName) ] = '\0' ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   /*
    * sdbImpl
    * SequoiaDB Connection Implementation
    */
   _sdbImpl::_sdbImpl ( BOOLEAN useSSL ) :
   _sock ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
   _useSSL ( useSSL ),
   _tb ( NULL ),
   _authVersion( 0 ),
   _peerProtocolVersion( SDB_PROTOCOL_VER_INVALID ),
   _msgConvertor( NULL ),
   _attributeCache ()
   {
      _pErrorBuf = NULL ;
      _errorBufSize = 0 ;
      _pResultBuf = NULL ;
      _resultBufSize = 0 ;
      ossMemset ( _address, 0, sizeof(_address) ) ;

      initHashTable( &_tb ) ;
      // get current time
      ossGetCurrentTime(_lastAliveTime) ;

      _isOldVersionLobServer = FALSE ;
   }

   _sdbImpl::~_sdbImpl ()
   {
      _removeObjects() ;
      if ( NULL != _tb )
      {
         releaseHashTable( &_tb ) ;
      }
      if ( _sock )
      {
         _disconnect () ;
      }
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
      if ( _msgConvertor )
      {
         delete _msgConvertor ;
      }
   }

   /**
    * remove the connection handle in the associated objects
    */
   void _sdbImpl::_removeObjects ()
   {
      std::set<ossValuePtr> copySet ;
      std::set<ossValuePtr>::iterator it ;

      // remove handles
      // when we remove element in the set, we should copy the set first,
      // and then traverse the copy, for we need to remove elements in the
      // original set when we call _dropConnection

      // release cursors
      copySet = _cursors ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbCursorImpl *)(*it))->_dropConnection () ;
      }
      // release collections
      copySet = _collections ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbCollectionImpl *)(*it))->_dropConnection () ;
      }
      // release collection spaces
      copySet = _collectionspaces ;
      for ( it = copySet.begin(); it != copySet.end(); ++it)
      {
         ((_sdbCollectionSpaceImpl *)(*it))->_dropConnection () ;
      }
      // release nodes
      copySet = _nodes ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbNodeImpl *)(*it))->_dropConnection () ;
      }
      // release _replicaGroups
      copySet = _replicaGroups ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbReplicaGroupImpl *)(*it))->_dropConnection () ;
      }
      // release _domains
      copySet = _domains ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbDomainImpl *)(*it))->_dropConnection () ;
      }
      // release data center
      copySet = _dataCenters ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbDataCenterImpl *)(*it))->_dropConnection () ;
      }
      // release lobs
      copySet = _lobs ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbLobImpl *)(*it))->_dropConnection () ;
      }
      // release sequences
      copySet = _sequences ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbSequenceImpl *)(*it))->_dropConnection () ;
      }
      // release data sources
      copySet = _dataSources ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbDataSourceImpl *)(*it))->_dropConnection () ;
      }
      // release recycle bin
      copySet = _recycleBinSet ;
      for ( it = copySet.begin() ; it != copySet.end() ; ++ it )
      {
         ((_sdbRecycleBinImpl *)(*it))->_dropConnection () ;
      }
   }

   void _sdbImpl::_disconnect ()
   {
      INT32 rc = SDB_OK ;

      // when network error happen, _sock will be deleted and set to NULL
      if ( _sock )
      {
         CHAR buffer [ sizeof ( MsgOpDisconnect ) ] ;
         CHAR *pBuffer = &buffer[0] ;
         INT32 bufferSize = sizeof ( buffer ) ;

         // send disconnect msg to engine
         rc = clientBuildDisconnectMsg ( &pBuffer, &bufferSize, 0,
                                         _endianConvert ) ;
         if ( _sock->isConnected() && !rc )
         {
            clientSocketSend ( _sock, pBuffer, bufferSize ) ;
         }
         if ( pBuffer != &buffer[0] )
         {
            SDB_OSS_FREE ( pBuffer ) ;
         }
         _sock->close () ;
         delete _sock ;
         _sock = NULL ;
      }
      _clearSessionAttrCache( FALSE ) ;
      _setErrorBuffer( NULL, 0 ) ;
      _setResultBuffer( NULL, 0 ) ;
   }

   INT32 _sdbImpl::_connect ( const CHAR *pHostName,
                              UINT16 port )
   {
      INT32 rc = SDB_OK ;

      if ( _sock )
      {
         _disconnect () ;
      }
      _sock = new(std::nothrow) ossSocket ( pHostName, port ) ;
      if ( !_sock )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = _sock->initSocket () ;
      if ( rc )
      {
         goto error ;
      }
      rc = _sock->connect ( SDB_CLIENT_SOCKET_TIMEOUT_DFT ) ;
      if ( rc )
      {
         goto error ;
      }
      _sock->disableNagle () ;
      // set keep alive
      _sock->setKeepAlive( 1, OSS_SOCKET_KEEP_IDLE,
                           OSS_SOCKET_KEEP_INTERVAL,
                           OSS_SOCKET_KEEP_CONTER ) ;

      if ( _useSSL )
      {
#ifdef SDB_SSL
         rc = _sock->secure () ;
         if ( rc )
         {
            goto error ;
         }
         goto done;
#endif
         // do not support SSL
         rc = SDB_INVALIDARG ;
         goto error;
      }

   done :
      return rc ;
   error :
      if ( _sock )
         delete _sock ;
      _sock = NULL ;
      goto done ;
   }

   INT32 _sdbImpl::_regAndUnregHandle ( CLIENT_CLASS_TYPE type,
                                        ossValuePtr handle,
                                        BOOLEAN isRegister )
   {
      INT32 rc = SDB_OK ;
      std::set<ossValuePtr> *pSet = NULL ;
      // get the handle set
      switch( type )
      {
         case CLIENT_CLASS_CS :
            pSet = &_collectionspaces ;
            break;
         case CLIENT_CLASS_CL :
            pSet = &_collections ;
            break;
         case CLIENT_CLASS_CURSOR :
            pSet = &_cursors ;
            break;
         case CLIENT_CLASS_RG :
            pSet = &_replicaGroups ;
            break;
         case CLIENT_CLASS_NODE :
            pSet = &_nodes ;
            break;
         case CLIENT_CLASS_LOB :
            pSet = &_lobs ;
            break;
         case CLIENT_CLASS_DOMAIN :
            pSet = &_domains ;
            break;
         case CLIENT_CLASS_DC :
            pSet = &_dataCenters ;
            break;
         case CLIENT_CLASS_SQ :
            pSet = &_sequences ;
            break;
         case CLIENT_CLASS_DS :
            pSet = &_dataSources ;
            break;
         case CLIENT_CLASS_RB :
            pSet = &_recycleBinSet ;
            break;
         default:
            return SDB_INVALIDARG ;
      }
      // register or unregister
      _mutex.get() ;
      try
      {
         if ( isRegister )
         {
            pSet->insert ( handle ) ;
         }
         else
         {
            pSet->erase ( handle ) ;
         }
      }
      catch( std::exception &e )
      {
         rc = ossException2RC( &e ) ;
      }
      _mutex.release() ;
      return rc ;
   }

   INT32 _sdbImpl::_registerHandle ( CLIENT_CLASS_TYPE type, ossValuePtr handle )
   {
      return _regAndUnregHandle( type, handle, true ) ;
   }

   INT32 _sdbImpl::_unregisterHandle ( CLIENT_CLASS_TYPE type, ossValuePtr handle )
   {
      return _regAndUnregHandle( type, handle, false ) ;
   }

   hashTable* _sdbImpl::_getCachedContainer() const
   {
      return _tb ;
   }

   INT32 _sdbImpl::connect( const CHAR *pHostName,
                            UINT16 port )
   {
      return connect( pHostName, port, "", "" ) ;
   }

   INT32 _sdbImpl::_requestSysInfo ()
   {
      INT32 rc = SDB_OK ;
      MsgSysInfoRequest request ;
      MsgSysInfoRequest *prq = &request ;
      INT32 requestSize = sizeof(request) ;
      MsgSysInfoReply reply ;
      MsgSysInfoReply *pReply = &reply ;
      rc = clientBuildSysInfoRequest ( (CHAR**)&prq, &requestSize ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = clientSocketSend ( _sock, (CHAR *)prq, requestSize ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = clientSocketRecv ( _sock, (CHAR *)pReply, sizeof(MsgSysInfoReply) ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = clientExtractSysInfoReply ( (CHAR*)pReply, &_endianConvert, NULL,
                                       &_authVersion, &_peerProtocolVersion,
                                       &_dbStartTime, &_version, &_subVersion,
                                       &_fixVersion ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   /** \fn INT32 _authVer0MsgProcess( const CHAR *pUsrName,
                                      const CHAR *pPasswd )
       \brief Build msg, send msg and extract msg when we use
              MD5 authentication.
       \param [in] pUsrName User name.
       \param [in] pPasswd  User password.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _sdbImpl::_authVer0MsgProcess( const CHAR *pUsrName,
                                        const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      SINT64 contextID = -1 ;

      rc = clientBuildAuthVer0Msg( &_pSendBuffer, &_sendBufferSize,
                                   pUsrName, pPasswd, 0, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID ) ;
      if ( rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   /** \fn INT32 _step1( const CHAR *pUsrName,
                         const CHAR *pPasswd,
                         UINT32 &iterationCount,
                         string &saltBase64,
                         string &combineNonceBase64,
                         BOOLEAN &needAuth )
       \brief The authentication using SCRAM-SHA256 is divided into two
              certifications in total. This is the first certification.
              In the first certification, the client need to send username and
              client's nonce to the server.
       \param [in] pUsrName User name.
       \param [in] pPasswd  User password.
       \param [out] iterationCount Number of encryption iterations.
       \param [out] saltBase64 Random salt in base64 format.
       \param [out] combineNonceBase64 Combine string for clien nonce in base64
                    and server nonce in base64.
       \param [out] needAuth Whether we need to authenticate.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _sdbImpl::_step1( const CHAR *pUsrName, const CHAR *pPasswd,
                           UINT32 &iterationCount,
                           string &saltBase64,
                           string &combineNonceBase64,
                           BOOLEAN &needAuth )
   {
      INT32 rc = SDB_OK ;
      INT32 step = 0 ;
      BOOLEAN locked = FALSE ;
      BOOLEAN foundStep = FALSE ;
      BOOLEAN foundSalt = FALSE ;
      BOOLEAN foundIteration = FALSE ;
      BOOLEAN foundNonce = FALSE ;
      BSONObj result ;
      string clientNonceBase64 ;
      BYTE clientNonce[ UTIL_AUTH_SCRAMSHA_NONCE_LEN ] = { 0 } ;
      SINT64 contextID = -1 ;

      if ( NULL == pUsrName || NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = utilAuthGenerateNonce( clientNonce, UTIL_AUTH_SCRAMSHA_NONCE_LEN ) ;
      if ( rc )
      {
         goto error ;
      }
      clientNonceBase64 = base64::encode( (CHAR*)clientNonce,
                                          UTIL_AUTH_SCRAMSHA_NONCE_LEN ) ;

      rc = clientBuildAuthVer1Step1Msg( &_pSendBuffer, &_sendBufferSize,
                                        pUsrName, 0, _endianConvert,
                                        clientNonceBase64.c_str() ) ;
      if ( rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID ) ;
      if ( rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

      try
      {
         if ( NULL != _pResultBuf )
         {
            result = BSONObj( _pResultBuf ) ;
         }
      }
      catch( std::exception )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      // When "auth" is false in catalog node configure file sdb.conf, or
      // there is no user in SYSAUTH.SYSUSRS, the rc is SDB_OK and the result
      // is empty. So we don't need to authenticate.
      if ( SDB_OK == rc && result.isEmpty() )
      {
         needAuth = FALSE ;
         goto done ;
      }

      try
      {
         BSONObjIterator itr = BSONObjIterator( result ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next();
            if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_STEP ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG  ;
                  goto error ;
               }
               step = ele.numberInt() ;
               foundStep = TRUE ;
               if ( SDB_AUTH_STEP_1 != step )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_SALT ) )

            {
               if ( ele.type() != String )
               {
                  rc = SDB_INVALIDARG  ;
                  goto error ;
               }
               saltBase64 = ele.valuestr() ;
               foundSalt = TRUE ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_ITERATIONCOUNT ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG  ;
                  goto error ;
               }
               iterationCount = ele.numberInt() ;
               foundIteration = TRUE ;
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_NONCE ) )
            {
               if ( ele.type() != String )
               {
                  rc = SDB_INVALIDARG  ;
                  goto error ;
               }
               combineNonceBase64 = ele.valuestr() ;
               foundNonce = TRUE ;
            }
         }
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !foundStep || !foundSalt || !foundIteration || !foundNonce )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   /** \fn INT32 _step2( const CHAR *pUsrName,
                         const CHAR *pPasswd,
                         UINT32 iterationCount,
                         const string &saltBase64,
                         const string &combineNonceBase64,
                         const string &clientProofBase64 )
       \brief The authentication using SCRAM-SHA256 is divided into two
              certifications in total. This is the second certification.
              In the second certification, the client needs to send client'
              proof to the server and check whether the server's proof is legal.
       \param [in] pUsrName User name.
       \param [in] pPasswd  User password.
       \param [in] iterationCount Number of encryption iterations.
       \param [in] saltBase64 Random salt in base64 format.
       \param [in] combineNonceBase64 Combine string for clien nonce in base64
                   and server nonce in base64.
       \param [in] clientProofBase64 Client proof in base64 format.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _sdbImpl::_step2( const CHAR *pUsrName,
                           const CHAR *pPasswd,
                           UINT32 iterationCount,
                           const string &saltBase64,
                           const string &combineNonceBase64,
                           const string &clientProofBase64 )
   {
      INT32   rc      = SDB_OK ;
      BOOLEAN locked  = FALSE ;
      BOOLEAN isValid = FALSE ;
      BSONObj result ;
      BYTE salt[UTIL_AUTH_SCRAMSHA256_SALT_LEN] = { 0 } ;
      string storedKeyBase64 ;
      string serverKeyBase64 ;
      string clientKeyBase64 ;
      const CHAR *serverProofBase64 = NULL ;
      SINT64 contextID = -1 ;

      rc = clientBuildAuthVer1Step2Msg( &_pSendBuffer, &_sendBufferSize,
                                        pUsrName, 0, _endianConvert,
                                        combineNonceBase64.c_str(),
                                        clientProofBase64.c_str(),
                                        SDB_AUTH_CPP_IDENTIFY ) ;
      if ( rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID ) ;
      if ( rc )
      {
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

      try
      {
         if ( NULL != _pResultBuf )
         {
            result = BSONObj( _pResultBuf ) ;
         }

         string saltDecode = base64::decode( string(saltBase64) ) ;
         ossMemcpy( salt, saltDecode.c_str(), saltDecode.length() ) ;
      }
      catch( std::exception )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      try
      {
         BOOLEAN foundStep = FALSE ;
         BOOLEAN foundProof = FALSE ;
         INT32 step = 0 ;
         BSONObjIterator itr = BSONObjIterator( result ) ;
         while ( itr.more() )
         {
            BSONElement ele = itr.next();
            if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_STEP ) )
            {
               if ( !ele.isNumber() )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               step = ele.numberInt() ;
               foundStep = TRUE ;
               if ( SDB_AUTH_STEP_2 != step )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
            }
            else if ( 0 == ossStrcmp( ele.fieldName(), SDB_AUTH_PROOF ) )
            {
               if ( ele.type() != String )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               serverProofBase64 = ele.valuestr() ;
               foundProof = TRUE ;
            }
         }

         if ( !foundStep || !foundProof )
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }
      catch( std::exception )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = utilAuthCaculateKey( pPasswd, salt, sizeof(salt),
                                iterationCount, storedKeyBase64,
                                serverKeyBase64, clientKeyBase64 ) ;
      if( rc )
      {
         goto error ;
      }

      rc = utilAuthVerifyServerProof( serverProofBase64, pUsrName,
                                      iterationCount, saltBase64.c_str(),
                                      combineNonceBase64.c_str(),
                                      SDB_AUTH_CPP_IDENTIFY,
                                      serverKeyBase64.c_str(), TRUE,
                                      isValid ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( !isValid )
      {
         rc = SDB_AUTH_AUTHORITY_FORBIDDEN ;
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   /** \fn INT32 _authVer1MsgProcess( const CHAR *pUsrName,
                                      const CHAR *pPasswd )
       \brief Build msg, send msg and extract msg when we use
              SCRAM-SHA256 authentication.
       \param [in] pUsrName User name.
       \param [in] pPasswd  User password.
       \retval SDB_OK Operation Success
       \retval Others Operation Fail
   */
   INT32 _sdbImpl::_authVer1MsgProcess( const CHAR *pUsrName,
                                        const CHAR *pPasswd )
   {
      INT32  rc = SDB_OK ;
      string saltBase64 ;
      string combineNonceBase64 ;
      UINT32 iterationCount = 0 ;
      BOOLEAN needDoAuth = TRUE ;
      string clientProofBase64 ;

      rc = _step1( pUsrName, pPasswd,
                   iterationCount, saltBase64, combineNonceBase64,
                   needDoAuth ) ;
      if ( rc )
      {
         goto error ;
      }

      // If authentication is not required or there is no user in
      // SYSAUTH.SYSUSRS, we don't need to process _step2.
      if ( !needDoAuth )
      {
         goto done ;
      }

      rc = utilAuthCaculateClientProof( pPasswd, pUsrName,
                                        iterationCount, saltBase64.c_str(),
                                        combineNonceBase64.c_str(),
                                        SDB_AUTH_CPP_IDENTIFY, TRUE,
                                        clientProofBase64 ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _step2( pUsrName, pPasswd,
                   iterationCount, saltBase64, combineNonceBase64,
                   clientProofBase64 ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::connect ( const CHAR *pHostName,
                             UINT16 port,
                             const CHAR *pUsrName,
                             const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pUN = "" ;
      const CHAR *pPW = "" ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] = { 0 } ;

#if defined( SDB_FMP )
      BOOLEAN isRetry = FALSE ;
#endif // SDB_FMP

      if ( !pHostName || !*pHostName || port <= 0 || port > 65535 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( pUsrName && pPasswd )
      {
         pUN = pUsrName ;
         pPW = pPasswd ;
      }

      rc = _connect( pHostName, port ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = _requestSysInfo() ;
      if ( rc )
      {
         goto error ;
      }

      if ( SDB_PROTOCOL_VER_1 == _peerProtocolVersion && !_msgConvertor )
      {
         _msgConvertor = new(std::nothrow)sdbMsgConvertor() ;
         if ( !_msgConvertor )
         {
            rc = SDB_OOM ;
            goto error ;
         }
      }

#if !defined( SDB_FMP )
      rc = md5Encrypt( pPW, md5, SDB_MD5_DIGEST_LENGTH*2+1 ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( _authVersion >= AUTH_SCRAM_SHA256 )
      {
         rc = _authVer1MsgProcess( pUN, md5 ) ;
         // During rolling upgrade, the old version of catalog node
         // does not support the SAH256 algorithm.
         if ( SDB_UNKNOWN_MESSAGE == rc )
         {
            rc = _authVer0MsgProcess( pUN, md5 ) ;
         }
      }
      else
      {
         rc = _authVer0MsgProcess( pUN, md5 ) ;
      }
#else
   retry:
      if ( !isRetry && isMd5String( pPW ) )
      {
         rc = _authVer0MsgProcess( pUN, pPW ) ;
         if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
         {
            isRetry = TRUE ;
            goto retry ;
         }
      }
      else
      {
         rc = md5Encrypt( pPW, md5, SDB_MD5_DIGEST_LENGTH*2+1 ) ;
         if ( rc )
         {
            goto error ;
         }

         if ( _authVersion >= AUTH_SCRAM_SHA256 )
         {
            rc = _authVer1MsgProcess( pUN, md5 ) ;
            // During rolling upgrade, the old version of catalog node
            // does not recognize the SAH256
            if ( SDB_UNKNOWN_MESSAGE == rc )
            {
               rc = _authVer0MsgProcess( pUN, md5 ) ;
            }
         }
         else
         {
            rc = _authVer0MsgProcess( pUN, md5 ) ;
         }
      }

#endif // SDB_FMP

      if ( rc )
      {
         goto error ;
      }

      ossStrncpy ( _hostName, pHostName, OSS_MAX_HOSTNAME ) ;
      ossItoa( port, _serviceName, OSS_MAX_SERVICENAME ) ;
      ossStrcpy ( _address, _hostName ) ;
      ossStrncat ( _address, NODE_NAME_SERVICE_SEP, 1 ) ;
      ossStrncat ( _address, _serviceName, OSS_MAX_SERVICENAME ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::connect ( const CHAR **pConnAddrs,
                             INT32 arrSize,
                             const CHAR *pUsrName,
                             const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pHostName = NULL ;
      const CHAR *pServiceName = NULL ;
      const CHAR *addr = NULL ;
      CHAR *pStr = NULL ;
      CHAR *pTmp = NULL ;
      INT32 mark = 0 ;
      INT32 i = 0 ;
      INT32 tmp = 0 ;
      if ( !pConnAddrs || arrSize <= 0 || !pUsrName || !pPasswd )
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
         tmp = (++i) % arrSize ;
         i = tmp ;
         pStr = ossStrdup ( addr ) ;
         if ( pStr == NULL )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         pTmp = ossStrchr ( pStr, ':' ) ;
         if ( pTmp == NULL )
         {
            SDB_OSS_FREE ( pStr ) ;
            rc = SDB_NET_CANNOT_CONNECT ;
            continue ;
         }
         *pTmp = 0 ;
         pHostName = pStr ;
         pServiceName = pTmp + 1 ;
         rc = connect ( pHostName, pServiceName, pUsrName, pPasswd ) ;
         SDB_OSS_FREE ( pStr ) ;
         pStr = NULL ;
         pTmp = NULL ;
         if ( SDB_AUTH_AUTHORITY_FORBIDDEN == rc || SDB_OK == rc )
         {
            break;
         }
      } while ( mark != i ) ;
      if( rc )
      {
        goto error;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::connect ( const CHAR **pConnAddrs,
                             INT32 arrSize,
                             const CHAR *pUsrName,
                             const CHAR *pToken,
                             const CHAR *pCipherFile )
   {
      INT32 rc = SDB_OK ;
      INT32 tmp_rc = SDB_OK ;
      CHAR pPasswd[SDB_MAX_PASSWORD_LENGTH + 1] = { '\0' } ;
      CHAR pUser[SDB_MAX_USERNAME_LENGTH + 1] = { '\0' } ;
      const CHAR *pHostName = NULL ;
      const CHAR *pServiceName = NULL ;
      const CHAR *addr = NULL ;
      CHAR *pStr = NULL ;
      CHAR *pTmp = NULL ;
      INT32 mark = 0 ;
      INT32 i = 0 ;
      INT32 tmp = 0 ;
      if ( !pConnAddrs || arrSize <= 0 || !pUsrName || !pToken || !pCipherFile )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // calculate the start position
      i = _sdbRand() % arrSize ;
      mark = i ;
      // get password
      if ( '\0' != *pUsrName)
      {
         tmp_rc = utilDecryptUserCipher( pUsrName, pToken,
                                         pCipherFile, pUser, pPasswd ) ;
      }

      // get host and port
      do
      {
         addr = pConnAddrs[i] ;
         tmp = (++i) % arrSize ;
         i = tmp ;
         pStr = ossStrdup ( addr ) ;
         if ( pStr == NULL )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         pTmp = ossStrchr ( pStr, ':' ) ;
         if ( pTmp == NULL )
         {
            SDB_OSS_FREE ( pStr ) ;
            rc = SDB_NET_CANNOT_CONNECT ;
            continue ;
         }
         *pTmp = 0 ;
         pHostName = pStr ;
         pServiceName = pTmp + 1 ;
         rc = connect ( pHostName, pServiceName, pUser, pPasswd ) ;
         SDB_OSS_FREE ( pStr ) ;
         pStr = NULL ;
         pTmp = NULL ;
         if( SDB_AUTH_AUTHORITY_FORBIDDEN == rc )
         {
            if( SDB_OK != tmp_rc )
            {
               rc = tmp_rc ;
            }
            break;
         }
         if ( SDB_OK == rc )
         {
            break;
         }
      } while ( mark != i ) ;
      if( rc )
      {
         goto error;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::createUsr( const CHAR *pUsrName,
                              const CHAR *pPasswd,
                              const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] = { 0 } ;

      if ( NULL == pUsrName || NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( MAX_USERNAME_LENGTH < ossStrlen( pUsrName ) || MAX_PASSWORD_LENGTH < ossStrlen( pPasswd ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = md5Encrypt( pPasswd, md5, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;

      rc = clientBuildAuthCrtMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     pUsrName, pPasswd, md5, options.objdata(),
                                     0, _endianConvert, _authVersion ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::alterUsr( const CHAR *pUsrName,
                             const CHAR *pAction,
                             const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj alterObj ;

      if ( NULL == pUsrName || NULL == pAction )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      builder.append( SDB_AUTH_USER, pUsrName ) ;
      builder.append( FIELD_NAME_ACTION, pAction ) ;
      builder.append( FIELD_NAME_OPTIONS, options ) ;
      alterObj = builder.obj() ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_ALTER_USR,
                        &alterObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::changeUsrPasswd( const CHAR *pUsrName,
                                    const CHAR *pOldPasswd,
                                    const CHAR *pNewPasswd )
   {
      INT32 rc = SDB_OK ;
      CHAR md5new[SDB_MD5_DIGEST_LENGTH*2+1] = { 0 } ;
      CHAR md5old[SDB_MD5_DIGEST_LENGTH*2+1] = { 0 } ;
      BSONObj obj ;

      if ( !pUsrName || !pOldPasswd || !pNewPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = md5Encrypt( pOldPasswd, md5old, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      rc = md5Encrypt( pNewPasswd, md5new, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      obj = BSON( SDB_AUTH_PASSWD << md5new <<
                  SDB_AUTH_OLDPASSWD << md5old ) ;

      rc = alterUsr( pUsrName, CMD_VALUE_NAME_CHANGEPASSWD, obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::removeUsr( const CHAR *pUsrName,
                              const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] = { 0 } ;

      if ( NULL == pUsrName || NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = md5Encrypt( pPasswd, md5, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;

      rc = clientBuildAuthDelMsg( &_pSendBuffer, &_sendBufferSize,
                                  pUsrName, md5, 0,
                                  _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::getSnapshot ( _sdbCursor **result,
                                 INT32 snapType,
                                 const BSONObj &condition,
                                 const BSONObj &selector,
                                 const BSONObj &orderBy,
                                 const BSONObj &hint,
                                 INT64 numToSkip,
                                 INT64 numToReturn
                               )
   {
      INT32 rc                        = SDB_OK ;
      const CHAR *p                   = NULL ;

      if ( !result )
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
      case SDB_SNAP_TASKS :
         p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_TASKS ;
         break ;
      case SDB_SNAP_INDEXES :
         p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_INDEXES ;
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

      rc = _runCommand( p, &condition, &selector, &orderBy, &hint,
                        FLG_QUERY_WITH_RETURNDATA | FLG_QUERY_CLOSE_EOF_CTX, 0,
                        numToSkip, numToReturn, result ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::resetSnapshot ( const BSONObj &options )
   {

      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RESET,
                          &options ) ;
   }

   INT32 _sdbImpl::getList ( _sdbCursor **result,
                             INT32 listType,
                             const BSONObj &condition,
                             const BSONObj &selector,
                             const BSONObj &orderBy,
                             const bson::BSONObj &hint,
                             INT64 numToSkip,
                             INT64 numToReturn
                           )
   {
      INT32 rc                        = SDB_OK ;
      const CHAR *p                   = NULL ;

      if ( !result )
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
      case SDB_LIST_CL_IN_DOMAIN :
         p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CL_IN_DOMAIN ;
         break ;
      case SDB_LIST_CS_IN_DOMAIN :
         p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CS_IN_DOMAIN ;
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
      case SDB_LIST_GROUPMODES:
         p = CMD_ADMIN_PREFIX CMD_NAME_LIST_GROUPMODES ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _runCommand( p, &condition, &selector, &orderBy, &hint,
                        FLG_QUERY_WITH_RETURNDATA | FLG_QUERY_CLOSE_EOF_CTX, 0,
                        numToSkip, numToReturn, result ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::connect ( const CHAR *pHostName,
                             const CHAR *pServiceName
                           )
   {
      return connect( pHostName, pServiceName, "", "" ) ;
   }

   INT32 _sdbImpl::connect( const CHAR *pHostName,
                            const CHAR *pServiceName,
                            const CHAR *pUsrName,
                            const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      UINT16 port ;

      if ( !pHostName || !*pHostName || !pServiceName || !*pServiceName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = ossSocket::getPort ( pServiceName, port ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = connect( pHostName, port, pUsrName, pPasswd ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   void _sdbImpl::disconnect ()
   {
      if ( _sock )
      {
         _disconnect () ;
      }
   }

   INT32 _sdbImpl::_reallocBuffer ( CHAR **ppBuffer, INT32 *buffersize,
                                    INT32 newSize )
   {
      INT32 rc = SDB_OK ;
      CHAR *pOriginalBuffer = NULL ;
      if ( !ppBuffer || !buffersize )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      pOriginalBuffer = *ppBuffer ;
      if ( *buffersize < newSize )
      {
         *ppBuffer = (CHAR*)SDB_OSS_REALLOC ( *ppBuffer, sizeof(CHAR)*newSize);
         if ( !*ppBuffer )
         {
            *ppBuffer = pOriginalBuffer ;
            rc = SDB_OOM ;
            goto error ;
         }
         *buffersize = newSize ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   void _sdbImpl::_setErrorBuffer( const CHAR *pBuf, INT32 bufSize )
   {
      _pErrorBuf = pBuf ;
      _errorBufSize = bufSize ;
   }

   void _sdbImpl::_setResultBuffer( const CHAR *pBuf, INT32 bufSize )
   {
      _pResultBuf = pBuf ;
      _resultBufSize = bufSize ;
   }

   INT32 _sdbImpl::_send ( CHAR *pBuffer )
   {
      INT32 rc = SDB_OK ;
      INT32 len = 0 ;
      if ( !isConnected() )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( _msgConvertor )
      {
         UINT32 tmpLen = 0 ;
         _msgConvertor->reset() ;
         rc = _msgConvertor->push( pBuffer, *(SINT32 *)pBuffer ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = _msgConvertor->output( pBuffer, tmpLen ) ;
         if ( rc )
         {
            goto error ;
         }
         SDB_ASSERT( tmpLen == *(UINT32 *)pBuffer,
                     "Length of converted message is not as expected" ) ;
      }

      ossEndianConvertIf4 ( *(SINT32*)pBuffer, len, _endianConvert ) ;
      rc = clientSocketSend ( _sock, pBuffer, len ) ;
      if ( rc )
      {
         goto error ;
      }
      // get current time
      ossGetCurrentTime(_lastAliveTime);
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_recv ( CHAR **ppBuffer, INT32 *size )
   {
      INT32 rc = SDB_OK ;
      INT32 length = 0 ;
      INT32 realLen = 0 ;
      BOOLEAN isNeedDiscWithErr = FALSE ;
      INT32 minReplySize = ( SDB_PROTOCOL_VER_1 == _peerProtocolVersion ) ?
                           sizeof(MsgOpReplyV1) : sizeof(MsgOpReply) ;

      if ( !isConnected () )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      /*
         When has send succed, then recv failed my cause recv buff error.
         So, need disconnect socket
      */
      isNeedDiscWithErr = TRUE ;
      // first let's get message length
      rc = clientSocketRecv ( _sock,
                              (CHAR*)&length,
                              sizeof(length) ) ;
      if ( rc )
      {
         goto error ;
      }
      _sock->quickAck() ;

      ossEndianConvertIf4 ( length, realLen, _endianConvert ) ;
      if ( realLen < minReplySize )
      {
         rc = SDB_NET_BROKEN_MSG ;
         goto error ;
      }

      rc = _reallocBuffer ( ppBuffer, size, realLen+1 ) ;
      if ( rc )
      {
         goto error ;
      }

      *(SINT32*)(*ppBuffer) = length ;
      rc = clientSocketRecv ( _sock,
                              &(*ppBuffer)[sizeof(realLen)],
                              realLen - sizeof(realLen) ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( _msgConvertor )
      {
         UINT32 tmpLen = 0 ;
         CHAR *tmpPtr = NULL ;
         _msgConvertor->reset() ;
         rc = _msgConvertor->push( *ppBuffer, length ) ;
         if ( rc )
         {
            goto error ;
         }
         rc = _msgConvertor->output( tmpPtr, tmpLen ) ;
         if ( rc )
         {
            goto error ;
         }

         SDB_ASSERT( tmpLen == *(UINT32 *)tmpPtr,
                     "Converted message length is not as expected" ) ;

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

      // get current time
      ossGetCurrentTime(_lastAliveTime);
   done :
      return rc ;
   error :
      if ( SDB_NETWORK_CLOSE == rc ||
           SDB_NETWORK == rc ||
           isNeedDiscWithErr )
      {
         delete _sock ;
         _sock = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::_recvExtract ( CHAR **ppBuffer, INT32 *size,
                                  SINT64 &contextID,
                                  BOOLEAN *pRemoteErr,
                                  BOOLEAN *pHasRecv )
   {
      INT32 rc          = SDB_OK ;
      INT32 replyFlag   = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom   = -1 ;

      if ( pRemoteErr )
      {
         *pRemoteErr = FALSE ;
      }
      if ( pHasRecv )
      {
         *pHasRecv = FALSE ;
      }

      _pErrorBuf = NULL ;
      _errorBufSize = 0 ;
      _pResultBuf = NULL ;
      _resultBufSize = 0 ;

      rc = _recv ( ppBuffer, size ) ;
      if ( rc )
      {
         goto error ;
      }

      /// has recv done
      if ( pHasRecv )
      {
         *pHasRecv = TRUE ;
      }

      rc = clientExtractReply ( *ppBuffer, &replyFlag, &contextID,
                                &startFrom, &numReturned, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = replyFlag ;
      if ( pRemoteErr && SDB_OK != replyFlag )
      {
         *pRemoteErr = TRUE ;
      }

      if ( SDB_OK != replyFlag && SDB_DMS_EOC != replyFlag )
      {
         INT32 dataOff     = 0 ;
         INT32 dataSize    = 0 ;
         const CHAR *pErr  = NULL ;
         const CHAR *pDetail = NULL ;

         dataOff = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
         dataSize = ( ( MsgHeader* )( *ppBuffer ) )->messageLength - dataOff ;
         /// save error info
         if ( dataSize > 0 )
         {
            _pErrorBuf = ( *ppBuffer ) + dataOff ;
            _errorBufSize = dataSize ;
            _pResultBuf = _pErrorBuf ;
            _resultBufSize = _errorBufSize ;
            if ( _sdbErrorOnReplyCallback &&
                 SDB_OK == extractErrorObj( _pErrorBuf,
                                             NULL, &pErr, &pDetail ) )
            {
               (*_sdbErrorOnReplyCallback)( _pErrorBuf, (UINT32)_errorBufSize,
                                            replyFlag, pErr, pDetail ) ;
            }
         }
      }
      /*
         Temp solution. Insert result return the LastGenerateID
      */
      else if ( SDB_OK == replyFlag && 1 == numReturned &&
                ( MSG_BS_INSERT_RES == ((MsgHeader*)(*ppBuffer))->opCode ||
                  MSG_BS_UPDATE_RES == ((MsgHeader*)(*ppBuffer))->opCode ||
                  MSG_BS_DELETE_RES == ((MsgHeader*)(*ppBuffer))->opCode ||
                  MSG_BS_SQL_RES == ((MsgHeader*)(*ppBuffer))->opCode ||
                  MSG_AUTH_VERIFY1_RES == ((MsgHeader*)(*ppBuffer))->opCode ) )
      {
         INT32 dataOff     = 0 ;
         INT32 dataSize    = 0 ;
         MsgOpReply *pTmpReply = ( MsgOpReply* )( *ppBuffer ) ;

         dataOff = ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ;
         dataSize = pTmpReply->header.messageLength - dataOff ;
         /// save result info
         if ( dataSize > 0 )
         {
            _pResultBuf = ( *ppBuffer ) + dataOff ;
            _resultBufSize = dataSize ;
            /// should truncate the message size
            pTmpReply->header.messageLength = sizeof(MsgOpReply) ;
            pTmpReply->numReturned = 0 ;
         }
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_getRetVersion ()
   {
      INT32 version = CATALOG_INVALID_VERSION ;

      if(NULL != _pReceiveBuffer )
      {
         version = ((MsgOpReply*)_pReceiveBuffer)->startFrom;
      }
      return version;
   }

   INT32 _sdbImpl::_getRetInfo ( CHAR **ppBuffer, INT32 *size,
                                 SINT64 contextID,
                                 _sdbCursor **ppCursor )
   {
      INT32 rc           = SDB_OK ;
      _sdbCursor *cursor = NULL ;

      if ( !ppCursor )
      {
         goto done ;
      }
      // when ppCursor != NULL, we mush return a cursor
      rc = _buildEmptyCursor( &cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      // set contextID got from engine
      ((_sdbCursorImpl*)cursor)->_contextID = contextID ;
      // set the receive buffer into cursor
      if ( ((UINT32)((MsgHeader*)*ppBuffer)->messageLength) >
           ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
      {
         ((_sdbCursorImpl*)cursor)->_pReceiveBuffer = *ppBuffer ;
         *ppBuffer = NULL ;
         ((_sdbCursorImpl*)cursor)->_receiveBufferSize = *size ;
         *size = 0 ;
      }
      // return cursor
      *ppCursor = cursor ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_runCommand ( const CHAR *pString,
                                 const BSONObj *arg1,
                                 const BSONObj *arg2,
                                 const BSONObj *arg3,
                                 const BSONObj *arg4,
                                 SINT32 flag,
                                 UINT64 reqID,
                                 SINT64 numToSkip,
                                 SINT64 numToReturn,
                                 _sdbCursor **ppCursor )
   {
      INT32 rc            = SDB_OK ;

      // when need return cursor, add these flags for optimization
      if ( ppCursor )
      {
         flag |= FLG_QUERY_WITH_RETURNDATA ;
         flag |= FLG_QUERY_CLOSE_EOF_CTX ;
      }

      lock () ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, pString,
                                    flag, reqID, numToSkip, numToReturn,
                                    arg1 ? arg1->objdata() : NULL,
                                    arg2 ? arg2->objdata() : NULL,
                                    arg3 ? arg3->objdata() : NULL,
                                    arg4 ? arg4->objdata() : NULL,
                                    _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize,
                         ppCursor, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_sendAndRecv( const CHAR *pSendBuf,
                                 CHAR **ppRecvBuf,
                                 INT32 *recvBufSize,
                                 _sdbCursor **ppCursor,
                                 BOOLEAN needLock )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN hasRecv     = FALSE ;
      BOOLEAN remoteErr   = FALSE ;
      SINT64 contextID    = -1 ;

      if ( needLock )
      {
         lock () ;
      }

      rc = _send( (CHAR*)pSendBuf ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _recvExtract ( ppRecvBuf, recvBufSize, contextID,
                          &remoteErr, &hasRecv ) ;
      if ( rc )
      {
         goto error ;
      }

      // check return msg header
      CHECK_RET_MSGHEADER( pSendBuf, *ppRecvBuf, this ) ;

      // try to get retObj
      rc = _getRetInfo( ppRecvBuf, recvBufSize, contextID, ppCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      if ( needLock )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_buildEmptyCursor( _sdbCursor **ppCursor )
   {
      INT32 rc           = SDB_OK ;
      _sdbCursor *cursor = NULL ;

      // build cursor obj
      cursor = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl() ) ;
      if ( NULL == cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)cursor)->_contextID = -1 ;
      rc = ((_sdbCursorImpl*)cursor)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      *ppCursor = cursor ;

   done:
      return rc ;
   error:
      if ( NULL != cursor )
      {
         delete cursor ;
         cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::getCollection ( const CHAR *pCollectionFullName,
                                   _sdbCollection **collection,
                                   BOOLEAN checkExist )
   {
      INT32 rc            = SDB_OK ;
      INT32 version       = CATALOG_INVALID_VERSION ;
      BSONObjBuilder builder ;
      BSONObj query ;
      BSONObj selector ;

      if ( !pCollectionFullName || !*pCollectionFullName || !collection ||
           ossStrlen ( pCollectionFullName ) >
           CLIENT_CS_NAMESZ + CLIENT_COLLECTION_NAMESZ + 1 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( fetchCachedVersion( _tb, pCollectionFullName, &version ) )
      {
         // DO NOTHING
      }
      else
      {
         if ( checkExist )
         {
            query     = BSON ( FIELD_NAME_NAME << pCollectionFullName ) ;
            selector  = builder.appendNull( FIELD_NAME_VERSION ).obj() ;
            rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION,
                               &query, &selector ) ;
            if ( rc )
            {
               goto error ;
            }

            version = _getRetVersion();

            rc = insertCachedVersion( _tb, pCollectionFullName, version ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         else
         {
            version = CATALOG_DEFAULT_VERSION ;
         }
      }

      *collection = (_sdbCollection*)( new(std::nothrow) sdbCollectionImpl ()) ;
      if ( !*collection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionImpl*)*collection)->_setName ( pCollectionFullName ) ;
      ((sdbCollectionImpl*)*collection)->setVersion( version ) ;
      rc = ((sdbCollectionImpl*)*collection)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( collection && NULL != *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::getCollectionSpace ( const CHAR *pCollectionSpaceName,
                                        _sdbCollectionSpace **cs,
                                        BOOLEAN checkExist )
   {
      INT32 rc            = SDB_OK ;

      if ( !pCollectionSpaceName || !*pCollectionSpaceName || !cs ||
           ossStrlen ( pCollectionSpaceName ) > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( fetchCachedObject( _tb, pCollectionSpaceName ) )
      {
         // DO NOTHING
      }
      else
      {
         if ( checkExist )
         {
            BSONObj newObj ;
            newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName ) ;
            rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTIONSPACE,
                               &newObj ) ;
            if ( rc )
            {
               goto error ;
            }
            rc = insertCachedObject( _tb, pCollectionSpaceName ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
      }

      *cs = (_sdbCollectionSpace*)( new(std::nothrow) sdbCollectionSpaceImpl());
      if ( !*cs )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;
      rc = ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( cs && NULL != *cs )
      {
         delete *cs ;
         *cs = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                           INT32 iPageSize,
                                           _sdbCollectionSpace **cs )
   {
      INT32 rc            = SDB_OK ;
      BSONObj newObj ;

      if ( !pCollectionSpaceName || !*pCollectionSpaceName || !cs ||
           ossStrlen ( pCollectionSpaceName ) > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName <<
                      FIELD_NAME_PAGE_SIZE << iPageSize ) ;
      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      *cs = (_sdbCollectionSpace*)( new(std::nothrow) sdbCollectionSpaceImpl());
      if ( !*cs )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;
      rc = ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( _tb, pCollectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( cs && NULL != *cs )
      {
         delete *cs ;
         *cs = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::createCollectionSpace ( const CHAR *pCollectionSpaceName,
                                           const bson::BSONObj &options,
                                           _sdbCollectionSpace **cs )
   {
      INT32 rc            = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      if ( !pCollectionSpaceName || !*pCollectionSpaceName || !cs ||
           ossStrlen ( pCollectionSpaceName ) > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // build bson
      try
      {
         bob.append ( FIELD_NAME_NAME, pCollectionSpaceName ) ;
         bob.appendElementsUnique( options ) ;
         newObj = bob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      *cs = (_sdbCollectionSpace*)( new(std::nothrow) sdbCollectionSpaceImpl());
      if ( !*cs )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;
      rc = ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( _tb, pCollectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( cs && NULL != *cs )
      {
         delete *cs ;
         *cs = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::dropCollectionSpace ( const CHAR *pCollectionSpaceName,
                                         const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( !pCollectionSpaceName || !*pCollectionSpaceName ||
           ossStrlen ( pCollectionSpaceName ) > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder builder ;
         BSONObjIterator iterOptions( options ) ;
         while ( iterOptions.more() )
         {
            BSONElement ele = iterOptions.next() ;
            // skip name field
            if ( 0 != ossStrcmp( FIELD_NAME_NAME, ele.fieldName() ) )
            {
               builder.append( ele ) ;
            }
         }
         builder.append( FIELD_NAME_NAME, pCollectionSpaceName ) ;
         newObj = builder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      /// ignore the result
      removeCachedObject( _tb, pCollectionSpaceName, TRUE ) ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::listCollectionSpaces ( _sdbCursor **result )
   {
      return getList ( result, SDB_LIST_COLLECTIONSPACES ) ;
   }

   INT32 _sdbImpl::listCollections ( _sdbCursor **result )
   {
      return getList ( result, SDB_LIST_COLLECTIONS ) ;
   }

   INT32 _sdbImpl::listSequences ( _sdbCursor **result )
   {
      return getList ( result, SDB_LIST_SEQUENCES ) ;
   }

   INT32 _sdbImpl::listReplicaGroups ( _sdbCursor **result )
   {
      return getList ( result, SDB_LIST_GROUPS ) ;
   }

   INT32 _sdbImpl::getReplicaGroup ( const CHAR *pName,
                                     _sdbReplicaGroup **result )
   {
      INT32 rc = SDB_OK ;
      _sdbReplicaGroupImpl *replset = NULL ;
      sdbCursor resultCursor ;
      BOOLEAN found = FALSE ;
      BSONObj record ;
      BSONObj condition ;
      BSONElement ele ;

      if ( !pName || !*pName || !result ||
           ossStrlen ( pName ) > CLIENT_REPLICAGROUP_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // create search condition
      condition = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = getList( &resultCursor.pCursor, SDB_LIST_GROUPS, condition ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( SDB_OK == ( rc = resultCursor.next ( record ) ) )
      {
         replset = new(std::nothrow) _sdbReplicaGroupImpl () ;
         if ( !replset )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         rc = replset->_setConnection( this ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         ele = record.getField ( CAT_GROUPID_NAME ) ;
         if ( ele.type() == NumberInt )
         {
            replset->_replicaGroupID = ele.numberInt() ;
         }
         ossStrncpy ( replset->_replicaGroupName, pName,
                      CLIENT_REPLICAGROUP_NAMESZ ) ;
         if ( ossStrcmp ( pName, CAT_CATALOG_GROUPNAME ) == 0 )
         {
            replset->_isCatalog = TRUE ;
         }
         found = TRUE ;
         *result = replset ;
      }
      else if ( SDB_DMS_EOC != rc )
      {
         goto error ;
      }
      if ( !found )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( NULL != replset )
      {
         delete replset ;
         replset = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::getReplicaGroup ( SINT32 id, _sdbReplicaGroup **result )
   {
      INT32 rc = SDB_OK ;
      _sdbReplicaGroupImpl *replset = NULL ;
      sdbCursor resultCursor ;
      BOOLEAN found = FALSE ;
      BSONObj record ;
      BSONObj condition ;
      BSONElement ele ;

      if ( !result )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // create search condition
      condition = BSON ( CAT_GROUPID_NAME << id ) ;
      rc = getList ( &resultCursor.pCursor, SDB_LIST_GROUPS, condition ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( SDB_OK == ( rc = resultCursor.next ( record ) ) )
      {
         ele = record.getField ( CAT_GROUPNAME_NAME ) ;
         if ( ele.type() == String )
         {
            const CHAR *pReplicaGroupName = ele.valuestr() ;
            replset = new(std::nothrow) _sdbReplicaGroupImpl () ;
            if ( !replset )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            rc = replset->_setConnection( this ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
            ossStrncpy ( replset->_replicaGroupName, pReplicaGroupName,
                         CLIENT_REPLICAGROUP_NAMESZ ) ;
            replset->_replicaGroupID = id ;
            if ( ossStrcmp ( pReplicaGroupName, CAT_CATALOG_GROUPNAME ) == 0 )
            {
               replset->_isCatalog = TRUE ;
            }
            *result = replset ;
            found = TRUE ;
         } // if ( ele.type() == String )
      } // while ( SDB_OK == result.next ( record ) )
      else if ( SDB_DMS_EOC != rc )
      {
         goto error ;
      }
      if ( !found )
      {
         rc = SDB_CLS_GRP_NOT_EXIST ;
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( NULL != replset )
      {
         delete replset ;
         replset = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::createReplicaGroup ( const CHAR *pName,
                                        _sdbReplicaGroup **rg )
   {
      INT32 rc = SDB_OK ;
      BSONObj replicaGroupName ;
      _sdbReplicaGroupImpl *replset = NULL ;

      if ( !pName || !*pName || !rg ||
           ossStrlen ( pName ) > CLIENT_REPLICAGROUP_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_GROUP,
                         &replicaGroupName ) ;
      if ( rc )
      {
         goto error ;
      }
      replset = new(std::nothrow) _sdbReplicaGroupImpl () ;
      if ( !replset )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = replset->_setConnection( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      ossStrncpy ( replset->_replicaGroupName, pName,
                   CLIENT_REPLICAGROUP_NAMESZ ) ;
      if ( ossStrcmp ( pName, CAT_CATALOG_GROUPNAME ) == 0 )
      {
         replset->_isCatalog = TRUE ;
      }
      *rg = replset ;
   done :
      return rc ;
   error :
      if ( NULL != replset )
      {
         delete replset ;
         replset = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::removeReplicaGroup ( const CHAR *pReplicaGroupName )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder ob ;
      BSONObj newObj ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_REMOVE_GROUP ;

      if ( !pReplicaGroupName || !*pReplicaGroupName ||
           ossStrlen( pReplicaGroupName ) > CLIENT_REPLICAGROUP_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( FIELD_NAME_GROUPNAME, pReplicaGroupName ) ;
      newObj = ob.obj() ;
      rc = _runCommand ( pCommand, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::createReplicaCataGroup ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const CHAR *pDatabasePath,
                                            const BSONObj &configure )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObjBuilder ob ;
      const CHAR *pCreateCataRG = CMD_ADMIN_PREFIX CMD_NAME_CREATE_CATA_GROUP ;

      if ( !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // HostName is required
      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;

      // ServiceName is required
      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

      // database path is required
      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;

      // append all other parameters
      {
         BSONObjIterator it ( configure ) ;
         while ( it.more() )
         {
            BSONElement ele = it.next () ;
            const CHAR *key = ele.fieldName() ;
            if ( ossStrcmp ( key, PMD_OPTION_DBPATH ) == 0 ||
                 ossStrcmp ( key, PMD_OPTION_SVCNAME ) == 0  ||
                 ossStrcmp ( key, CAT_HOST_FIELD_NAME ) == 0 )
            {
               // skip the ones we already created
               continue ;
            }
            ob.append ( ele ) ;
         } // while
      } // if ( configure )
      configuration = ob.obj () ;

      rc = _runCommand ( pCreateCataRG, &configuration ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::activateReplicaGroup ( const CHAR *pName,
                                          _sdbReplicaGroup **rg )
   {
      INT32 rc = SDB_OK ;
      BSONObj replicaGroupName ;
      _sdbReplicaGroupImpl *replset = NULL ;

      if ( !pName || !*pName ||
           ossStrlen ( pName ) > CLIENT_REPLICAGROUP_NAMESZ || !rg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP,
                         &replicaGroupName ) ;
      if ( rc )
      {
         goto error ;
      }
      replset = new(std::nothrow) _sdbReplicaGroupImpl () ;
      if ( !replset )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = replset->_setConnection( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      ossStrncpy ( replset->_replicaGroupName, pName,
                   CLIENT_REPLICAGROUP_NAMESZ ) ;
      if ( ossStrcmp ( pName, CAT_CATALOG_GROUPNAME ) == 0 )
      {
         replset->_isCatalog = TRUE ;
      }
      *rg = replset ;
   done :
      return rc ;
   error :
      if ( NULL != replset )
      {
         delete replset ;
         replset = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::execUpdate( const CHAR *sql, bson::BSONObj *pResult )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

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

      lock () ;
      locked = TRUE ;

      rc = clientBuildSqlMsg( &_pSendBuffer, &_sendBufferSize,
                              sql, 0, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      /// get result
      if ( pResult )
      {
         getLastResultObj( *pResult, FALSE ) ;
      }
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::exec( const CHAR *sql, _sdbCursor **result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      if ( !sql || !result )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = clientValidateSql( sql, TRUE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;

      rc = clientBuildSqlMsg( &_pSendBuffer, &_sendBufferSize,
                              sql, 0,
                              _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, result, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      if ( result && *result )
      {
         delete *result ;
         *result = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::transactionBegin()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      lock () ;
      locked = TRUE ;

      rc = clientBuildTransactionBegMsg( &_pSendBuffer,
                                         &_sendBufferSize, 0,
                                         _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::transactionCommit( const bson::BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      lock () ;
      locked = TRUE ;

      rc = clientBuildTransactionCommitMsgCpp( &_pSendBuffer,
                                               &_sendBufferSize, 0,
                                               hint.objdata(),
                                               _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::transactionRollback()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      lock () ;
      locked = TRUE ;

      rc = clientBuildTransactionRollbackMsg( &_pSendBuffer,
                                              &_sendBufferSize, 0,
                                              _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::flushConfigure( const bson::BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_EXPORT_CONFIG,
                          &options ) ;
   }

   INT32 _sdbImpl::crtJSProcedure ( const CHAR *code )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      if ( !code )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ob.appendCode ( FIELD_NAME_FUNC, code ) ;
      ob.append ( FMP_FUNC_TYPE, FMP_FUNC_TYPE_JS ) ;
      newObj = ob.obj() ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_CRT_PROCEDURE,
                        &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::rmProcedure( const CHAR *spName )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      if ( !spName || !*spName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ob.append ( FIELD_NAME_FUNC, spName ) ;
      newObj = ob.obj() ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_RM_PROCEDURE,
                        &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::listProcedures( _sdbCursor **cursor,
                                   const bson::BSONObj &condition )
   {
      return getList ( cursor, SDB_LIST_STOREPROCEDURES, condition ) ;
   }

   INT32 _sdbImpl::evalJS( const CHAR *code,
                           SDB_SPD_RES_TYPE &type,
                           _sdbCursor **cursor,
                           bson::BSONObj &errmsg )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN remoteErr = FALSE ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      const MsgOpReply *replyHeader = NULL ;

      if ( !code || !*code || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ob.appendCode ( FIELD_NAME_FUNC, code ) ;
      ob.appendIntOrLL ( FIELD_NAME_FUNCTYPE, FMP_FUNC_TYPE_JS ) ;
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_EVAL,
                                   0, 0, 0, -1, newObj.objdata(),
                                   NULL, NULL, NULL,
                                   _endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, &remoteErr ) ;
      if ( rc )
      {
         if( remoteErr )
         {
            try
            {
               errmsg.init( _pReceiveBuffer + sizeof( MsgOpReply ) ) ;
            }
            catch( std::exception )
            {
               rc = SDB_SYS ;
            }
         }
         goto error ;
      }
      // check return msg header
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

      if ( *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      *cursor = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl () ) ;
      if ( !*cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;
      rc = ((_sdbCursorImpl*)*cursor)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      replyHeader = ( const MsgOpReply * )_pReceiveBuffer ;
      if ( 1 == replyHeader->numReturned &&
           (INT32)(sizeof( MsgOpReply )) < replyHeader->header.messageLength )
      {
         try
         {
            BSONObj runInfo( _pReceiveBuffer + sizeof( MsgOpReply ) ) ;
            BSONElement rType = runInfo.getField( FIELD_NAME_RTYPE ) ;
            if ( NumberInt != rType.type() )
            {
               rc = SDB_SYS ;
               goto error ;
            }
            type = ( SDB_SPD_RES_TYPE )( rType.Int() ) ;
         }
         catch ( std::exception )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
   done:
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error:
      if ( cursor && NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::backup ( const bson::BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_BACKUP_OFFLINE, &options ) ;
   }

   INT32 _sdbImpl::listBackup ( _sdbCursor **cursor,
                                const bson::BSONObj &options,
                                const bson::BSONObj &condition,
                                const bson::BSONObj &selector,
                                const bson::BSONObj &orderBy )
   {
      return getList( cursor, SDB_LIST_BACKUPS, condition,
                      selector, orderBy, options ) ;
   }

   INT32 _sdbImpl::removeBackup ( const bson::BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_REMOVE_BACKUP, &options ) ;
   }

   INT32 _sdbImpl::listTasks ( _sdbCursor **cursor,
                               const bson::BSONObj &condition,
                               const bson::BSONObj &selector,
                               const bson::BSONObj &orderBy,
                               const bson::BSONObj &hint )

   {
      return getList ( cursor, SDB_LIST_TASKS, condition, selector, orderBy ) ;
   }

   INT32 _sdbImpl::waitTasks ( const SINT64 *taskIDs, SINT32 num )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      // check argument
      if ( !taskIDs || num < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // append argument
      try
      {
         BSONObjBuilder bob ;
         BSONObjBuilder idBd( bob.subobjStart( FIELD_NAME_TASKID ) ) ;
         BSONArrayBuilder inBd( idBd.subarrayStart( "$in" ) ) ;

         // append subObj first
         for ( INT32 i = 0 ; i < num; i++ )
         {
            inBd.append( taskIDs[i] ) ;
         }
         inBd.done() ;
         idBd.done() ;

         newObj = bob.obj () ;
      }
      catch (std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_WAITTASK,
                        &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::cancelTask ( SINT64 taskID, BOOLEAN isAsync )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      // check argument
      if ( taskID <= 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // append argument
      try
      {
         BSONObjBuilder builder ;
         builder.appendIntOrLL ( FIELD_NAME_TASKID, taskID ) ;
         builder.appendBool( FIELD_NAME_ASYNC, isAsync ) ;
         newObj = builder.obj () ;
      }
      catch (std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK,
                        &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _sdbImpl::_getIsOldVersionLobServer()
   {
      return _isOldVersionLobServer ;
   }

   void _sdbImpl::_setIsOldVersionLobServer( BOOLEAN isOldVersionLobServer )
   {
      _isOldVersionLobServer = isOldVersionLobServer ;
   }

   void _sdbImpl::_clearSessionAttrCache ( BOOLEAN needLock )
   {
      if ( needLock )
      {
         lock() ;
      }
      _attributeCache = _sdbStaticObject ;
      if ( needLock )
      {
         unlock() ;
      }
   }

   void _sdbImpl::_setSessionAttrCache ( const BSONObj & attribute )
   {
      lock() ;
      _attributeCache = attribute.getOwned() ;
      unlock() ;
   }

   void _sdbImpl::_getSessionAttrCache ( BSONObj & attribute )
   {
      lock() ;
      attribute = _attributeCache ;
      unlock() ;
   }

   INT32 _sdbImpl::setSessionAttr ( const BSONObj &options )
   {
      INT32 rc         = SDB_OK ;
      BSONObjBuilder builder ;
      BSONObj newObj ;
      BSONObjIterator it ( options ) ;

      if ( !it.more() )
      {
         _clearSessionAttrCache( TRUE ) ;
         goto done ;
      }

      while ( it.more() )
      {
         BSONElement ele = it.next() ;
         const CHAR * key = ele.fieldName() ;
         if ( 0 == ossStrcasecmp( FIELD_NAME_PREFERRED_INSTANCE, key ) ||
              0 == ossStrcasecmp( FIELD_NAME_PREFERRED_INSTANCE_LEGACY, key ) )
         {
            switch ( ele.type() )
            {
               case String :
               {
                  try
                  {
                     INT32 value = PREFER_REPL_TYPE_MAX ;
                     const CHAR * str_value = ele.valuestrsafe() ;
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
                        //anyone
                        value = PREFER_REPL_ANYONE ;
                     }
                     else
                     {
                        rc = SDB_INVALIDARG ;
                        goto error ;
                     }

                     builder.append( key, value ) ;
                  }
                  catch( std::exception )
                  {
                     rc = SDB_DRIVER_BSON_ERROR ;
                     goto error ;
                  }
                  break ;
               }
               case NumberInt :
               {
                  builder.append( ele ) ;
                  break ;
               }
               default :
               {
                  break ;
               }
            }
            builder.appendAs( ele, FIELD_NAME_PREFERED_INSTANCE_V1 ) ;
         }
         else
         {
            builder.append( ele ) ;
         }
      }

      newObj = builder.obj() ;

      _clearSessionAttrCache( TRUE ) ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR,
                        &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::getSessionAttr ( BSONObj &attribute, BOOLEAN useCache )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN gotAttribute = FALSE ;
      _sdbCursor * cursor = NULL ;

      if ( !isConnected() )
      {
         goto error ;
      }

      if ( useCache )
      {
         _getSessionAttrCache( attribute ) ;
         if ( !attribute.isEmpty() )
         {
            gotAttribute = TRUE ;
            goto done ;
         }
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_GETSESS_ATTR,
                        NULL, NULL, NULL, NULL,
                        0, 0, 0, -1, &cursor ) ;
      if ( rc )
      {
         goto error ;
      }

      // Empty result
      if ( NULL == cursor )
      {
         _clearSessionAttrCache( TRUE ) ;
         rc = SDB_OK ;
      }
      else
      {
         rc = cursor->next( attribute ) ;
         if ( SDB_OK == rc )
         {
            _setSessionAttrCache( attribute ) ;
            gotAttribute = TRUE ;
         }
         else if ( SDB_DMS_EOC == rc )
         {
            _clearSessionAttrCache( TRUE ) ;
            rc = SDB_OK ;
         }
         else
         {
            goto error ;
         }
      }

      if ( !gotAttribute )
      {
         attribute = _sdbStaticObject ;
      }

   done :
      SAFE_OSS_DELETE( cursor ) ;
      return rc ;
   error :
      _clearSessionAttrCache( TRUE ) ;
      goto done ;
   }

   INT32 _sdbImpl::closeAllCursors()
   {
      return interrupt() ;
   }

   INT32 _sdbImpl::interrupt()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      std::set<ossValuePtr>::iterator it ;
      std::set<ossValuePtr> cursors ;
      std::set<ossValuePtr> lobs ;

      rc = clientBuildInterruptMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                    FALSE, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }

      // release resource of cursors in local,
      // remember to handle cursor._connection,
      // cursor._isClose
      cursors = _cursors ;
      for ( it = cursors.begin(); it != cursors.end(); ++it )
      {
         ((_sdbCursorImpl*)(*it))->_dropConnection () ;
         ((_sdbCursorImpl*)(*it))->_close () ;
      }
      _cursors.clear();
      // release resource of lob in local
      lobs = _lobs ;
      for ( it = lobs.begin(); it != lobs.end(); ++it )
      {
         ((_sdbLobImpl*)(*it))->_dropConnection () ;
         ((_sdbLobImpl*)(*it))->_close () ;
      }
      _lobs.clear() ;

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::interruptOperation()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      rc = clientBuildInterruptMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                    TRUE, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }


   INT32 _sdbImpl::isValid( BOOLEAN *result )
   {
      INT32 rc = SDB_OK ;
      // check argument
      if ( result == NULL )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *result = isValid() ;
   done :
      return rc ;
   error :
      goto done ;
   }

   BOOLEAN _sdbImpl::isValid()
   {
      BOOLEAN flag = FALSE ;
      // if client don't connect to database or
      // it had closed the connection
      if ( _sock == NULL )
      {
         flag = FALSE ;
      }
      else
      {
         flag =  _sock->isConnected() ;
      }
      return flag ;
   }

   BOOLEAN _sdbImpl::isClosed()
   {
      return _sock == NULL ? TRUE : FALSE ;
   }


   INT32 _sdbImpl::createDomain ( const CHAR *pDomainName,
                                  const bson::BSONObj &options,
                                  _sdbDomain **domain )
   {
      INT32 rc       = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      if ( !pDomainName || !*pDomainName || !domain ||
           ossStrlen ( pDomainName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // build bson
      try
      {
         ob.append ( FIELD_NAME_NAME, pDomainName ) ;
         ob.append ( FIELD_NAME_OPTIONS, options ) ;
         newObj = ob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_DOMAIN,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( *domain )
      {
         delete *domain ;
         *domain = NULL ;
      }
      *domain = (_sdbDomain*)( new(std::nothrow) sdbDomainImpl () ) ;
      if ( !(*domain) )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbDomainImpl*)*domain)->_setName ( pDomainName ) ;
      rc = ((sdbDomainImpl*)*domain)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( domain && NULL != *domain )
      {
         delete *domain ;
         *domain = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::dropDomain ( const CHAR *pDomainName )
   {
      INT32 rc       = SDB_OK ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      if ( !pDomainName || !*pDomainName ||
           ossStrlen ( pDomainName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // build bson
      try
      {
         ob.append ( FIELD_NAME_NAME, pDomainName ) ;
         newObj = ob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( CMD_ADMIN_PREFIX CMD_NAME_DROP_DOMAIN,
                         &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::getDomain ( const CHAR *pDomainName,
                               _sdbDomain **domain )
   {
      INT32 rc       = SDB_OK ;
      BSONObj result ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      sdbCursor cursor ;

      if ( !pDomainName || !*pDomainName ||
            ossStrlen ( pDomainName ) > CLIENT_COLLECTION_NAMESZ ||
            !domain )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      // build bson
      try
      {
         ob.append ( FIELD_NAME_NAME, pDomainName ) ;
         newObj = ob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      // test wether the demain is exsit or not
      rc = getList ( &cursor.pCursor, SDB_LIST_DOMAINS, newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( SDB_OK == ( rc = cursor.next( result ) ) )
      {
         // if domain exsit
         *domain = (_sdbDomain*)( new(std::nothrow) sdbDomainImpl() ) ;
         if ( !(*domain) )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ((sdbDomainImpl*)*domain)->_setName ( pDomainName ) ;
         rc = ((sdbDomainImpl*)*domain)->_setConnection ( this ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if ( SDB_DMS_EOC == rc )
      {
         // if domain not exsit
         rc = SDB_CAT_DOMAIN_NOT_EXIST ;
         goto done ;
      }
      else
      {
         // error happen
         goto error ;
      }

   done :
      return rc ;
   error :
      if ( domain && NULL != *domain )
      {
         delete *domain ;
         *domain = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::listDomains ( _sdbCursor **cursor,
                                 const bson::BSONObj &condition,
                                 const bson::BSONObj &selector,
                                 const bson::BSONObj &orderBy,
                                 const bson::BSONObj &hint )
   {
      return getList ( cursor, SDB_LIST_DOMAINS,
                       condition, selector, orderBy, hint ) ;
   }

   INT32 _sdbImpl::getDC( _sdbDataCenter **dc )
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pClusterName  = NULL ;
      const CHAR *pBusinessName = NULL ;
      _sdbDataCenter *pDC       = NULL ;
      BSONElement ele ;
      BSONObj retObj ;
      BSONObj subObj ;

      // check
      if ( NULL == _sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( NULL == dc )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build dc obj
      pDC = (_sdbDataCenter*)( new(std::nothrow) _sdbDataCenterImpl() ) ;
      if ( NULL == pDC )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      // register
      rc = ((sdbDataCenterImpl*)pDC)->_setConnection ( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // get dc name
      rc = pDC->getDetail( retObj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      ele = retObj.getField( FIELD_NAME_DATACENTER ) ;
      if ( Object == ele.type() )
      {
         subObj = ele.embeddedObject().getOwned() ;
         ele = subObj.getField( FIELD_NAME_CLUSTERNAME ) ;
         if ( String == ele.type() )
         {
            pClusterName = ele.valuestr() ;
         }
         ele = subObj.getField( FIELD_NAME_BUSINESSNAME ) ;
         if ( String == ele.type() )
         {
            pBusinessName = ele.valuestr() ;
         }
      }
      if ( NULL == pClusterName || NULL == pBusinessName )
      {
         rc = SDB_SYS ;
         goto done ;
      }
      rc = ((sdbDataCenterImpl*)pDC)->_setName ( pClusterName, pBusinessName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // return the newly build dc obj
      *dc = pDC ;

   done :
      return rc ;
   error :
      if ( NULL != pDC )
      {
         delete pDC ;
      }
      goto done ;
   }

   INT32 _sdbImpl::getRecycleBin( _sdbRecycleBin **recycleBin )
   {
      INT32 rc                    = SDB_OK ;
      _sdbRecycleBin *pRecycleBin = NULL ;

      // check
      if ( NULL == _sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( NULL == recycleBin )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      // build recycle bin object
      pRecycleBin =
            (_sdbRecycleBin *)( new( std::nothrow ) _sdbRecycleBinImpl() ) ;
      if ( NULL == pRecycleBin )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      // register
      rc = ( (_sdbRecycleBinImpl *)pRecycleBin )->_setConnection( this ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      // return the newly build recycle bin object
      *recycleBin = pRecycleBin ;

   done:
      return rc ;

   error:
      if ( NULL != pRecycleBin )
      {
         delete pRecycleBin ;
      }
      goto done ;
   }

   INT32 _sdbImpl::syncDB( const bson::BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_SYNC_DB, &options ) ;
   }

   INT32 _sdbImpl::analyze ( const bson::BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_ANALYZE, &options ) ;
   }

   INT32 _sdbImpl::forceSession( SINT64 sessionID, const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_SESSIONID, sessionID ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_FORCE_SESSION), &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::forceStepUp( const BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_FORCE_STEP_UP,
                          &options ) ;
   }

   INT32 _sdbImpl::invalidateCache( const BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE,
                          &options ) ;
   }

   INT32 _sdbImpl::reloadConfig( const BSONObj &options )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_RELOAD_CONFIG,
                          &options ) ;
   }

   INT32 _sdbImpl::updateConfig( const BSONObj &configs,
                                 const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.appendElements( options ) ;
         queryBuilder.append( FIELD_NAME_CONFIGS, configs ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_UPDATE_CONFIG, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::deleteConfig( const BSONObj &configs,
                                 const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.appendElements( options ) ;
         queryBuilder.append( FIELD_NAME_CONFIGS, configs ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_DELETE_CONFIG, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::setPDLevel( INT32 level, const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_PDLEVEL, level ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_SET_PDLEVEL, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::memTrim( const CHAR *maskStr,
                            const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         BSONObjIterator itr( options ) ;

         if ( maskStr )
         {
            queryBuilder.append( FIELD_NAME_MASK, maskStr ) ;
         }

         while( itr.more() )
         {
            BSONElement e = itr.next() ;
            if ( 0 == ossStrcmp( e.fieldName(), FIELD_NAME_MASK ) )
            {
               continue ;
            }
            queryBuilder.append( e ) ;
         }
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_MEM_TRIM, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::msg( const CHAR* msg )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;

      if( NULL == msg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      lock () ;
      locked = TRUE ;

      rc = clientBuildTestMsg( &_pSendBuffer, &_sendBufferSize, msg, 0,
                               _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = _sendAndRecv( _pSendBuffer, &_pReceiveBuffer,
                         &_receiveBufferSize, NULL, FALSE ) ;
      if ( rc )
      {
         goto error ;
      }

   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::loadCS( const CHAR* csName, const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      if( NULL == csName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_NAME, csName ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_LOAD_COLLECTIONSPACE,
                        &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      rc = insertCachedObject( _tb, csName ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::unloadCS( const CHAR* csName, const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      if( NULL == csName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_NAME, csName ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_UNLOAD_COLLECTIONSPACE,
                        &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      /// ignore the result
      removeCachedObject( _tb, csName, FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::_traceStrtok( BSONArrayBuilder &arrayBuilder,
                                 const CHAR* pLine )
   {
      INT32 rc     = SDB_OK ;
      INT32 len    = 0 ;
      CHAR *pStart = NULL ;
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
         try
         {
            arrayBuilder.append( pStart ) ;
         }
         catch( std::exception )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
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

   INT32 _sdbImpl::traceStart( UINT32 traceBufferSize,
                               const CHAR* component,
                               const CHAR* breakpoint,
                               const vector<UINT32> &tidVec )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_SIZE, traceBufferSize ) ;
         // append array element, field name: Components
         {
            BSONArrayBuilder componentBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_COMPONENTS ) ) ;

            rc = _traceStrtok( componentBuilder, component ) ;
            if( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }

            componentBuilder.doneFast() ;
         }
         // append array element, field name: Breakpoint
         {
            BSONArrayBuilder breakpointBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_BREAKPOINTS ) ) ;

            rc = _traceStrtok( breakpointBuilder, breakpoint ) ;
            if( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
            breakpointBuilder.doneFast() ;
         }

         // append array element, field name: Threads
         {
            BSONArrayBuilder threadsBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_THREADS ) ) ;
            for( vector< UINT32 >::const_iterator itr = tidVec.begin();
                 itr != tidVec.end(); itr++ )
            {
               threadsBuilder.append( *itr ) ;
            }
            threadsBuilder.doneFast() ;
         }
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRACE_START, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::traceStart( UINT32 traceBufferSize,
                               const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_SIZE, traceBufferSize ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRACE_START, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;

   }

   INT32 _sdbImpl::traceStop( const CHAR* dumpFileName )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      if( dumpFileName )
      {
         try
         {
            query = BSON( FIELD_NAME_FILENAME << dumpFileName ) ;
         }
         catch( std::exception )
         {
            rc = SDB_DRIVER_BSON_ERROR ;
            goto error ;
         }
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRACE_STOP, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::traceResume()
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRACE_RESUME ) ;
   }

   INT32 _sdbImpl::traceStatus( _sdbCursor** cursor )
   {
      return _runCommand( CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS,
                          NULL, NULL, NULL, NULL, 0, 0, 0, -1,
                          cursor ) ;
   }

   INT32 _sdbImpl::renameCollectionSpace( const CHAR* oldName,
                                          const CHAR* newName,
                                          const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      if( NULL == oldName || NULL == newName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_OLDNAME, oldName ) ;
         queryBuilder.append( FIELD_NAME_NEWNAME, newName ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTIONSPACE,
                        &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      /// ingore the result
      removeCachedObject( _tb, oldName, FALSE ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::getLastErrorObj( bson::BSONObj &result )
   {
      INT32 rc = SDB_OK ;
      BSONObj localObj ;

      if ( _pErrorBuf && _errorBufSize >= 5 &&
           *(INT32*)_pErrorBuf >= 5 )
      {
         try
         {
            localObj.init( _pErrorBuf ) ;
         }
         catch( std::exception )
         {
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         try
         {
            result = localObj.copy() ;
         }
         catch( std::exception )
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
      return rc ;
   error :
      goto done ;
   }

   void _sdbImpl::cleanLastErrorObj()
   {
      _setErrorBuffer( NULL, 0 ) ;
   }

   INT32 _sdbImpl::getLastResultObj( bson::BSONObj &result,
                                     BOOLEAN getOwned ) const
   {
      INT32 rc = SDB_OK ;
      BSONObj localObj ;

      result = localObj ;

      if ( _pResultBuf && _resultBufSize >= 5 &&
           *(INT32*)_pResultBuf >= 5 )
      {
         try
         {
            localObj.init( _pResultBuf ) ;
         }
         catch( std::exception )
         {
            rc = SDB_CORRUPTED_RECORD ;
            goto error ;
         }
         try
         {
            if ( getOwned )
            {
               result = localObj.copy() ;
            }
            else
            {
               result = localObj ;
            }
         }
         catch( std::exception )
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
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::createDataSource( _sdbDataSource **dataSource,
                                     const CHAR *pDataSourceName,
                                     const CHAR *addresses,
                                     const CHAR *user,
                                     const CHAR *password,
                                     const CHAR *type,
                                     const bson::BSONObj *options )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( !pDataSourceName || !*pDataSourceName || !addresses ||
           ( user && !password ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         BSONObjBuilder bob ;
         bob.append( FIELD_NAME_NAME, pDataSourceName ) ;
         bob.append( FIELD_NAME_ADDRESS, addresses ) ;
         if ( user )
         {
            CHAR md5[ SDB_MD5_VALUE_BUF_LEN ] = { 0 } ;
            bob.append( FIELD_NAME_USER, user ) ;
            rc = md5Encrypt( password, md5, SDB_MD5_VALUE_BUF_LEN ) ;
            if ( rc )
            {
               goto error ;
            }
            bob.append( FIELD_NAME_PASSWD, md5 ) ;
         }
         if ( type )
         {
            bob.append( FIELD_NAME_TYPE, type ) ;
         }
         if ( options )
         {
            bob.appendElementsUnique( *options ) ;
         }
         newObj = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_CREATE_DATASOURCE, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( dataSource && *dataSource )
      {
         delete *dataSource ;
         *dataSource = NULL ;
      }

      if ( dataSource )
      {
         *dataSource = ( _sdbDataSource *)( new(std::nothrow) sdbDataSourceImpl() ) ;
         if ( !*dataSource )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ((sdbDataSourceImpl*)*dataSource)->_setName( pDataSourceName ) ;
         rc = ((sdbDataSourceImpl*)*dataSource)->_setConnection( this ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      if ( dataSource && *dataSource )
      {
         delete *dataSource ;
         *dataSource = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::dropDataSource(const CHAR *pDataSourceName)
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObjBuilder builder ;

      if ( !pDataSourceName ||
           ossStrlen( pDataSourceName ) > CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_NAME, pDataSourceName ) ;
         obj = builder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_DROP_DATASOURCE, &obj ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::getDataSource( const CHAR *pDataSourceName,
                                  _sdbDataSource **dataSource )
   {
      INT32 rc = SDB_OK ;
      BSONObj condition ;
      BSONObjBuilder ob ;
      BSONObj result ;
      sdbCursor cursor ;

      if ( !pDataSourceName || !*pDataSourceName ||
           ossStrlen( pDataSourceName) > CLIENT_DATASOURCE_NAMESZ ||
           !dataSource )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         ob.append( FIELD_NAME_NAME, pDataSourceName ) ;
         condition = ob.done() ;
      }
      catch( const std::exception& )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = getList( &cursor.pCursor, SDB_LIST_DATASOURCES, condition ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = cursor.next( result ) ;
      if ( SDB_OK == rc )
      {
         *dataSource = (_sdbDataSource*)( new(std::nothrow) sdbDataSourceImpl() ) ;
         if ( !(*dataSource) )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ((sdbDataSourceImpl*)*dataSource)->_setName( pDataSourceName ) ;
         rc = ((sdbDataSourceImpl*)*dataSource)->_setConnection( this ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CAT_DATASOURCE_NOTEXIST ;
         goto error ;
      }
      else
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      if ( dataSource && NULL != *dataSource )
      {
         delete *dataSource ;
         *dataSource = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::listDataSources( _sdbCursor **cursor,
                                    const BSONObj &condition,
                                    const BSONObj &selector,
                                    const BSONObj &orderBy,
                                    const BSONObj &hint )
   {
      return getList( cursor, SDB_LIST_DATASOURCES,
                      condition, selector, orderBy, hint ) ;
   }

   INT32 _sdbImpl::createRole( const bson::BSONObj &role )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_CREATE_ROLE ;

      if ( !role.hasField( FIELD_NAME_ROLE ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &role ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 _sdbImpl::dropRole( const CHAR *roleName )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_DROP_ROLE ;

      if ( !roleName || !*roleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      {
         BSONObj query = BSON( FIELD_NAME_ROLE << roleName );
         rc = _runCommand( pCommand, &query );
         if ( rc )
         {
            goto error;
         }
      }

      done:
   return rc ;
error:
   goto done ;

   }

   INT32 _sdbImpl::getRole( const CHAR *pRoleName,
                            const bson::BSONObj &options,
                            bson::BSONObj &role )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      BSONObjBuilder builder ;
      _sdbCursor * cursor = NULL ;

      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendElements( options ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_ROLE, &query, NULL, NULL, NULL, 0, 0, 0, -1,
                        &cursor );
      if ( rc )
      {
         goto error ;
      }

      rc = cursor->next( role ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
      }
      else
      {
         goto error ;
      }

   done:
      SAFE_OSS_DELETE( cursor ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::listRoles( _sdbCursor **result, const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_LIST_ROLES, &options, NULL, NULL, NULL, 0, 0, 0,
                        -1, result );
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::updateRole( const CHAR *pRoleName, const bson::BSONObj &role )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_UPDATE_ROLE ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendElements( role ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::grantPrivilegesToRole( const CHAR *pRoleName, const bson::BSONObj &privileges )
   {   
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_GRANT_PRIVILEGES ;
      BSONObjBuilder builder;
      BSONObj query;


      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendArray( FIELD_NAME_PRIVILEGES, privileges ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::revokePrivilegesFromRole( const CHAR *pRoleName,
                                             const bson::BSONObj &privileges )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_REVOKE_PRIVILEGES ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendArray( FIELD_NAME_PRIVILEGES, privileges ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::grantRolesToRole( const CHAR *pRoleName, const bson::BSONObj &roles )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_GRANT_ROLES_TO_ROLE ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendArray( FIELD_NAME_ROLES, roles ) ;
         query = builder.done();
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::revokeRolesFromRole( const CHAR *pRoleName, const bson::BSONObj &roles )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_REVOKE_ROLES_FROM_ROLE ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pRoleName || !*pRoleName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_ROLE, pRoleName ) ;
         builder.appendArray( FIELD_NAME_ROLES, roles ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::grantRolesToUser( const CHAR *pUsrName, const bson::BSONObj &roles )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_GRANT_ROLES_TO_USER ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pUsrName || !*pUsrName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_USER, pUsrName ) ;
         builder.appendArray( FIELD_NAME_ROLES, roles ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::revokeRolesFromUser( const CHAR *pUsrName, const bson::BSONObj &roles )
   {
      INT32 rc = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_REVOKE_ROLES_FROM_USER ;
      BSONObjBuilder builder;
      BSONObj query;

      if ( !pUsrName || !*pUsrName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_USER, pUsrName ) ;
         builder.appendArray( FIELD_NAME_ROLES, roles ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand ( pCommand, &query ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::getUser( const CHAR *pUserName,
                            const bson::BSONObj &options,
                            bson::BSONObj &user )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      BSONObjBuilder builder ;
      _sdbCursor * cursor = NULL ;

      if ( !pUserName || !*pUserName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         builder.append( FIELD_NAME_USER, pUserName ) ;
         builder.appendElements( options ) ;
         query = builder.done() ;
      }
      catch ( const std::exception & )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_GET_USER, &query, NULL, NULL, NULL, 0, 0, 0, -1,
                        &cursor );
      if ( rc )
      {
         goto error ;
      }

      rc = cursor->next( user ) ;
      if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_AUTH_ROLE_NOT_EXIST;
      }
      else
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::invalidateUserCache( const CHAR *pUserName, const BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;
      
      try
      {
         BSONObjBuilder queryBuilder ;
         if ( pUserName )
         {
            queryBuilder.append( FIELD_NAME_USER, pUserName );
         }
         if ( !options.isEmpty() )
         {
            queryBuilder.appendElements( options );
         }
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_USER_CACHE, &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   _sdb *_sdb::getObj ( BOOLEAN useSSL )
   {
      return (_sdb*)(new(std::nothrow) sdbImpl ( useSSL )) ;
   }

   INT32 initClient( sdbClientConf* config )
   {
      if ( NULL == config )
      {
         return SDB_OK ;
      }

      return initCacheStrategy( config->enableCacheStrategy,
                                config->cacheTimeInterval ) ;
   }

   INT32 _sdbImpl::createSequence( const CHAR *pSequenceName,
                                   const bson::BSONObj &options,
                                   _sdbSequence **sequence )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder ob ;
      BSONObj newObj ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_CREATE_SEQUENCE ;

      if ( !pSequenceName || !*pSequenceName || !sequence ||
           options.hasField( FIELD_NAME_NAME ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( FIELD_NAME_NAME, pSequenceName ) ;
      ob.appendElements( options ) ;
      newObj = ob.obj() ;
      rc = _runCommand ( pCommand, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      *sequence = (_sdbSequence*)( new(std::nothrow) sdbSequenceImpl () ) ;
      if ( !*sequence )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      rc = ((sdbSequenceImpl*)*sequence)->_setConnection ( this ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = ((sdbSequenceImpl*)*sequence)->_setName ( pSequenceName ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error :
      if ( sequence && NULL != *sequence )
      {
         delete *sequence ;
         *sequence = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::getSequence( const CHAR *pSequenceName,
                                _sdbSequence **sequence )
   {
      INT32 rc = SDB_OK ;
      sdbCursor resultCursor ;
      BSONObj condition ;
      BSONObj result ;

      if ( !pSequenceName || !*pSequenceName || !sequence )
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

      try
      {
         // TODO: cache it
         condition = BSON ( FIELD_NAME_NAME << pSequenceName ) ;
         rc = getList( &resultCursor.pCursor, SDB_LIST_SEQUENCES, condition ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = resultCursor.next ( result ) ;
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_SEQUENCE_NOT_EXIST ;
         }
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      *sequence = (_sdbSequence*)( new(std::nothrow) sdbSequenceImpl () ) ;
      if ( !*sequence )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      rc = ((sdbSequenceImpl*)*sequence)->_setConnection ( this ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = ((sdbSequenceImpl*)*sequence)->_setName ( pSequenceName ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error :
      if ( sequence && NULL != *sequence )
      {
         delete *sequence ;
         *sequence = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::renameSequence( const CHAR *pOldName, const CHAR *pNewName )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj obj ;

      if ( !pOldName || !*pOldName || !pNewName || !*pNewName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         bob.append( FIELD_NAME_ACTION, CMD_VALUE_NAME_RENAME ) ;
         BSONObjBuilder subBob( bob.subobjStart( FIELD_NAME_OPTIONS ) ) ;
         subBob.append( FIELD_NAME_NAME, pOldName ) ;
         subBob.append( FIELD_NAME_NEWNAME, pNewName ) ;
         subBob.done() ;
         obj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_ALTER_SEQUENCE, &obj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::dropSequence( const CHAR *pSequenceName )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder ob ;
      BSONObj newObj ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_DROP_SEQUENCE ;

      if ( !pSequenceName || !*pSequenceName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( FIELD_NAME_NAME, pSequenceName ) ;
      newObj = ob.obj() ;
      rc = _runCommand ( pCommand, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error :
      goto done ;
   }

   _sdbMsgConvertor::_sdbMsgConvertor()
   : _hasData( FALSE  ),
     _buff( NULL ),
     _buffSize( 0 )
   {
   }

   _sdbMsgConvertor::~_sdbMsgConvertor()
   {
      if ( _buff )
      {
         SDB_OSS_FREE( _buff ) ;
      }
   }

   void _sdbMsgConvertor::reset( BOOLEAN releaseBuff )
   {
      _hasData = FALSE ;
   }

   INT32 _sdbMsgConvertor::push( const CHAR *data, UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( MSG_COMM_EYE_DEFAULT == ((MsgHeader *)data)->eye )
      {
         rc = _downgradeRequest( (MsgHeader *)data ) ;
      }
      else
      {
         rc = _upgradeReply( (MsgOpReplyV1 *)data ) ;
      }
      if ( rc )
      {
         goto error ;
      }

      _hasData = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbMsgConvertor::output( CHAR *&data, UINT32 &len )
   {
      data = NULL ;
      len = 0 ;

      if ( _hasData )
      {
         data = _buff ;
         len = *(INT32 *)_buff ;
      }

      return SDB_OK ;
   }

   INT32 _sdbMsgConvertor::_downgradeRequest( MsgHeader *header )
   {
      INT32 rc = SDB_OK ;
      MsgHeaderV1 newHeader ;

      clientMsgHeaderDowngrade( header, &newHeader ) ;
      rc = _ensureBuff( newHeader.messageLength ) ;
      if ( rc )
      {
         goto error ;
      }

      ossMemcpy( _buff, &newHeader, sizeof(MsgHeaderV1) ) ;
      ossMemcpy( _buff + sizeof(MsgHeaderV1), (CHAR *)header + sizeof(MsgHeader),
                 header->messageLength - sizeof(MsgHeader) ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbMsgConvertor::_upgradeReply( MsgOpReplyV1 *reply )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply newReply ;

      clientMsgReplyHeaderUpgrade( reply, &newReply ) ;
      rc = _ensureBuff( newReply.header.messageLength ) ;
      if ( rc )
      {
         goto error ;
      }

      ossMemcpy( _buff, &newReply, sizeof(MsgOpReply) ) ;
      ossMemcpy( _buff + sizeof(MsgOpReply),
                (CHAR *)reply + sizeof(MsgOpReplyV1),
                reply->header.messageLength - sizeof(MsgOpReplyV1) ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbMsgConvertor::_ensureBuff( UINT32 size )
   {
      INT32 rc = SDB_OK ;

      if ( size > _buffSize )
      {
         CHAR *newBuff =
            (CHAR *)SDB_OSS_REALLOC( _buff, size ) ;
         if ( !newBuff )
         {
            rc = SDB_OOM ;
            goto error ;
         }

         _buff = newBuff ;
         _buffSize = size ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }
}
