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
#include <string>
#include <vector>
#ifdef SDB_SSL
#include "ossSSLWrapper.h"
#endif

using namespace std ;
using namespace bson ;

#define LOB_ALIGNED_LEN 524288

namespace sdbclient
{
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

   /*
    * sdbCursorImpl
    * Cursor Implementation
    */
   _sdbCursorImpl::_sdbCursorImpl () :
   _connection ( NULL ),
   _collection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
   _modifiedCurrent ( NULL ),
   _isDeleteCurrent ( FALSE ),
   _contextID ( -1 ),
   _isClosed ( FALSE ),
   _totalRead ( 0 ),
   _offset ( -1 )
   {
      _hintObj = BSON ( "" << CLIENT_RECORD_ID_INDEX ) ;
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
         _detachConnection() ;
      }
      if ( _collection )
      {
         _detachCollection();
      }
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
      if ( _modifiedCurrent )
      {
         delete _modifiedCurrent ;
         _modifiedCurrent = NULL ;
      }
   }

   void _sdbCursorImpl::_attachConnection ( _sdbImpl *connection )
   {
      if ( NULL != connection )
      {
         _connection = connection ;
         _connection->_regCursor ( this ) ;
      }
   }

   void _sdbCursorImpl::_attachCollection ( _sdbCollectionImpl *collection )
   {
      if ( NULL != collection )
      {
         _collection = collection ;
         _collection->_regCursor ( this ) ;
      }
   }

   void _sdbCursorImpl::_detachConnection()
   {
      if ( NULL != _connection )
      {
         _connection->_unregCursor( this ) ;
         _connection = NULL ;
      }
   }

   void _sdbCursorImpl::_detachCollection()
   {
      if ( NULL != _collection )
      {
         _collection->_unregCursor( this ) ;
         _collection = NULL ;
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
      SINT64 contextID = 0 ;
      BOOLEAN result   = FALSE ;
      BOOLEAN locked   = FALSE ;

      if ( -1 == _contextID || !_connection )
      {
         goto done ;
      }
      rc = clientBuildKillContextsMsg ( &_pSendBuffer, &_sendBufferSize, 0,
                                        1, &_contextID,
                                        _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
   done :
      if ( locked )
      {
         _connection->unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::_readNextBuffer ()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked  = FALSE ;
      SINT64 contextID = 0 ;
      if ( -1 == _contextID )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      rc = clientBuildGetMoreMsg ( &_pSendBuffer, &_sendBufferSize, -1,
                                   _contextID, 0, _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc || contextID != _contextID )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
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
      _close() ;
      _detachConnection() ;
      _detachCollection() ;
      goto done ;
   }

   INT32 _sdbCursorImpl::next ( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      BSONObj localobj ;
      MsgOpReply *pReply = NULL ;
      if ( _isClosed )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if ( _modifiedCurrent )
      {
         delete _modifiedCurrent ;
         _modifiedCurrent = NULL ;
      }
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
      if ( -1 == _offset )
      {
         _offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
      }
      else
      {
         _offset += ossRoundUpToMultipleX ( *(INT32*)&_pReceiveBuffer[_offset],
                                            4 ) ;
      }
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
      localobj.init ( &_pReceiveBuffer [ _offset ] ) ;
      obj = localobj.copy () ;
      ++ _totalRead ;
   done :
      _isDeleteCurrent = FALSE ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::current ( BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      MsgOpReply *pReply = NULL ;
      BSONObj localobj ;
      if ( _isClosed )
      {
         rc = SDB_DMS_CONTEXT_IS_CLOSE ;
         goto error ;
      }
      if(_isDeleteCurrent)
      {
         rc = SDB_CURRENT_RECORD_DELETED ;
         goto error ;
      }
      if ( !&obj )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _modifiedCurrent )
      {
         obj=_modifiedCurrent->copy() ;
         goto done ;
      }
      if ( !_pReceiveBuffer )
      {
         if ( -1 == _offset && _connection )
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
      if ( -1 == _offset )
      {
         _offset = ossRoundUpToMultipleX ( sizeof ( MsgOpReply ), 4 ) ;
      }
      if ( _offset > pReply->header.messageLength ||
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
      localobj.init ( &_pReceiveBuffer [ _offset ] ) ;
      obj = localobj.copy () ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCursorImpl::close()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      if ( _isClosed || -1 == _contextID )
      {
         goto done ;
      }
      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      rc = _killCursor() ;
      if ( rc )
      {
         goto error ;
      }
      _close() ;
   done :
      if ( locked )
      {
         _connection->unlock () ;
      }
      if ( SDB_OK == rc )
      {
         if ( NULL != _connection )
         {
            _detachConnection() ;
         }
         if ( NULL != _collection )
         {
            _detachCollection() ;
         }
      }
      return rc ;
   error :
      goto done ;
   }


/*
   PD_TRACE_DECLARE_FUNCTION ( SDB_CLIENT_UPDATECURRENT, "_sdbCursorImpl::updateCurrent" )
   INT32 _sdbCursorImpl::updateCurrent ( BSONObj &rule )
   {
      PD_TRACE_ENTRY ( SDB_CLIENT_UPDATECURRENT ) ;
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BSONObj updateCondition ;
      BSONObj modifiedObj ;
      BSONElement it ;
      _sdbCursor *tempQuery = NULL ;
      if(_isDeleteCurrent)
      {
         rc = SDB_CURRENT_RECORD_DELETED ;
         goto error ;
      }
      if ( !_collection )
      {
         rc = SDB_CLT_OBJ_NOT_EXIST ;
         goto error ;
      }
      rc = current ( obj ) ;
      if ( rc )
      {
         goto error ;
      }
      it = obj.getField ( CLIENT_RECORD_ID_FIELD ) ;
      if ( it.eoo() )
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }
      if ( BSON_EOO != bson_find ( &it, &obj, CLIENT_RECORD_ID_FIELD ) )
      {
         rc = bson_append_element ( &updateCondition, NULL, &it ) ;
         if ( rc )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         rc = bson_finish ( &updateCondition ) ;
         if ( rc )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      else
      {
         rc = SDB_CORRUPTED_RECORD ;
         goto error ;
      }
      rc = _collection->update ( &rule, &updateCondition,
                                 &_hintObj ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _collection->query(&tempQuery, updateCondition,
               _sdbStaticObject, _sdbStaticObject, _hintObj, 0, 1) ;
      if ( rc )
      {
         goto error ;
      }
      rc = tempQuery->next( modifiedObj ) ;
      if ( rc )
      {
         goto error ;
      }
   if ( !_modifiedCurrent )
   {
      _modifiedCurrent = (bson*)SDB_OSS_MALLOC ( sizeof(bson) ) ;
      if ( !_modifiedCurrent )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      bson_init ( _modifiedCurrent ) ;
   }
   rc = bson_copy ( _modifiedCurrent, &modifiedObj ) ;
   if ( BSON_OK != rc )
   {
      rc = SDB_SYS ;
      goto error ;
   }
   done :
      if ( tempQuery )
      {
         delete  tempQuery ;
         tempQuery = NULL ;
      }
      bson_destroy ( &updateCondition ) ;
      bson_destroy ( &hintObj ) ;
      bson_destroy ( &modifiedObj ) ;
      PD_TRACE_EXITRC ( SDB_CLIENT_UPDATECURRENT, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB_CLIENT_DELCURRENT, "_sdbCursorImpl::delCurrent" )
   INT32 _sdbCursorImpl::delCurrent ()
   {
      PD_TRACE_ENTRY ( SDB_CLIENT_DELCURRENT ) ;
      INT32 rc = SDB_OK ;
      bson obj ;
      bson_init ( &obj ) ;
      bson deleteCondition ;
      bson_init ( &deleteCondition ) ;
      bson_iterator it ;
      if ( !_collection )
      {
         rc = SDB_CLT_OBJ_NOT_EXIST ;
         goto error ;
      }
      if(_isDeleteCurrent)
      {
         rc = SDB_CURRENT_RECORD_DELETED ;
         goto error ;
      }
      rc = current ( obj ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( BSON_EOO != bson_find ( &it, &obj, CLIENT_RECORD_ID_FIELD ) )
      {
         rc = bson_append_element ( &deleteCondition, NULL, &it ) ;
         if ( rc )
         {
            rc = SDB_SYS ;
            goto error ;
         }
         rc = bson_finish ( &deleteCondition ) ;
         if ( rc )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      rc = _collection->del ( &deleteCondition, &_hintObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      bson_destroy ( &deleteCondition ) ;
      _isDeleteCurrent = TRUE ;
      PD_TRACE_EXITRC ( SDB_CLIENT_DELCURRENT, rc ) ;
      return rc ;
   error :
      goto done ;
   }*/


   /*
    * sdbCollectionImpl
    * Collection Implementation
    */
   _sdbCollectionImpl::_sdbCollectionImpl () :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
   _pAppendOIDBuffer ( NULL ),
   _appendOIDBufferSize ( 0 )
   {
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) ) ;
      ossMemset ( _collectionName, 0, sizeof ( _collectionName ) ) ;
      ossMemset ( _collectionFullName, 0, sizeof ( _collectionFullName ) ) ;
   }

   _sdbCollectionImpl::_sdbCollectionImpl ( CHAR *pCollectionFullName ) :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
   _pAppendOIDBuffer ( NULL ),
   _appendOIDBufferSize ( 0 )
   {
      _setName ( pCollectionFullName ) ;
   }

   _sdbCollectionImpl::_sdbCollectionImpl ( CHAR *pCollectionSpaceName,
                                            CHAR *pCollectionName ) :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
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
      CHAR collectionFullName [ CLIENT_COLLECTION_NAMESZ +
                                CLIENT_CS_NAMESZ +
                                1 + 1 ] = { 0 } ;

      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) );
      ossMemset ( _collectionName, 0, sizeof ( _collectionName ) ) ;
      ossMemset ( _collectionFullName, 0, sizeof ( _collectionFullName ) ) ;
      if ( !pCollectionFullName ||
           ossStrlen ( pCollectionFullName ) >
           CLIENT_COLLECTION_NAMESZ + CLIENT_CS_NAMESZ + 1 )
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
      std::set<ossValuePtr> copySet ;
      std::set<ossValuePtr>::iterator it ;
      copySet = _cursors ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbCursorImpl*)(*it))->_detachCollection() ;
      }
      _cursors.clear() ;

      if ( _connection )
      {
         _connection->_unregCollection ( this ) ;
      }
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
      if ( !input.getField( CLIENT_RECORD_ID_FIELD ).eoo() )
      {
         output = input ;
         goto done ;
      }
      oid._oid.init() ;
      oidLen = oidEle.size() ;
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
         _pAppendOIDBuffer = (CHAR*)SDB_OSS_REALLOC(_pAppendOIDBuffer,
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

   void _sdbCollectionImpl::_setConnection ( _sdb *connection )
   {
      _connection = (_sdbImpl*)connection ;
      _connection->_regCollection ( this ) ;
   }

   void* _sdbCollectionImpl::_getConnection ()
   {
      return _connection ;
   }

   void _sdbCollectionImpl::_dropConnection()
   {
      _connection = NULL ;
   }

   void _sdbCollectionImpl::_regCursor ( _sdbCursorImpl *cursor )
   {
      lock () ;
      _cursors.insert ( (ossValuePtr)cursor ) ;
      unlock () ;
   }

   void _sdbCollectionImpl::_unregCursor ( _sdbCursorImpl * cursor )
   {
      lock () ;
      _cursors.erase ( (ossValuePtr)cursor ) ;
      unlock () ;
   }

   INT32 _sdbCollectionImpl::getCount ( SINT64 &count,
                                        const BSONObj &condition,
                                        const BSONObj &hint )
   {
      INT32 rc            = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      BSONObj newObj ;
      BSONObj countObj ;

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
                                     0, 0, -1, -1, &pCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL == pCursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = pCursor->next ( countObj ) ;
      if ( rc )
      {
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

   INT32 _sdbCollectionImpl::bulkInsert ( SINT32 flags,
                                          vector<BSONObj> &obj
                                        )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = 0 ;
      BOOLEAN result ;
      SINT32 count = 0 ;
      SINT32 num = obj.size() ;

      if ( _collectionFullName[0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      if ( num <= 0 )
      {
         goto exit ;
      }
      for ( count = 0; count < num; ++count )
      {
         BSONObj temp ;
         rc = _appendOID ( obj[count], temp ) ;
         if ( rc )
         {
            goto exit ;
         }
         if ( 0 == count )
            rc = clientBuildInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                           _collectionFullName, flags, 0,
                                           temp.objdata(),
                                           _connection->_endianConvert ) ;
         else
            rc = clientAppendInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                            temp.objdata(),
                                            _connection->_endianConvert ) ;
         if ( rc )
         {
            goto exit ;
         }
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer,
                                       &_receiveBufferSize,
                                       contextID,
                                       result ) ;
      if ( rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   exit:
      return rc ;
   done :
      _connection->unlock () ;
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::insert ( const BSONObj &obj, OID *id )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = 0 ;
      BOOLEAN result ;
      BSONObj temp ;
      if ( _collectionFullName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      rc = _appendOID ( obj, temp ) ;
      if ( rc )
      {
         goto exit ;
      }
      rc = clientBuildInsertMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, 0, 0, temp.objdata(),
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto exit ;
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( id )
      {
         *id = temp.getField ( CLIENT_RECORD_ID_FIELD ).__oid();
      }

   exit :
      return rc ;
   done :
      _connection->unlock () ;
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::update ( const BSONObj &rule,
                                      const BSONObj &condition,
                                      const BSONObj &hint,
                                      INT32 flag )
   {
      return _update ( rule, condition, hint, flag ) ;
   }

   INT32 _sdbCollectionImpl::upsert ( const BSONObj &rule,
                                      const BSONObj &condition,
                                      const BSONObj &hint,
                                      const BSONObj &setOnInsert,
                                      INT32 flag )
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
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _update ( rule, condition, newHint, flag | FLG_UPDATE_UPSERT ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_update ( const BSONObj &rule,
                                       const BSONObj &condition,
                                       const BSONObj &hint,
                                       INT32 flag )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = 0 ;
      BOOLEAN result ;
      if ( _collectionFullName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      rc = clientBuildUpdateMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, flag, 0,
                                     condition.objdata(),
                                     rule.objdata(),
                                     hint.objdata(),
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto exit ;
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   exit:
      return rc ;
   done :
      _connection->unlock () ;
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::del ( const BSONObj &condition,
                                   const BSONObj &hint )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      if ( _collectionFullName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      rc = clientBuildDeleteMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     _collectionFullName, 0, 0,
                                     condition.objdata(),
                                     hint.objdata(),
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto exit ;
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }


   exit:
      return rc ;
   done :
      _connection->unlock () ;
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::pop( const BSONObj &option )
   {
      INT32 rc = SDB_OK ;
      BSONElement lidEle ;
      BSONObj cmdOption ;
      BSONObjBuilder builder ;

      if ( '\0' == _collectionFullName[0] || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( option.isEmpty() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
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
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = _connection->_runCommand( CMD_ADMIN_PREFIX CMD_NAME_POP,
                                     &cmdOption ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::query ( _sdbCursor **cursor,
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

      if ( _collectionFullName [0] == '\0' || !_connection || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto done;
      }
      if ( 0 != flags )
      {
         rc = regulateQueryFlags( &newFlags, flags ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      if ( 1 == numToReturn )
      {
         newFlags |= FLG_QUERY_WITH_RETURNDATA ;
      }

      rc = _connection->_runCommand( _collectionFullName,
                                     &condition, &selected,
                                     &orderBy, &hint,
                                     newFlags, 0, numToSkip, numToReturn,
                                     &pCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL == pCursor )
      {
         rc = _connection->_buildEmptyCursor( &pCursor ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         if ( NULL == pCursor )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }

      ((_sdbCursorImpl*)pCursor)->_attachCollection( this ) ;

      *cursor = pCursor ;
   done :
      return rc ;
   error :
      goto done;
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

      rc = query( cursor,
                  condition,
                  selected,
                  orderBy,
                  hint,
                  numToSkip,
                  1,
                  flag | FLG_QUERY_WITH_RETURNDATA ) ;
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

         if ( !hint.isEmpty() )
         {
            newHintBuilder.appendElements( hint ) ;
         }
         newHintBuilder.appendObject( FIELD_NAME_MODIFY, modify.objdata() ) ;
         newHint = newHintBuilder.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_SYS ;
         goto error ;
      }

      flag |= FLG_QUERY_MODIFY ;

      rc = query( cursor, condition, selected, orderBy, newHint,
                  numToSkip, numToReturn, flag ) ;

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
      const CHAR *p = NULL ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObjBuilder bo ;
      BSONObj newHint ;

      if ( _collectionFullName [0] == '\0' || !_connection || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto done;
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

      p = CMD_ADMIN_PREFIX CMD_NAME_GET_QUERYMETA ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    p, 0, 0, numToSkip,
                                    numToReturn,
                                    condition.objdata(),
                                    NULL,
                                    orderBy.objdata(),
                                    newHint.objdata(),
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto done ;
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      *cursor = (_sdbCursor*)( new(std::nothrow) _sdbCursorImpl () ) ;
      if ( !*cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*cursor)->_attachConnection ( _connection ) ;
      ((_sdbCursorImpl*)*cursor)->_attachCollection ( this ) ;
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;
   done :
      return rc ;
   error :
      if ( NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }

      _connection->unlock () ;
      goto done;
   }

   /*PD_TRACE_DECLARE_FUNCTION ( SDB_CLIENT__RENAMEATTEMP, "_sdbCollectionImpl::_renameAttempt" )
   void _sdbCollectionImpl::_renameAttempt ( const CHAR *pOldName,
                                             const CHAR *pNewName )
   {
      PD_TRACE_ENTRY ( SDB_CLIENT__RENAMEATTEMP ) ;
      INT32 newNameLen = ossStrlen ( pNewName ) ;
      if ( (UINT32)newNameLen > sizeof(_collectionName) )
         goto exit ;
      if ( ossStrncmp ( _collectionName, pOldName,
                        CLIENT_COLLECTION_NAMESZ ) == 0 )
      {
         ossMemset ( _collectionName, 0,
                     sizeof ( _collectionName ) ) ;
         ossMemset ( _collectionFullName, 0,
                     sizeof ( _collectionFullName ) ) ;
         ossStrncpy ( _collectionName, pNewName,
                      CLIENT_COLLECTION_NAMESZ ) ;
         ossStrncpy ( _collectionFullName, _collectionSpaceName,
                      CLIENT_CS_NAMESZ ) ;
         ossStrncat ( _collectionFullName, ".", 1 ) ;
         ossStrncat ( _collectionFullName, pNewName,
                      newNameLen ) ;
      }
   exit:
      PD_TRACE_EXIT ( SDB_CLIENT__RENAMEATTEMP );
      return ;
   }

   PD_TRACE_DECLARE_FUNCTION ( SDB_CLIENT_RENAME, "_sdbCollectionImpl::rename" )
   INT32 _sdbCollectionImpl::rename ( const CHAR *pNewName )
   {
      PD_TRACE_ENTRY ( SDB_CLIENT_RENAME ) ;
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj obj ;
      BOOLEAN locked = FALSE ;
      if ( !pNewName || _collectionSpaceName[0] == '\0' ||
           _collectionName[0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      obj = BSON ( FIELD_NAME_COLLECTIONSPACE << _collectionSpaceName <<
                   FIELD_NAME_OLDNAME << _collectionName <<
                   FIELD_NAME_NEWNAME << pNewName ) ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTION,
                                    0, 0, -1, -1, obj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->_changeCollectionName ( _collectionSpaceName,
                                           _collectionName,
                                           pNewName ) ;
   done :
      if ( locked )
         _connection->unlock () ;
      PD_TRACE_EXITRC ( SDB_CLIENT_RENAME, rc );
      return rc ;
   error :
      goto done ;

   }*/

   INT32 _sdbCollectionImpl::_createIndex ( const BSONObj &indexDef,
                                            const CHAR *pName,
                                            BOOLEAN isUnique,
                                            BOOLEAN isEnforced,
                                            INT32 sortBufferSize )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj indexObj ;
      BSONObj newObj ;
      BSONObj hintObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection ||
           !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( sortBufferSize < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      indexObj = BSON ( IXM_FIELD_NAME_KEY << indexDef <<
                        IXM_FIELD_NAME_NAME << pName <<
                        IXM_FIELD_NAME_UNIQUE << (isUnique ? true : false) <<
                        IXM_FIELD_NAME_ENFORCED << (isEnforced ? true : false) ) ;

      newObj = BSON ( FIELD_NAME_COLLECTION << _collectionFullName <<
                      FIELD_NAME_INDEX << indexObj ) ;

      hintObj = BSON ( IXM_FIELD_NAME_SORT_BUFFER_SIZE << sortBufferSize ) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_CREATE_INDEX,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL,
                                    hintObj.objdata(),
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::createIndex ( const BSONObj &indexDef,
                                           const CHAR *pName,
                                           BOOLEAN isUnique,
                                           BOOLEAN isEnforced )
   {
      return _createIndex ( indexDef, pName, isUnique, isEnforced,
                            SDB_INDEX_SORT_BUFFER_DEFAULT_SIZE) ;
   }

   INT32 _sdbCollectionImpl::createIndex ( const BSONObj &indexDef,
                                           const CHAR *pName,
                                           BOOLEAN isUnique,
                                           BOOLEAN isEnforced,
                                           INT32 sortBufferSize )
   {
      return _createIndex ( indexDef, pName, isUnique, isEnforced, sortBufferSize ) ;
   }

   INT32 _sdbCollectionImpl::getIndexes ( _sdbCursor **cursor,
                                          const CHAR *pName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj queryCond ;
      BSONObj newObj ;
      if ( _collectionFullName [0] == '\0' || !_connection || !cursor )
      {
         rc = SDB_INVALIDARG ;
         return rc ;
      }
      /* build query condition */
      if ( pName )
      {
         queryCond = BSON ( IXM_FIELD_NAME_INDEX_DEF "." IXM_FIELD_NAME_NAME <<
                            pName ) ;
      }
      /* build collection name */
      newObj = BSON ( FIELD_NAME_COLLECTION << _collectionFullName ) ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_GET_INDEXES,
                                    0, 0, -1,
                                    -1, pName?queryCond.objdata():NULL,
                                    NULL, NULL,
                                    newObj.objdata(),
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto exit ;
      }
      _connection->lock () ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

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
      ((_sdbCursorImpl*)*cursor)->_attachConnection ( _connection ) ;
      ((_sdbCursorImpl*)*cursor)->_attachCollection ( this ) ;
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;

   exit:
      return rc ;
   done :
      _connection->unlock () ;
      goto exit ;
   error :
      if ( NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }

      goto done ;

   }

   INT32 _sdbCollectionImpl::dropIndex ( const CHAR *pName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj indexObj ;
      BSONObj newObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection ||
           !pName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      indexObj = BSON ( "" << pName ) ;
      newObj = BSON ( FIELD_NAME_COLLECTION << _collectionFullName <<
                      FIELD_NAME_INDEX << indexObj ) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_DROP_INDEX,
                                    0, 0, -1, -1, newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

    done :
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::create ()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << _collectionFullName ) ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;

      rc = insertCachedObject( _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::drop ()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << _collectionFullName ) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;

      rc = removeCachedObject( _connection->_getCachedContainer(),
                               _collectionFullName, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      if ( locked )
         _connection->unlock () ;
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
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection ||
         !pSourceGroupName || 0 == ossStrcmp ( pSourceGroupName, "" ) ||
         !pTargetGroupName || 0 == ossStrcmp ( pTargetGroupName, "" ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( CAT_COLLECTION_NAME << _collectionFullName <<
                      CAT_SOURCE_NAME << pSourceGroupName <<
                      CAT_TARGET_NAME << pTargetGroupName <<
                      CAT_SPLITQUERY_NAME << splitCondition <<
                      CAT_SPLITENDQUERY_NAME << splitEndCondition) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::split ( const CHAR *pSourceGroupName,
                                     const CHAR *pTargetGroupName,
                                     double percent )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection ||
         !pSourceGroupName || 0 == ossStrcmp ( pSourceGroupName, "" ) ||
         !pTargetGroupName || 0 == ossStrcmp ( pTargetGroupName, "" ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( percent <= 0.0 || percent > 100.0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( CAT_COLLECTION_NAME << _collectionFullName <<
                      CAT_SOURCE_NAME << pSourceGroupName <<
                      CAT_TARGET_NAME << pTargetGroupName <<
                      CAT_SPLITPERCENT_NAME << percent ) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done :
      if ( locked )
         _connection->unlock () ;
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
      BOOLEAN result ;
      SINT64 contextID    = 0 ;
      _sdbCursor *cursor  = NULL ;
      BOOLEAN locked      = FALSE ;
      BSONObjBuilder bob ;
      BSONObj newObj  ;
      BSONObj countObj ;
      if ( _collectionFullName[0] == '\0' || !_connection ||
            !pSourceGroupName || 0 == ossStrcmp ( pSourceGroupName, "" ) ||
            !pTargetGroupName || 0 == ossStrcmp ( pTargetGroupName, "" ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
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
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                    0, 0, 0, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      cursor = (_sdbCursor*) (new(std::nothrow) sdbCursorImpl () ) ;
      if ( !cursor  )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)cursor)->_attachConnection ( _connection ) ;
      ((_sdbCursorImpl*)cursor)->_attachCollection ( this ) ;
      ((_sdbCursorImpl*)cursor)->_contextID = contextID ;
      rc = cursor->next ( countObj ) ;
      if ( rc )
      {
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
      delete ( cursor ) ;
   done :
      if ( locked )
         _connection->unlock () ;
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
      BOOLEAN result ;
      SINT64 contextID    = 0 ;
      _sdbCursor *cursor  = NULL ;
      BSONObjBuilder bob ;
      BSONObj newObj ;
      BSONObj countObj ;
      BOOLEAN locked = FALSE ;
      if ( _collectionFullName [0] == '\0' || !_connection ||
         !pSourceGroupName || 0 == ossStrcmp ( pSourceGroupName, "" ) ||
         !pTargetGroupName || 0 == ossStrcmp ( pTargetGroupName, "" ) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( percent <= 0.0 || percent > 100.0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
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
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_SPLIT,
                                    0, 0, -1, -1,
                                    newObj.objdata(),
                                    NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      cursor = (_sdbCursor*) (new(std::nothrow) sdbCursorImpl () ) ;
      if ( !cursor  )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)cursor)->_attachConnection ( _connection ) ;
      ((_sdbCursorImpl*)cursor)->_attachCollection ( this ) ;
      ((_sdbCursorImpl*)cursor)->_contextID = contextID ;
      rc = cursor->next ( countObj ) ;
      if ( rc )
      {
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
      delete ( cursor ) ;
   done :
      if ( locked )
         _connection->unlock () ;
   return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionImpl::aggregate ( _sdbCursor **cursor,
                            std::vector<bson::BSONObj> &obj )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      SINT32 count = 0 ;
      SINT32 num = obj.size() ;

      if ( _collectionFullName[0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      for ( count = 0; count < num; ++count )
      {
         if ( 0 == count )
         rc = clientBuildAggrRequestCpp( &_pSendBuffer, &_sendBufferSize,
                                           _collectionFullName, obj[count].objdata(),
                                           _connection->_endianConvert ) ;
         else
         rc = clientAppendAggrRequestCpp ( &_pSendBuffer, &_sendBufferSize,
                                           obj[count].objdata(), _connection->_endianConvert ) ;
         if ( rc )
         {
            goto error ;
         }
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                    contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }
      *cursor = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl() ) ;
      if ( !*cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*cursor)->_attachConnection( _connection ) ;
      ((_sdbCursorImpl*)*cursor)->_attachCollection( this ) ;
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;

   done:
      if ( locked )
      {
         _connection->unlock () ;
      }
      return rc ;
error:
      if ( NULL != *cursor )
      {
         delete *cursor ;
         *cursor = NULL ;
      }

      goto done ;
   }

   INT32 _sdbCollectionImpl::detachCollection ( const CHAR *subClFullName)
   {
      INT32 rc         = SDB_OK ;
      BOOLEAN locked   = FALSE ;
      SINT64 contextID = 0 ;
      INT32 nameLength = 0 ;
      BOOLEAN result ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_UNLINK_CL ) ;
      if ( !subClFullName || !_connection ||
            (nameLength = ossStrlen ( subClFullName) ) >
            CLIENT_COLLECTION_NAMESZ ||
            _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( FIELD_NAME_NAME, _collectionFullName ) ;
      ob.append ( FIELD_NAME_SUBCLNAME, subClFullName ) ;
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     command.c_str(), 0, 0, 0,
                                     -1, newObj.objdata(),
                                     NULL, NULL, NULL,
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto done ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                        contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
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
      goto done ;
   }

   INT32 _sdbCollectionImpl::attachCollection ( const CHAR *subClFullName,
                                                const bson::BSONObj &options)
   {
      INT32 rc         = SDB_OK ;
      BOOLEAN locked   = FALSE ;
      SINT64 contextID = 0 ;
      INT32 nameLength = 0 ;
      BOOLEAN result ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_LINK_CL ) ;
      if ( !subClFullName || !_connection ||
         ( nameLength = ossStrlen ( subClFullName) ) >
           CLIENT_COLLECTION_NAMESZ ||
           _collectionFullName[0] == '\0' )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( FIELD_NAME_NAME, _collectionFullName ) ;
      ob.append ( FIELD_NAME_SUBCLNAME, subClFullName ) ;
      {
         BSONObjIterator it( options ) ;
         while ( it.more() )
         {
            ob.append ( it.next() ) ;
         }
      }
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     command.c_str(), 0, 0, 0,
                                     -1, newObj.objdata(),
                                     NULL, NULL, NULL,
                                     _connection->_endianConvert ) ;
      if ( rc )
      {
         goto done ;
      }
      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                        contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
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
      goto done ;
   }

   INT32 _sdbCollectionImpl::_alterCollection2 ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      BSONObjBuilder bob ;
      BSONElement ele ;
      BSONObj newObj ;
      string collectionS ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION ) ;
      if ( '\0' == _collectionFullName[0] || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      collectionS = string (_collectionFullName) ;
      try
      {
         bob.append( FIELD_NAME_ALTER_TYPE, SDB_ALTER_CL ) ;
         bob.append( FIELD_NAME_VERSION, SDB_ALTER_VERSION ) ;
         bob.append ( FIELD_NAME_NAME, collectionS ) ;

         ele = options.getField( FIELD_NAME_ALTER ) ;
         if ( Object == ele.type() )
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
      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
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

   INT32 _sdbCollectionImpl::_alterCollection1 ( const bson::BSONObj &options )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      BSONObjBuilder bob ;
      BSONObj newObj ;
      string collectionS ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_COLLECTION ) ;
      if ( '\0' == _collectionFullName[0] || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      collectionS = string (_collectionFullName) ;
      try
      {
         bob.append ( FIELD_NAME_NAME, collectionS ) ;
         bob.append ( FIELD_NAME_OPTIONS, options ) ;
         newObj = bob.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
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

   INT32 _sdbCollectionImpl::explain (
                              _sdbCursor **cursor,
                              const bson::BSONObj &condition,
                              const bson::BSONObj &select,
                              const bson::BSONObj &orderBy,
                              const bson::BSONObj &hint,
                              INT64 numToSkip,
                              INT64 numToReturn,
                              INT32 flags,
                              const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      if ( '\0' == _collectionFullName[0] || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         bob.append( FIELD_NAME_HINT, hint ) ;
         bob.append( FIELD_NAME_OPTIONS, options ) ;
         newObj= bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = query( cursor, condition, select, orderBy, newObj,
                  numToSkip, numToReturn, flags | FLG_QUERY_EXPLAIN ) ;
      if ( rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::createLob( _sdbLob **lob, const bson::OID *oid )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj obj ;
      BSONElement ele ;
      BSONType bType ;
      SINT64 contextID = -1 ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      const CHAR *bsonBuf = NULL ;
      OID oidObj ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( oid )
      {
         oidObj = *oid ;
      }
      else
      {
         oidObj = OID::gen() ;
      }
      try
      {
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, &oidObj ) ;
         bob.append( FIELD_NAME_LOB_OPEN_MODE, SDB_LOB_CREATEONLY ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = clientBuildOpenLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     obj.objdata(), 0, 1, 0,
                                     _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      bsonBuf = _pReceiveBuffer + sizeof( MsgOpReply ) ;
      try
      {
         obj = BSONObj( bsonBuf ).getOwned() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
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
      ((_sdbLobImpl*)*lob)->_attachConnection( _connection ) ;
      ((_sdbLobImpl*)*lob)->_attachCollection( this ) ;
      ((_sdbLobImpl*)*lob)->_oid = oidObj ;
      ((_sdbLobImpl*)*lob)->_contextID = contextID ;
      ((_sdbLobImpl*)*lob)->_isOpen = TRUE ;
      ((_sdbLobImpl*)*lob)->_mode = SDB_LOB_CREATEONLY ;
      ((_sdbLobImpl*)*lob)->_lobSize = 0 ;
      ((_sdbLobImpl*)*lob)->_createTime = 0 ;
      ((_sdbLobImpl*)*lob)->_modificationTime = 0 ;

      ele = obj.getField( FIELD_NAME_LOB_SIZE ) ;
      bType = ele.type() ;
      if ( NumberInt == bType || NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_lobSize = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ele = obj.getField( FIELD_NAME_LOB_CREATETIME ) ;
      bType = ele.type() ;
      if ( NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_createTime = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ele = obj.getField( FIELD_NAME_LOB_MODIFICATION_TIME ) ;
      bType = ele.type() ;
      if ( NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_modificationTime = ele.numberLong() ;
      }
      else
      {
         ((_sdbLobImpl*)*lob)->_modificationTime =
            ((_sdbLobImpl*)*lob)->_createTime ;
      }
      ele = obj.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
      bType = ele.type() ;
      if ( NumberInt == bType )
      {
         ((_sdbLobImpl*)*lob)->_pageSize =  ele.numberInt() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error:
      if ( *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::removeLob( const bson::OID &oid )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      BSONObjBuilder bob ;
      BSONObj meta ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (OID *)(&oid) ) ;
         meta = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = clientBuildRemoveLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                       meta.objdata(), 0, 1, 0,
                                       _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      if ( locked )
         _connection->unlock() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::truncateLob( const bson::OID &oid, INT64 length )
   {
      INT32 rc = SDB_OK ;
      SINT64 contextID = -1 ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      BSONObjBuilder bob ;
      BSONObj meta ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( length < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (OID *)(&oid) ) ;
         bob.append( FIELD_NAME_LOB_LENGTH, length ) ;
         meta = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = clientBuildTruncateLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                         meta.objdata(), 0, 1, 0,
                                         _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      if ( locked )
         _connection->unlock() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::openLob( _sdbLob **lob, const bson::OID &oid,
                                          SDB_LOB_OPEN_MODE mode )
   {
      INT32 rc = SDB_OK ;
      INT32 flag = 0 ;
      BSONObjBuilder bob ;
      BSONObj obj ;
      BSONElement ele ;
      BSONType bType ;
      SINT64 contextID = -1 ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      const CHAR *bsonBuf = NULL ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_LOB_READ != mode && SDB_LOB_WRITE != mode )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_LOB_READ == mode )
      {
         flag |= FLG_LOBOPEN_WITH_RETURNDATA ;
      }

      try
      {
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         bob.appendOID( FIELD_NAME_LOB_OID, (bson::OID *)&oid ) ;
         bob.append( FIELD_NAME_LOB_OPEN_MODE, mode ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = clientBuildOpenLobMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                     obj.objdata(), flag,
                                     1, 0, _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      bsonBuf = _pReceiveBuffer + sizeof( MsgOpReply ) ;
      try
      {
         obj = BSONObj( bsonBuf ).getOwned() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
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
      ((_sdbLobImpl*)*lob)->_attachConnection( _connection ) ;
      ((_sdbLobImpl*)*lob)->_attachCollection( this ) ;
      ((_sdbLobImpl*)*lob)->_oid = oid ;
      ((_sdbLobImpl*)*lob)->_contextID = contextID ;
      ((_sdbLobImpl*)*lob)->_isOpen = TRUE ;
      ((_sdbLobImpl*)*lob)->_mode = mode ;
      ((_sdbLobImpl*)*lob)->_currentOffset = 0 ;
      ((_sdbLobImpl*)*lob)->_cachedOffset = -1 ;
      ((_sdbLobImpl*)*lob)->_cachedSize =0 ;
      ((_sdbLobImpl*)*lob)->_dataCache = 0 ;
      ele = obj.getField( FIELD_NAME_LOB_SIZE ) ;
      bType = ele.type() ;
      if ( NumberInt == bType || NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_lobSize = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ele = obj.getField( FIELD_NAME_LOB_CREATETIME ) ;
      bType = ele.type() ;
      if ( NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_createTime = ele.numberLong() ;
      }
      else
      {
         rc = SDB_SYS ;
         goto error ;
      }
      ele = obj.getField( FIELD_NAME_LOB_MODIFICATION_TIME ) ;
      bType = ele.type() ;
      if ( NumberLong == bType )
      {
         ((_sdbLobImpl*)*lob)->_modificationTime = ele.numberLong() ;
      }
      else
      {
         ((_sdbLobImpl*)*lob)->_modificationTime =
            ((_sdbLobImpl*)*lob)->_createTime ;
      }
      ele = obj.getField( FIELD_NAME_LOB_PAGE_SIZE ) ;
      bType = ele.type() ;
      if ( NumberInt == bType )
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
      {
      const MsgLobTuple *tuple = NULL ;
      const CHAR *body = NULL ;
      UINT32 retMsgLen =
         (UINT32)(((MsgHeader*)_pReceiveBuffer)->messageLength);
      UINT32 tupleOffset =
         ossRoundUpToMultipleX( sizeof( MsgOpReply ) + obj.objsize(), 4 ) ;
      if ( retMsgLen > tupleOffset )
      {
         ((_sdbLobImpl*)*lob)->_pReceiveBuffer = _pReceiveBuffer ;
         _pReceiveBuffer = NULL ;
         ((_sdbLobImpl*)*lob)->_receiveBufferSize = _receiveBufferSize ;
         _receiveBufferSize = 0 ;
         tuple = (MsgLobTuple *)(((_sdbLobImpl*)*lob)->_pReceiveBuffer +
                                  tupleOffset) ;
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
         ((_sdbLobImpl*)*lob)->_currentOffset = 0 ;
         ((_sdbLobImpl*)*lob)->_cachedOffset = 0 ;
         ((_sdbLobImpl*)*lob)->_cachedSize = tuple->columns.len ;
         ((_sdbLobImpl*)*lob)->_dataCache = body ;
      }
      }

   done:
      if ( locked )
         _connection->unlock () ;
      return rc ;
   error:
      if ( *lob )
      {
         delete *lob ;
         *lob = NULL ;
      }
      goto done ;
   }

   INT32 _sdbCollectionImpl::listLobs ( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj obj ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      try
      {
         bob.append( FIELD_NAME_COLLECTION, _collectionFullName ) ;
         obj = bob.obj() ;
      }
      catch ( std::exception &e )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS, obj, cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::listLobPieces( _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_COLLECTION, this->getFullName() ) ;
         queryBuilder.appendBool( FIELD_NAME_LOB_LIST_PIECES_MODE, TRUE ) ;
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCmdOfLob( CMD_ADMIN_PREFIX CMD_NAME_LIST_LOBS,
                         query, cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::_runCmdOfLob ( const CHAR *cmd, const BSONObj &obj,
                                            _sdbCursor **cursor )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      SINT64 contextID = -1 ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL == cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    cmd, 0, 0, -1, -1,
                                    obj.objdata(), NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( -1 != contextID )
      {
         if ( *cursor )
         {
            delete *cursor ;
            *cursor = NULL ;
         }
         *cursor = (_sdbCursor*)( new(std::nothrow) _sdbCursorImpl () ) ;
         if ( NULL == *cursor )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ((_sdbCursorImpl*)*cursor)->_attachConnection ( _connection ) ;
         ((_sdbCursorImpl*)*cursor)->_attachCollection ( this ) ;
         ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;
      }

   done:
      if ( locked )
         _connection->unlock() ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::truncate()
   {
      INT32 rc = SDB_OK ;
      BSONObj obj ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result = TRUE ;
      SINT64 contextID = -1 ;

      if ( '\0' == _collectionFullName[0] || NULL == _connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      obj = BSON( FIELD_NAME_COLLECTION << _collectionFullName ) ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_TRUNCATE,
                                    0, 0, -1, -1,
                                    obj.objdata(), NULL, NULL, NULL,
                                    _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      _connection->lock () ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      rc = updateCachedObject( rc, _connection->_getCachedContainer(),
                               _collectionFullName ) ;
      if ( SDB_OK != rc )
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
      goto done ;
   }

   INT32 _sdbCollectionImpl::createIdIndex( const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj obj ;
      BSONObj subObj ;

      bob.append( FIELD_NAME_NAME, SDB_ALTER_CRT_ID_INDEX ) ;
      if ( options.isEmpty() )
      {
         bob.appendNull( FIELD_NAME_ARGS ) ;
      }
      else
      {
         bob.append( FIELD_NAME_ARGS, options ) ;
      }
      subObj = bob.obj() ;
      obj = BSON( FIELD_NAME_ALTER << subObj ) ;

      rc = alterCollection( obj ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbCollectionImpl::dropIdIndex()
   {
      INT32 rc = SDB_OK ;
      BSONObjBuilder bob ;
      BSONObj obj ;
      BSONObj subObj ;

      bob.append( FIELD_NAME_NAME, SDB_ALTER_DROP_ID_INDEX ) ;
      bob.appendNull( FIELD_NAME_ARGS ) ;
      subObj = bob.obj() ;
      obj = BSON( FIELD_NAME_ALTER << subObj ) ;
      rc = alterCollection( obj ) ;
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
    * _sdbNodeImpl
    * Sdb Node Implementation
    */
   _sdbNodeImpl::_sdbNodeImpl () :
   _connection ( NULL )
   {
      ossMemset ( _hostName, 0, sizeof(_hostName) ) ;
      ossMemset ( _serviceName, 0, sizeof(_serviceName) ) ;
      ossMemset ( _nodeName, 0, sizeof(_nodeName) ) ;
      _nodeID = SDB_NODE_INVALID_NODEID ;
   }

   _sdbNodeImpl::~_sdbNodeImpl ()
   {
      if ( _connection )
      {
         _connection->_unregNode ( this ) ;
      }
   }

   INT32 _sdbNodeImpl::connect ( _sdb **dbConn )
   {
      INT32 rc = SDB_OK ;
      _sdbImpl *connection = NULL ;
      if ( !_connection || !dbConn || ossStrlen ( _hostName ) == 0 ||
           ossStrlen ( _serviceName ) == 0 )
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
         delete connection ;
      goto done ;
   }

   sdbNodeStatus _sdbNodeImpl::getStatus ( )
   {
      sdbNodeStatus sns     = SDB_NODE_UNKNOWN ;
      INT32 rc              = SDB_OK ;
      SINT64 contextID      = 0 ;
      BOOLEAN r             = FALSE ;
      CHAR *_pSendBuffer    = NULL ;
      CHAR *_pReceiveBuffer = NULL ;
      INT32 _sendBufferSize = 0 ;
      INT32 _receiveBufferSize = 0 ;
      BSONObj condition ;
      condition = BSON ( CAT_GROUPID_NAME << _replicaGroupID <<
                         CAT_NODEID_NAME << _nodeID ) ;

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_DATABASE,
                                    0, 0, 0, -1, condition.objdata(), NULL, NULL,
                                    NULL,
                                    _connection->_endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, r ) ;
      if ( rc )
      {
         if ( SDB_NET_CANNOT_CONNECT == rc )
         {
            sns = SDB_NODE_INACTIVE ;
         }
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      sns = SDB_NODE_ACTIVE ;
   done :
      if ( _pSendBuffer )
      {
         SDB_OSS_FREE ( _pSendBuffer ) ;
      }
      if ( _pReceiveBuffer )
      {
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
      }
      return sns ;
   error :
      goto done ;
   }

   INT32 _sdbNodeImpl::_stopStart ( BOOLEAN start )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      BSONObj configuration ;
      if ( !_connection || ossStrlen ( _hostName ) == 0 ||
           ossStrlen ( _serviceName ) == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      configuration = BSON ( CAT_HOST_FIELD_NAME << _hostName <<
                             PMD_OPTION_SVCNAME << _serviceName ) ;
      rc = _connection->_runCommand ( start?
                                      (CMD_ADMIN_PREFIX CMD_NAME_STARTUP_NODE) :
                                      (CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_NODE),
                                      result,
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

/*   INT32 _sdbNodeImpl::modifyConfig ( std::map<std::string,std::string>
                                             &config )
   {
      INT32 rc = SDB_OK ;
      if ( !_connection || _nodeID == SDB_NODE_INVALID_NODEID )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = _connection->modifyConfig ( _nodeID, config ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   } */

   _sdbReplicaGroupImpl::_sdbReplicaGroupImpl () :
   _connection ( NULL )
   {
      ossMemset ( _replicaGroupName, 0, sizeof(_replicaGroupName) ) ;
      _isCatalog = FALSE ;
      _replicaGroupID = 0 ;
   }

   _sdbReplicaGroupImpl::~_sdbReplicaGroupImpl ()
   {
      if ( _connection )
      {
         _connection->_unregReplicaGroup ( this ) ;
      }
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
         goto exit ;
      }
      rc = getDetail ( result ) ;
      if ( rc )
      {
         goto error ;
      }
      ele = result.getField ( CAT_GROUP_NAME ) ;
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
   exit:
      return rc ;
   done :
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::createNode ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const CHAR *pDatabasePath,
                                            const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObj pattern ;
      BSONObj tmpObj ;
      BSONObjBuilder ob ;
      BOOLEAN result ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE ) ;
      map<string,string>::iterator it ;
      if ( !_connection || ossStrlen ( _replicaGroupName ) == 0 ||
           !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;

      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;

      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;

      pattern = BSON( CAT_GROUPNAME_NAME   << 1 <<
                      CAT_HOST_FIELD_NAME  << 1 <<
                      PMD_OPTION_SVCNAME   << 1 <<
                      PMD_OPTION_DBPATH    << 1  ) ;
      tmpObj = options.filterFieldsUndotted( pattern, false ) ;
      ob.appendElements( tmpObj ) ;
      configuration = ob.obj () ;

      rc = _connection->_runCommand ( command.c_str(), result, &configuration );
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::createNode ( const CHAR *pHostName,
                                            const CHAR *pServiceName,
                                            const CHAR *pDatabasePath,
                                            map<string,string> &config )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObjBuilder ob ;
      BOOLEAN result ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE ) ;
      map<string,string>::iterator it ;
      if ( !_connection || ossStrlen ( _replicaGroupName ) == 0 ||
           !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;
      config.erase ( CAT_GROUPNAME_NAME ) ;

      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;
      config.erase ( CAT_HOST_FIELD_NAME ) ;

      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;
      config.erase ( PMD_OPTION_SVCNAME ) ;

      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;
      config.erase ( PMD_OPTION_DBPATH ) ;

      for ( it = config.begin(); it != config.end(); ++it )
      {
         ob.append ( it->first.c_str(),
                     it->second.c_str() ) ;
      }
      configuration = ob.obj () ;

      rc = _connection->_runCommand ( command.c_str(), result, &configuration );
      if ( rc )
      {
         goto error ;
      }

   done :
      return rc ;
   error :
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
      BOOLEAN result = FALSE ;
      if ( !pHostName || !pServiceName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ob.append ( CAT_GROUPNAME_NAME, _replicaGroupName ) ;
      ob.append ( FIELD_NAME_HOST, pHostName ) ;

      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

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
               continue ;
            }
            ob.append ( ele ) ;
         } // while
      } // if ( configure )
      removeInfo = ob.obj () ;

      rc = _connection->_runCommand ( pRemoveNode, result, &removeInfo ) ;
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
         rc = SDB_INVALIDARG ;
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
      if ( !_connection || !node || !primaryData )
      {
         rc = SDB_INVALIDARG ;
         goto error1 ;
      }
      pNode = new ( std::nothrow) _sdbNodeImpl () ;
      if ( !pNode )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      pNode->_replicaGroupID = _replicaGroupID ;
      pNode->_connection = this->_connection ;
      _connection->_regNode ( pNode ) ;

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
   done :
      *node = pNode ;
      return rc ;
   error :
      if ( pNode )
      {
         delete pNode ;
         pNode = NULL ;
      }
   error1 :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::getMaster ( _sdbNode **node )
   {
      INT32 rc = SDB_OK ;
      INT32 primaryNode = -1 ;
      const CHAR *primaryData = NULL ;
      BSONObj result ;
      BSONElement ele ;
      if ( !_connection || !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = getDetail ( result ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
         goto error ;
      }
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
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
         rc = SDB_RTN_NO_PRIMARY_FOUND ;
         goto error ;
      }
      {
         ele = result.getField ( CAT_GROUP_NAME ) ;
         BSONObjIterator it ( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONObj embObj ;
            BSONElement embEle = it.next() ;
            if ( Object == embEle.type() )
            {
               embObj = embEle.embeddedObject() ;
               BSONElement embEle1 = embObj.getField ( CAT_NODEID_NAME ) ;
               if ( embEle1.type() != NumberInt )
               {
                  rc = SDB_SYS ;
                  goto error ;
               }
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
         rc = SDB_SYS ;
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

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

      if ( !_connection || !node )
      {
         rc = SDB_INVALIDARG ;
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
      if ( validPositions.size() == 0 )
      {
         needGeneratePosition = TRUE ;
      }
      rc = getDetail ( result ) ;
      if ( rc )
      {
         if ( SDB_DMS_EOC == rc )
         {
            rc = SDB_CLS_GRP_NOT_EXIST ;
         }
         goto error ;
      }
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
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
         primaryNodeId = ele.numberInt () ;
         if ( -1 == primaryNodeId )
         {
            hasPrimary = FALSE ;
         }
      }
      {
         ele = result.getField ( CAT_GROUP_NAME ) ;
         BSONObj objReplicaGroupList = ele.embeddedObject() ;
         BSONObjIterator it ( objReplicaGroupList ) ;
         INT32 counter = 0 ;
         while ( it.more() )
         {
            ++counter ;
            BSONObj embObj ;
            BSONElement embEle ;
            embEle = it.next() ;
            if ( embEle.type() == Object )
            {
               embObj = embEle.embeddedObject() ;
               BSONElement embEle1 = embObj.getField ( CAT_NODEID_NAME ) ;
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
      if ( hasPrimary && 0 == primaryNodePosition )
      {
         rc = SDB_SYS ;
         goto error ;
      }
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
      if ( !_connection || !pHostName || !pServiceName || !node )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      *node = NULL ;
      rc = getDetail ( result ) ;
      if ( rc )
      {
         goto error ;
      }
      ele = result.getField ( CAT_GROUP_NAME ) ;
      if ( ele.type() != Array )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      {
         BSONObjIterator it ( ele.embeddedObject() ) ;
         while ( it.more() )
         {
            BSONElement embEle ;
            BSONObj embObj ;
            embEle = it.next() ;
            if ( embEle.type() == Object )
            {
               embObj = embEle.embeddedObject() ;
               rc = _extractNode ( node, embObj.objdata() ) ;
               if ( rc )
               {
                  goto error ;
               }
               if ( ossStrcmp ( ((_sdbNodeImpl*)(*node))->_hostName,
                                pHostName ) == 0 &&
                    ossStrcmp ( ((_sdbNodeImpl*)(*node))->_serviceName,
                                pServiceName ) == 0 )
                  break ;
               SDB_OSS_DEL ( (*node ) ) ;
               *node = NULL ;
            }
         }
      }
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
      if ( !_connection || !pNodeName || !node )
      {
         rc = SDB_INVALIDARG ;
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
         SDB_OSS_FREE ( pHostName ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbReplicaGroupImpl::start ()
   {
      INT32 rc = SDB_OK ;
      BSONObj replicaGroupName ;
      BOOLEAN result = FALSE ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP ) ;
      if ( !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << _replicaGroupName ) ;
      rc = _connection->_runCommand ( command.c_str(), result, &replicaGroupName ) ;
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
      BOOLEAN result = FALSE ;
      BSONObj configuration ;
      if ( !_connection || ossStrlen ( _replicaGroupName ) == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      configuration = BSON ( CAT_GROUPNAME_NAME << _replicaGroupName ) ;
      rc = _connection->_runCommand ( CMD_ADMIN_PREFIX CMD_NAME_SHUTDOWN_GROUP,
                                      result,
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
      BSONElement ele ;
      BSONObj newObj ;
      BSONObjIterator it ( options ) ;
      const CHAR *pKey = NULL ;
      BOOLEAN result = FALSE ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_NODE ) ;

      if ( !_connection || ossStrlen ( _replicaGroupName ) == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL == pHostName || NULL == pSvcName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bob.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
      bob.append( FIELD_NAME_HOST, pHostName ) ;
      bob.append( PMD_OPTION_SVCNAME, pSvcName ) ;
      bob.appendBool( FIELD_NAME_ONLY_ATTACH, 1 ) ;
      while ( it.more() )
      {
         ele = it.next() ;
         pKey = ele.fieldName() ;
         if ( 0 != ossStrcmp( pKey, FIELD_NAME_ONLY_ATTACH ) )
         {
            bob.append( ele ) ;
         }
      }
      newObj = bob.obj() ;

      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
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
      BSONElement ele ;
      BSONObj newObj ;
      BSONObjIterator it ( options ) ;
      const CHAR *pKey = NULL ;
      BOOLEAN result = FALSE ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_REMOVE_NODE ) ;

      if ( !_connection || ossStrlen ( _replicaGroupName ) == 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( NULL == pHostName || NULL == pSvcName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bob.append( FIELD_NAME_GROUPNAME, _replicaGroupName ) ;
      bob.append( FIELD_NAME_HOST, pHostName ) ;
      bob.append( PMD_OPTION_SVCNAME, pSvcName ) ;
      bob.appendBool( FIELD_NAME_ONLY_DETACH, 1 ) ;
      while ( it.more() )
      {
         ele = it.next() ;
         pKey = ele.fieldName() ;
         if ( 0 != ossStrcmp( pKey, FIELD_NAME_ONLY_DETACH ) )
         {
            bob.append( ele ) ;
         }
      }
      newObj = bob.obj() ;

      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
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

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_GROUPNAME, this->getName() ) ;
         queryBuilder.appendElementsUnique( options ) ;
         query = queryBuilder.obj() ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _connection->_runCommand( (CMD_ADMIN_PREFIX CMD_NAME_REELECT),
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
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 )
   {
      ossMemset ( _collectionSpaceName, 0, sizeof ( _collectionSpaceName ) ) ;
   }

   _sdbCollectionSpaceImpl::_sdbCollectionSpaceImpl (
         CHAR *pCollectionSpaceName ) :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 )
   {
      _setName ( pCollectionSpaceName ) ;
   }

   _sdbCollectionSpaceImpl::~_sdbCollectionSpaceImpl ()
   {
      if ( _connection )
      {
         _connection->_unregCollectionSpace ( this ) ;
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

   void _sdbCollectionSpaceImpl::_setConnection ( _sdb *connection )
   {
      _connection = (_sdbImpl*)connection ;
      _connection->_regCollectionSpace ( this ) ;
   }

   INT32 _sdbCollectionSpaceImpl::getCollection ( const CHAR *pCollectionName,
                                                  _sdbCollection **collection )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      BSONObj newObj ;
      string collectionS = string ("") ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION ) ;
      if ( !pCollectionName || ossStrlen ( pCollectionName ) >
                               CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' || !_connection || !collection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      collectionS = string (_collectionSpaceName) + "." +
                    string (pCollectionName) ;
      if ( fetchCachedObject( _connection->_getCachedContainer(),
                              collectionS.c_str() ) )
      {
      }
      else
      {
         newObj = BSON ( FIELD_NAME_NAME << collectionS.c_str() ) ;

         rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
         if ( rc )
         {
            goto error ;
         }
         if ( NULL != (*collection) )
         {
            delete *collection ;
            *collection = NULL ;
         }
         rc = insertCachedObject( _connection->_getCachedContainer(),
                                  collectionS.c_str() ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
      *collection = (_sdbCollection*)( new(std::nothrow) sdbCollectionImpl () ) ;
      if ( !*collection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionImpl*)*collection)->_setConnection ( _connection ) ;
      ((sdbCollectionImpl*)*collection)->_setName ( collectionS.c_str() ) ;

   done :
      return rc ;
error :
      if ( NULL != *collection )
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
      BOOLEAN result      = FALSE ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string collectionS = string ("") ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTION ) ;
      if ( !pCollectionName || ossStrlen ( pCollectionName ) >
                               CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' || !_connection || !collection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      collectionS = string (_collectionSpaceName) + "." +
                    string (pCollectionName) ;
      ob.append ( FIELD_NAME_NAME, collectionS ) ;
      {
         BSONObjIterator it ( options ) ;
         while ( it.more() )
         {
            ob.append ( it.next() ) ;
         }
      }
      newObj = ob.obj () ;

      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
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
      ((sdbCollectionImpl*)*collection)->_setConnection ( _connection ) ;
      ((sdbCollectionImpl*)*collection)->_setName ( collectionS.c_str() ) ;

      rc = insertCachedObject( _connection->_getCachedContainer(),
                               collectionS.c_str() ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      if ( NULL != *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }

      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::dropCollection ( const CHAR *pCollectionName )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      BSONObj newObj ;
      string collectionS = string ("") ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTION ) ;
      if ( !pCollectionName || ossStrlen ( pCollectionName ) >
                               CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( _collectionSpaceName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      collectionS = string (_collectionSpaceName) + "." +
                    string (pCollectionName) ;
      newObj = BSON ( FIELD_NAME_NAME << collectionS ) ;

      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = removeCachedObject( _connection->_getCachedContainer(),
                               collectionS.c_str(), FALSE ) ;
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
      BOOLEAN result      = FALSE ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_COLLECTIONSPACE ) ;
      if ( _collectionSpaceName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << _collectionSpaceName ) ;

      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = insertCachedObject( _connection->_getCachedContainer(),
                               _collectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbCollectionSpaceImpl::drop ()
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_DROP_COLLECTIONSPACE ) ;
      if ( _collectionSpaceName [0] == '\0' || !_connection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << _collectionSpaceName ) ;
      rc = _connection->_runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = removeCachedObject( _connection->_getCachedContainer(),
                               _collectionSpaceName, TRUE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
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

      if( NULL == oldName || NULL == newName )
      {
         rc = SDB_INVALIDARG ;
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
      rc = _connection->_runCommand( (CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTION),
                                     &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      {
         string oldFullName = string( this->getCSName() ) + "."
                              + string( oldName ) ;
         rc = removeCachedObject( _connection->_getCachedContainer(),
                                  oldFullName.c_str(), FALSE ) ;
         if( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   /*
    * sdbDomainImpl
    * SequoiaDB Domain Implementation
    */
   _sdbDomainImpl::_sdbDomainImpl () :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ) ,
   _pReceiveBuffer ( NULL ) ,
   _receiveBufferSize ( 0 )
   {
      ossMemset( _domainName, 0, sizeof ( _domainName ) ) ;
   }

   _sdbDomainImpl::_sdbDomainImpl ( const CHAR *pDomainName ) :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ) ,
   _pReceiveBuffer ( NULL ) ,
   _receiveBufferSize ( 0 )
   {
      _setName( pDomainName ) ;
   }

   _sdbDomainImpl::~_sdbDomainImpl ()
   {
      if ( _connection )
      {
         _connection->_unregDomain ( this ) ;
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

   void _sdbDomainImpl::_setConnection ( _sdb *connection )
   {
      _connection = (_sdbImpl*)connection ;
      _connection->_regDomain ( this ) ;
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
      INT32 rc       = SDB_OK ;
      BOOLEAN result = FALSE ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_ALTER_DOMAIN ) ;
      if ( !_connection || '\0' == _domainName[0] )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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
      rc = _connection->_runCommand(command.c_str(), result, &newObj ) ;
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

      if ( !_connection || '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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
      rc = _connection->getList( cursor, SDB_LIST_CS_IN_DOMAIN, condition, selector ) ;
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

      if ( !_connection || '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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
      rc = _connection->getList( cursor, SDB_LIST_CL_IN_DOMAIN, condition, selector ) ;
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
      if ( !_connection || '\0' == _domainName[0] || !cursor )
      {
         rc = SDB_INVALIDARG ;
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

   /*
    * sdbDataCenterImpl
    * SequoiaDB Data Center Implementation
    */
   _sdbDataCenterImpl::_sdbDataCenterImpl () :
   _connection ( NULL ),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ) ,
   _pReceiveBuffer ( NULL ) ,
   _receiveBufferSize ( 0 )
   {
      ossMemset( _dcName, 0, sizeof ( _dcName ) ) ;
   }

   _sdbDataCenterImpl::~_sdbDataCenterImpl ()
   {
      if ( _connection )
      {
         _connection->_unregDataCenter( this ) ;
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

   void _sdbDataCenterImpl::_setConnection ( _sdb *connection )
   {
      _connection = (_sdbImpl*)connection ;
      _connection->_regDataCenter( this ) ;
   }

   INT32 _sdbDataCenterImpl::_DCCommon( const CHAR *pValue, const bson::BSONObj *pInfo )
   {
      INT32 rc            = SDB_OK ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_ALTER_DC ;
      BSONObjBuilder bob ;
      BSONObj newObj ;

      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( NULL == pValue )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      bob.append( FIELD_NAME_ACTION, pValue ) ;
      if ( NULL != pInfo )
      {
         bob.append( FIELD_NAME_OPTIONS, *pInfo ) ;
      }
      newObj = bob.obj() ;

      rc = _connection->_runCommand( pCommand, &newObj, NULL, NULL, NULL,
                                     0, 0, -1, -1, NULL ) ;
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

      if ( NULL == _connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }

      rc = _connection->_runCommand( pCommand, NULL, NULL, NULL, NULL,
                                     0, 0, -1, -1, &retInfoCursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if ( NULL == retInfoCursor )
      {
         rc = SDB_SYS ;
         goto error ;
      }

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
      INT32 rc = _DCCommon( CMD_VALUE_NAME_ACTIVATE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::deactivateDC()
   {
      INT32 rc = _DCCommon( CMD_VALUE_NAME_DEACTIVATE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::enableReadOnly( BOOLEAN isReadOnly )
   {
      INT32 rc = SDB_OK ;
      if ( TRUE == isReadOnly )
      {
         rc = _DCCommon( CMD_VALUE_NAME_ENABLE_READONLY, NULL ) ;
      }
      else
      {
         rc = _DCCommon( CMD_VALUE_NAME_DISABLE_READONLY, NULL ) ;
      }
      return rc ;
   }

   INT32 _sdbDataCenterImpl::createImage( const CHAR *pCataAddrList )
   {
      INT32 rc = SDB_OK ;
      BSONObj newObj ;

      if ( NULL == pCataAddrList )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      newObj = BSON( FIELD_NAME_ADDRESS << pCataAddrList ) ;

      rc = _DCCommon( CMD_VALUE_NAME_CREATE, &newObj ) ;
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
      INT32 rc = _DCCommon( CMD_VALUE_NAME_REMOVE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::enableImage()
   {
      INT32 rc = _DCCommon( CMD_VALUE_NAME_ENABLE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::disableImage()
   {
      INT32 rc = _DCCommon( CMD_VALUE_NAME_DISABLE, NULL ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::attachGroups( const bson::BSONObj &info )
   {
      INT32 rc = _DCCommon( CMD_VALUE_NAME_ATTACH, &info ) ;
      return rc ;
   }

   INT32 _sdbDataCenterImpl::detachGroups( const bson::BSONObj &info )
   {
      INT32 rc = _DCCommon( CMD_VALUE_NAME_DETACH, &info ) ;
      return rc ;
   }


   /*
    * sdbLobImpl
    * SequoiaDB large object Implementation
    */
   _sdbLobImpl::_sdbLobImpl () :
   _connection (NULL),
   _collection (NULL),
   _pSendBuffer ( NULL ),
   _sendBufferSize ( 0 ),
   _pReceiveBuffer ( NULL ),
   _receiveBufferSize ( 0 ),
   _isOpen( FALSE ),
   _contextID ( -1 ),
   _mode( -1 ),
   _seekWrite( FALSE ),
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
         _detachConnection() ;
      }
      if ( _collection )
      {
         _detachCollection() ;
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

   void _sdbLobImpl::_attachConnection ( _sdb *connection )
   {
      _connection = (_sdbImpl*)connection ;
      if ( _connection )
      {
         _connection->_regLob ( this ) ;
      }
   }

   void _sdbLobImpl::_attachCollection ( _sdbCollectionImpl *collection )
   {
      _collection = collection ;
   }

   void _sdbLobImpl::_detachConnection()
   {
      if ( NULL != _connection )
      {
         _connection->_unregLob( this ) ;
         _connection = NULL ;
      }
   }

   void _sdbLobImpl::_detachCollection()
   {
      _collection = NULL ;
   }

   void _sdbLobImpl::_close()
   {
      _isOpen = FALSE ;
      _collection = NULL ;
      _contextID = -1 ;
      _mode = -1 ;
      _currentOffset = 0 ;
      _cachedOffset = 0 ;
      _cachedSize = 0 ;
      _dataCache = NULL ;
   }

   BOOLEAN _sdbLobImpl::_dataCached()
   {
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
      BOOLEAN result           = TRUE ;

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
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
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
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      const MsgOpReply* reply = NULL ;

      if ( !_isOpen || -1 == _contextID )
      {
         goto done ;
      }
      if ( !_connection )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error;
      }
      rc = clientBuildCloseLobMsg( &_pSendBuffer, &_sendBufferSize,
                                   0, 1, _contextID, 0,
                                   _connection->_endianConvert ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      _connection->lock() ;
      locked = TRUE ;
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
      _close() ;

      reply = ( const MsgOpReply * )( _pReceiveBuffer ) ;
      if ( reply->numReturned > 0 &&
           (UINT32)reply->header.messageLength > ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
      {
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
         catch ( std::exception &e )
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
         if ( NULL != _connection )
         {
            _detachConnection() ;
         }
         if ( NULL != _collection )
         {
            _detachCollection() ;
         }
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
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( SDB_LOB_READ != _mode || -1 == _contextID )
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
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;
      UINT32 totalLen = 0 ;
      const UINT32 maxSendLen = 2 * 1024 * 1024 ;

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
      if ( NULL == buf )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_LOB_CREATEONLY != _mode && SDB_LOB_WRITE != _mode )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 == len )
      {
         goto done ;
      }
      do
      {
         INT64 offset = _seekWrite ? _currentOffset : -1 ;
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
         rc = _connection->_send ( _pSendBuffer ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                          contextID, result ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, _connection ) ;
         locked = FALSE ;
         _connection->unlock() ;

         totalLen += sendLen ;

         _currentOffset += sendLen ;
         _lobSize = OSS_MAX( _lobSize, _currentOffset ) ;
         _seekWrite = FALSE ;
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
      if ( SDB_LOB_READ != _mode &&
           SDB_LOB_CREATEONLY != _mode &&
           SDB_LOB_WRITE != _mode )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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

      if ( SDB_LOB_CREATEONLY == _mode || SDB_LOB_WRITE == _mode )
      {
         _seekWrite = TRUE ;
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
      BOOLEAN result = FALSE ;

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

      if ( SDB_LOB_WRITE != _mode )
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
      rc = _connection->_send ( _pSendBuffer ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
      rc = _connection->_recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                                       contextID, result ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
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
   _attributeCache ()
   {
      initHashTable( &_tb ) ;
      ossGetCurrentTime(_lastAliveTime);
   }

   _sdbImpl::~_sdbImpl ()
   {
      std::set<ossValuePtr> copySet ;
      std::set<ossValuePtr>::iterator it ;

      copySet = _cursors ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbCursorImpl*)(*it))->_detachConnection () ;
      }
      copySet = _collections ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbCollectionImpl*)(*it))->_dropConnection () ;
      }
      copySet = _collectionspaces ;
      for ( it = copySet.begin(); it != copySet.end(); ++it)
      {
         ((_sdbCollectionSpaceImpl*)(*it))->_dropConnection () ;
      }
      copySet = _nodes ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbNodeImpl*)(*it))->_dropConnection () ;
      }
      copySet = _replicaGroups ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbReplicaGroupImpl*)(*it))->_dropConnection () ;
      }
      copySet = _domains ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbDomainImpl*)(*it))->_dropConnection () ;
      }
      copySet = _dataCenters ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbDataCenterImpl*)(*it))->_dropConnection () ;
      }
      copySet = _lobs ;
      for ( it = copySet.begin(); it != copySet.end(); ++it )
      {
         ((_sdbLobImpl*)(*it))->_detachConnection () ;
      }
      if ( NULL != _tb )
      {
         releaseHashTable( &_tb ) ;
      }
      if ( _sock )
         _disconnect () ;
      if ( _pSendBuffer )
         SDB_OSS_FREE ( _pSendBuffer ) ;
      if ( _pReceiveBuffer )
         SDB_OSS_FREE ( _pReceiveBuffer ) ;
   }

   void _sdbImpl::_disconnect ()
   {
      INT32 rc = SDB_OK ;
      CHAR buffer [ sizeof ( MsgOpDisconnect ) ] ;
      CHAR *pBuffer = &buffer[0] ;
      INT32 bufferSize = sizeof ( buffer ) ;
      if ( _sock )
      {
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

   void _sdbImpl::_regCursor ( _sdbCursorImpl *cursor )
   {
      lock () ;
      _cursors.insert ( (ossValuePtr)cursor ) ;
      unlock () ;
   }

   void _sdbImpl::_regCollection ( _sdbCollectionImpl *collection )
   {
      lock () ;
      _collections.insert ( (ossValuePtr)collection ) ;
      unlock () ;
   }

   void _sdbImpl::_regCollectionSpace ( _sdbCollectionSpaceImpl *collectionspace )
   {
      lock () ;
      _collectionspaces.insert ( (ossValuePtr)collectionspace ) ;
      unlock () ;
   }

   void _sdbImpl::_regNode ( _sdbNodeImpl *node )
   {
      lock () ;
      _nodes.insert ( (ossValuePtr)node ) ;
      unlock () ;
   }

   void _sdbImpl::_regReplicaGroup ( _sdbReplicaGroupImpl *replicaGroup )
   {
      lock () ;
      _replicaGroups.insert ( (ossValuePtr)replicaGroup ) ;
      unlock () ;
   }
   void _sdbImpl::_regDomain ( _sdbDomainImpl *domain )
   {
      lock () ;
      _domains.insert ( (ossValuePtr)domain ) ;
      unlock () ;
   }

   void _sdbImpl::_regDataCenter ( _sdbDataCenterImpl *dc )
   {
      lock () ;
      _dataCenters.insert ( (ossValuePtr)dc ) ;
      unlock () ;
   }

   void _sdbImpl::_regLob ( _sdbLobImpl *lob )
   {
      lock () ;
      _lobs.insert ( (ossValuePtr)lob ) ;
      unlock () ;
   }

   void _sdbImpl::_unregCursor ( _sdbCursorImpl *cursor )
   {
      lock () ;
      _cursors.erase ( (ossValuePtr)cursor ) ;
      unlock () ;
   }

   void _sdbImpl::_unregCollection ( _sdbCollectionImpl *collection )
   {
      lock () ;
      _collections.erase ( (ossValuePtr)collection ) ;
      unlock () ;
   }

   void _sdbImpl::_unregCollectionSpace ( _sdbCollectionSpaceImpl *collectionspace )
   {
      lock () ;
      _collectionspaces.erase ( (ossValuePtr)collectionspace ) ;
      unlock () ;
   }

   void _sdbImpl::_unregNode ( _sdbNodeImpl *node )
   {
      lock () ;
      _nodes.erase ( (ossValuePtr)node ) ;
      unlock () ;
   }

   void _sdbImpl::_unregReplicaGroup ( _sdbReplicaGroupImpl *replicaGroup )
   {
      lock () ;
      _replicaGroups.erase ( (ossValuePtr)replicaGroup ) ;
      unlock () ;
   }

   void _sdbImpl::_unregDomain ( _sdbDomainImpl *domain )
   {
      lock () ;
      _domains.erase ( (ossValuePtr)domain ) ;
      unlock () ;
   }

   void _sdbImpl::_unregDataCenter ( _sdbDataCenterImpl *dc )
   {
      lock () ;
      _dataCenters.erase ( (ossValuePtr)dc ) ;
      unlock () ;
   }

   void _sdbImpl::_unregLob ( _sdbLobImpl *lob )
   {
      lock () ;
      _lobs.erase ( (ossValuePtr)lob ) ;
      unlock () ;
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
      rc = clientExtractSysInfoReply ( (CHAR*)pReply, &_endianConvert, NULL ) ;
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
      BOOLEAN locked = FALSE ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;

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

      rc = md5Encrypt( pPasswd, md5, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      rc = clientBuildAuthMsg( &_pSendBuffer, &_sendBufferSize,
                               pUsrName, md5, 0, _endianConvert ) ;
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
                          contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
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
      i = _sdbRand() % arrSize ;
      mark = i ;

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
            continue ;
         }
         *pTmp = 0 ;
         pHostName = pStr ;
         pServiceName = pTmp + 1 ;
         rc = connect ( pHostName, pServiceName, pUsrName, pPasswd ) ;
         SDB_OSS_FREE ( pStr ) ;
         pStr = NULL ;
         pTmp = NULL ;
         if ( rc == SDB_OK)
            goto done ;
      } while ( mark != i ) ;
      rc = SDB_NET_CANNOT_CONNECT ;
   done :
      return rc ;
   error :
      goto done ;

   }

   INT32 _sdbImpl::createUsr( const CHAR *pUsrName,
                              const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;

      if ( NULL == pUsrName ||
           NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = md5Encrypt( pPasswd, md5, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      rc = clientBuildAuthCrtMsg( &_pSendBuffer, &_sendBufferSize,
                               pUsrName, md5, 0,
                               _endianConvert ) ;
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
                          contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::removeUsr( const CHAR *pUsrName,
                              const CHAR *pPasswd )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      CHAR md5[SDB_MD5_DIGEST_LENGTH*2+1] ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;

      if ( NULL == pUsrName ||
           NULL == pPasswd )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = md5Encrypt( pPasswd, md5, SDB_MD5_DIGEST_LENGTH*2+1) ;
      if ( rc )
      {
         goto error ;
      }

      rc = clientBuildAuthDelMsg( &_pSendBuffer, &_sendBufferSize,
                                  pUsrName, md5, 0,
                                  _endianConvert ) ;
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
                          contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::getSnapshot ( _sdbCursor **result,
                                 INT32 snapType,
                                 const BSONObj &condition,
                                 const BSONObj &selector,
                                 const BSONObj &orderBy
                               )
   {
      INT32 rc                        = SDB_OK ;
      const CHAR *p                   = NULL ;
      SINT64 contextID                = 0 ;
      BOOLEAN r                       = FALSE ;
      if ( !result )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
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
      case SDB_SNAP_CONFIG :
         p = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_CONFIG ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      lock () ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                    p, 0, 0, 0, -1,
                                    condition.objdata(),
                                    selector.objdata(),
                                    orderBy.objdata(),
                                    NULL, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
      if ( *result )
      {
         delete *result ;
         *result = NULL ;
      }
      *result = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl () ) ;
      if ( !*result )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*result)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)*result)->_contextID = contextID ;
   exit :
      return rc ;
   done :
      unlock () ;
      goto exit ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::resetSnapshot ( const BSONObj &options )
   {
      INT32 rc                = SDB_OK ;
      BOOLEAN r               = FALSE ;
      const CHAR *p           = CMD_ADMIN_PREFIX CMD_NAME_SNAPSHOT_RESET ;
      if ( !_sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      lock () ;
      rc = _runCommand ( p, r, &options,
                         NULL, NULL, NULL ) ;
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

   INT32 _sdbImpl::getList ( _sdbCursor **result,
                             INT32 listType,
                             const BSONObj &condition,
                             const BSONObj &selector,
                             const BSONObj &orderBy
                           )
   {
      INT32 rc                        = SDB_OK ;
      const CHAR *p                   = NULL ;
      SINT64 contextID                = 0 ;
      BOOLEAN r                       = FALSE ;
      if ( !result )
      {
         rc = SDB_INVALIDARG ;
         goto exit ;
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
      case SDB_LIST_CL_IN_DOMAIN :
         p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CL_IN_DOMAIN ;
         break ;
      case SDB_LIST_CS_IN_DOMAIN :
         p = CMD_ADMIN_PREFIX CMD_NAME_LIST_CS_IN_DOMAIN ;
         break ;
      default :
         rc = SDB_INVALIDARG ;
         goto exit ;
      }
      lock () ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer,
                                    &_sendBufferSize,
                                    p, 0, 0, 0, -1,
                                    condition.objdata(),
                                    selector.objdata(),
                                    orderBy.objdata(),
                                    NULL, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer,
                          &_receiveBufferSize,
                          contextID,
                          r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
      if ( *result )
      {
         delete *result ;
         *result = NULL ;
      }
      *result = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl () ) ;
      if ( !*result )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*result)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)*result)->_contextID = contextID ;
   exit:
      return rc ;
   done :
      unlock () ;
      goto exit ;
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

   INT32 _sdbImpl::_send ( CHAR *pBuffer )
   {
      INT32 rc = SDB_OK ;
      INT32 len = 0 ;
      if ( !isConnected() )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      ossEndianConvertIf4 ( *(SINT32*)pBuffer, len, _endianConvert ) ;
      rc = clientSocketSend ( _sock, pBuffer, len ) ;
      if ( rc )
      {
         goto error ;
      }
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
      if ( !isConnected () )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      rc = clientSocketRecv ( _sock,
                              (CHAR*)&length,
                              sizeof(length) ) ;
      if ( rc )
      {
         goto error ;
      }
      _sock->quickAck() ;

      ossEndianConvertIf4 (length, realLen, _endianConvert ) ;
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

      ossGetCurrentTime(_lastAliveTime);
   done :
      return rc ;
   error :
      if ( SDB_NETWORK_CLOSE == rc )
      {
         delete (_sock) ;
         _sock = NULL ;
      }
      goto done ;
   }

   INT32 _sdbImpl::_recvExtract ( CHAR **ppBuffer, INT32 *size,
                                  SINT64 &contextID,
                                  BOOLEAN &result )
   {
      INT32 rc          = SDB_OK ;
      INT32 replyFlag   = -1 ;
      INT32 numReturned = -1 ;
      INT32 startFrom   = -1 ;
      rc = _recv ( ppBuffer, size ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = clientExtractReply ( *ppBuffer, &replyFlag, &contextID,
                                &startFrom, &numReturned, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( replyFlag != SDB_OK )
      {
         result = FALSE ;
         rc = replyFlag ;
         goto done ;
      }

      result = TRUE ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_getRetInfo ( CHAR **ppBuffer, INT32 *size,
                                 SINT64 contextID,
                                 _sdbCursor **ppCursor )
   {
      INT32 rc           = SDB_OK ;
      _sdbCursor *cursor = NULL ;

      if ( -1 == contextID &&
           ( ((UINT32)((MsgHeader*)*ppBuffer)->messageLength) <=
              ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) ) )
      {
         if ( NULL != ppCursor && NULL != *ppCursor )
         {
            *ppCursor = NULL ;
         }
         goto done ;
      }

      if ( NULL == ppCursor )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      cursor = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl() ) ;
      if ( NULL == cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)cursor)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)cursor)->_contextID = contextID ;

      if ( NULL != ppBuffer )
      {
         if ( ((UINT32)((MsgHeader*)*ppBuffer)->messageLength) >
              ossRoundUpToMultipleX( sizeof(MsgOpReply), 4 ) )
         {
            ((_sdbCursorImpl*)cursor)->_pReceiveBuffer = *ppBuffer ;
            *ppBuffer = NULL ;
            ((_sdbCursorImpl*)cursor)->_receiveBufferSize = *size ;
            *size = 0 ;
         }
      }

      *ppCursor = cursor ;

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_runCommand ( const CHAR *pString, BOOLEAN &result,
                                 const BSONObj *arg1,
                                 const BSONObj *arg2,
                                 const BSONObj *arg3,
                                 const BSONObj *arg4 )
   {
      INT32 rc            = SDB_OK ;
      SINT64 contextID    = 0 ;
      lock () ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, pString,
                                    0, 0, -1, -1,
                                    arg1?arg1->objdata():NULL,
                                    arg2?arg2->objdata():NULL,
                                    arg3?arg3->objdata():NULL,
                                    arg4?arg4->objdata():NULL,
                                    _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done :
      unlock () ;
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
      BOOLEAN result      = FALSE ;
      SINT64 contextID    = -1 ;
      lock () ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize, pString,
                                    flag, reqID, numToSkip, numToReturn,
                                    arg1?arg1->objdata():NULL,
                                    arg2?arg2->objdata():NULL,
                                    arg3?arg3->objdata():NULL,
                                    arg4?arg4->objdata():NULL,
                                    _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( SDB_OK != rc )
      {
         if ( FALSE == result ) // error happened in driver
         {
            goto error ;
         }
         else // error happened in engine
         {
            goto error ;
         }
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

      if ( NULL != ppCursor )
      {
         rc = _getRetInfo( &_pReceiveBuffer, &_receiveBufferSize,
                           contextID, ppCursor ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done :
      unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::_buildEmptyCursor( _sdbCursor **ppCursor )
   {
      INT32 rc           = SDB_OK ;
      _sdbCursor *cursor = NULL ;

      cursor = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl() ) ;
      if ( NULL == cursor )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)cursor)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)cursor)->_contextID = -1 ;

      *ppCursor = cursor ;

   done:
      return rc ;
   error:
      goto done ;
   }


   INT32 _sdbImpl::getCollection ( const CHAR *pCollectionFullName,
                                   _sdbCollection **collection )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      INT32 nameLength    = ossStrlen ( pCollectionFullName ) ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_TEST_COLLECTION ) ;
      if ( !_sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( !pCollectionFullName || nameLength > CLIENT_CS_NAMESZ +
           CLIENT_COLLECTION_NAMESZ + 1 || !collection )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( fetchCachedObject( _tb, pCollectionFullName ) )
      {
      }
      else
      {
         newObj = BSON ( FIELD_NAME_NAME << pCollectionFullName ) ;
         rc = _runCommand ( command.c_str(), result, &newObj ) ;
         if ( rc )
         {
            goto error ;
         }

         rc = insertCachedObject( _tb, pCollectionFullName ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

      *collection = (_sdbCollection*)( new(std::nothrow) sdbCollectionImpl ()) ;
      if ( !*collection )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionImpl*)*collection)->_setConnection ( this ) ;
      ((sdbCollectionImpl*)*collection)->_setName ( pCollectionFullName ) ;

   done :
      return rc ;
   error :
      if ( NULL != *collection )
      {
         delete *collection ;
         *collection = NULL ;
      }

      goto done ;
   }

   INT32 _sdbImpl::getCollectionSpace ( const CHAR *pCollectionSpaceName,
                                        _sdbCollectionSpace **cs )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      INT32 nameLength = ossStrlen ( pCollectionSpaceName ) ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX
                                CMD_NAME_TEST_COLLECTIONSPACE ) ;
      if ( !_sock )
      {
         rc = SDB_NOT_CONNECTED ;
         goto error ;
      }
      if ( nameLength > CLIENT_CS_NAMESZ || !cs )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      if ( fetchCachedObject( _tb, pCollectionSpaceName ) )
      {
      }
      else
      {
         newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName ) ;
         rc = _runCommand ( command.c_str(), result, &newObj ) ;
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

      *cs = (_sdbCollectionSpace*)( new(std::nothrow) sdbCollectionSpaceImpl());
      if ( !*cs )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;

   done :
      return rc ;
   error :
      if ( NULL != *cs )
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
      BOOLEAN result      = FALSE ;
      INT32 nameLength = ossStrlen ( pCollectionSpaceName ) ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX
                                CMD_NAME_CREATE_COLLECTIONSPACE ) ;
      if ( nameLength > CLIENT_CS_NAMESZ || !cs )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName <<
                      FIELD_NAME_PAGE_SIZE << iPageSize ) ;
      rc = _runCommand ( command.c_str(), result, &newObj ) ;
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
      ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;

      rc = insertCachedObject( _tb, pCollectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
error :
      if ( NULL != *cs )
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
      BOOLEAN result      = FALSE ;
      INT32 nameLength = ossStrlen ( pCollectionSpaceName ) ;
      BSONObjBuilder bob ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX
                                CMD_NAME_CREATE_COLLECTIONSPACE ) ;
      if ( nameLength > CLIENT_CS_NAMESZ || !cs )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }


      try
      {
         bob.append ( FIELD_NAME_NAME, pCollectionSpaceName ) ;
         BSONObjIterator it( options ) ;
         while ( it.more() )
         {
            bob.append ( it.next() ) ;
         }
         newObj = bob.obj () ;
      }
      catch ( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
/*
      newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName <<
                      FIELD_NAME_PAGE_SIZE << iPageSize ) ;
*/
      rc = _runCommand ( command.c_str(), result, &newObj ) ;
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
      ((sdbCollectionSpaceImpl*)*cs)->_setConnection ( this ) ;
      ((sdbCollectionSpaceImpl*)*cs)->_setName ( pCollectionSpaceName ) ;

      rc = insertCachedObject( _tb, pCollectionSpaceName ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }


   INT32 _sdbImpl::dropCollectionSpace ( const CHAR *pCollectionSpaceName )
   {
      INT32 rc            = SDB_OK ;
      BOOLEAN result      = FALSE ;
      INT32 nameLength = ossStrlen ( pCollectionSpaceName ) ;
      BSONObj newObj ;
      string command = string ( CMD_ADMIN_PREFIX
                                CMD_NAME_DROP_COLLECTIONSPACE ) ;
      if ( nameLength > CLIENT_CS_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      newObj = BSON ( FIELD_NAME_NAME << pCollectionSpaceName ) ;
      rc = _runCommand ( command.c_str(), result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }

      rc = removeCachedObject( _tb, pCollectionSpaceName, TRUE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

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

   INT32 _sdbImpl::listReplicaGroups ( _sdbCursor **result )
   {
      return getList ( result, SDB_LIST_GROUPS ) ;
   }

   INT32 _sdbImpl::getReplicaGroup ( const CHAR *pName, _sdbReplicaGroup **result )
   {
      INT32 rc = SDB_OK ;
      sdbCursor resultCursor ;
      BOOLEAN found = FALSE ;
      BSONObj record ;
      BSONObj condition ;
      BSONElement ele ;
      if ( !pName || !result )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      condition = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = getList ( &resultCursor.pCursor, SDB_LIST_GROUPS, condition ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( SDB_OK == ( rc = resultCursor.next ( record ) ) )
      {
         _sdbReplicaGroupImpl *replset =
                     new(std::nothrow) _sdbReplicaGroupImpl () ;
         if ( !replset )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ele = record.getField ( CAT_GROUPID_NAME ) ;
         if ( ele.type() == NumberInt )
         {
            replset->_replicaGroupID = ele.numberInt() ;
         }
         replset->_connection = this ;
         _regReplicaGroup ( replset ) ;
         ossStrncpy ( replset->_replicaGroupName, pName, CLIENT_REPLICAGROUP_NAMESZ ) ;
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
      goto done ;
   }

   INT32 _sdbImpl::getReplicaGroup ( SINT32 id, _sdbReplicaGroup **result )
   {
      INT32 rc = SDB_OK ;
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
            _sdbReplicaGroupImpl *replset =
                  new(std::nothrow) _sdbReplicaGroupImpl () ;
            if ( !replset )
            {
               rc = SDB_OOM ;
               goto error ;
            }
            replset->_connection = this ;
            _regReplicaGroup ( replset ) ;
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
      goto done ;
   }

   INT32 _sdbImpl::createReplicaGroup ( const CHAR *pName,
                                        _sdbReplicaGroup **rg )
   {
      INT32 rc = SDB_OK ;
      BSONObj replicaGroupName ;
      BOOLEAN result = FALSE ;
      _sdbReplicaGroupImpl *replset = NULL ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_GROUP ) ;

      if ( ossStrlen ( pName ) > CLIENT_REPLICAGROUP_NAMESZ || !rg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = _runCommand ( command.c_str(), result, &replicaGroupName ) ;
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
      replset->_connection = this ;
      _regReplicaGroup ( replset ) ;
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
      goto done ;
   }

   INT32 _sdbImpl::removeReplicaGroup ( const CHAR *pReplicaGroupName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN result = FALSE ;
      INT32 nameLength = 0 ;
      BSONObjBuilder ob ;
      BSONObj newObj ;
      const CHAR *pCommand = CMD_ADMIN_PREFIX CMD_NAME_REMOVE_GROUP ;
      const CHAR *pName = FIELD_NAME_GROUPNAME ;
      if ( !pReplicaGroupName )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      nameLength = ossStrlen( pReplicaGroupName ) ;
      if ( 0 == nameLength || CLIENT_REPLICAGROUP_NAMESZ < nameLength )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      ob.append ( pName, pReplicaGroupName ) ;
      newObj = ob.obj() ;
      rc = _runCommand ( pCommand, result, &newObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::createReplicaCataGroup (  const CHAR *pHostName,
                                        const CHAR *pServiceName,
                                        const CHAR *pDatabasePath,
                                        const BSONObj &configure )
   {
      INT32 rc = SDB_OK ;
      BSONObj configuration ;
      BSONObjBuilder ob ;
      BOOLEAN result = FALSE ;
      const CHAR *pCreateCataRG = CMD_ADMIN_PREFIX CMD_NAME_CREATE_CATA_GROUP ;

      if ( !pHostName || !pServiceName || !pDatabasePath )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ob.append ( CAT_HOST_FIELD_NAME, pHostName ) ;

      ob.append ( PMD_OPTION_SVCNAME, pServiceName ) ;

      ob.append ( PMD_OPTION_DBPATH, pDatabasePath ) ;

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
               continue ;
            }
            ob.append ( ele ) ;
         } // while
      } // if ( configure )
      configuration = ob.obj () ;

      rc = _runCommand ( pCreateCataRG, result, &configuration ) ;
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
      BOOLEAN result = FALSE ;
      _sdbReplicaGroupImpl *replset = NULL ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_ACTIVE_GROUP ) ;

      if ( ossStrlen ( pName ) > CLIENT_REPLICAGROUP_NAMESZ || !rg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      replicaGroupName = BSON ( CAT_GROUPNAME_NAME << pName ) ;
      rc = _runCommand ( command.c_str(), result, &replicaGroupName ) ;
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
      replset->_connection = this ;
      _regReplicaGroup ( replset ) ;
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
      goto done ;
   }

   INT32 _sdbImpl::execUpdate( const CHAR *sql )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;

      rc = clientValidateSql( sql, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = clientBuildSqlMsg( &_pSendBuffer, &_sendBufferSize,
                              sql, 0, _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::exec( const CHAR *sql,
                         _sdbCursor **result )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;

      rc = clientValidateSql( sql, TRUE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      rc = clientBuildSqlMsg( &_pSendBuffer, &_sendBufferSize,
                              sql, 0,
                              _endianConvert ) ;
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
                           contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
      if ( *result )
      {
         delete *result ;
         *result = NULL ;
      }
      *result = (_sdbCursor*)( new(std::nothrow) sdbCursorImpl () ) ;
      if ( !*result )
      {
         rc = SDB_OOM ;
         goto error ;
      }
      ((_sdbCursorImpl*)*result)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)*result)->_contextID = contextID ;
   done :
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl:: transactionBegin()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      rc = clientBuildTransactionBegMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                         _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl:: transactionCommit()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      rc = clientBuildTransactionCommitMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                            _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl:: transactionRollback()
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      rc = clientBuildTransactionRollbackMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                              _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::flushConfigure( const bson::BSONObj &options )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;
      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                (CMD_ADMIN_PREFIX CMD_NAME_EXPORT_CONFIG),
                                0, 0, 0, -1, options.objdata(), NULL, NULL, NULL,
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
                                       contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done:
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::crtJSProcedure ( const CHAR *code )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;
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

      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                   (CMD_ADMIN_PREFIX CMD_NAME_CRT_PROCEDURE),
                                   0, 0, 0, -1, newObj.objdata(), NULL, NULL, NULL,
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
                                          contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done:
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::rmProcedure( const CHAR *spName )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN r ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;

      ob.append ( FIELD_NAME_FUNC, spName ) ;
      newObj = ob.obj() ;

      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                (CMD_ADMIN_PREFIX CMD_NAME_RM_PROCEDURE),
                                0, 0, 0, -1, newObj.objdata(), NULL, NULL, NULL,
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
                           contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done:
      if ( locked )
      {
         unlock () ;
      }
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::listProcedures( _sdbCursor **cursor, const bson::BSONObj &condition )
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
      BOOLEAN r ;
      SINT64 contextID = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      const MsgOpReply *replyHeader = NULL ;

      ob.appendCode ( FIELD_NAME_FUNC, code ) ;
      ob.appendIntOrLL ( FIELD_NAME_FUNCTYPE, FMP_FUNC_TYPE_JS ) ;
      newObj = ob.obj() ;

      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                (CMD_ADMIN_PREFIX CMD_NAME_EVAL),
                                0, 0, 0, -1, newObj.objdata(), NULL, NULL, NULL,
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
                        contextID, r ) ;
      if ( rc )
      {
         goto error ;
      }
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
      ((_sdbCursorImpl*)*cursor)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;

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
         catch ( std::exception &e )
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
      goto done ;
   }

   INT32 _sdbImpl::backupOffline ( const bson::BSONObj &options)
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN result    = FALSE ;
      SINT64 contextID  = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_BACKUP_OFFLINE ) ;
      {
      BSONObjIterator it ( options ) ;
      while ( it.more() )
      {
         ob.append ( it.next() ) ;
      }
      }
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     command.c_str(), 0, 0, 0, -1,
                                     newObj.objdata(), NULL, NULL, NULL,
                                    _endianConvert ) ;
      if ( rc )
      {
         goto done ;
      }
      lock () ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done :
      return rc ;
   error :
      unlock () ;
      goto done ;
   }

   INT32 _sdbImpl::listBackup ( _sdbCursor **cursor,
                                 const bson::BSONObj &options,
                                 const bson::BSONObj &condition,
                                 const bson::BSONObj &selector,
                              const bson::BSONObj &orderBy)
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN result    = FALSE ;
      BOOLEAN locked    = FALSE ;
      SINT64 contextID  = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_LIST_BACKUPS ) ;
      {
      BSONObjIterator it ( options ) ;
      while ( it.more() )
      {
         ob.append ( it.next() ) ;
      }
      }
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     command.c_str(), 0, 0, 0, -1,
                                     condition.objdata(), selector.objdata(),
                                     orderBy.objdata(), newObj.objdata(),
                                     _endianConvert ) ;
      if ( rc )
      {
         goto done ;
      }
      lock () ;
      locked = TRUE ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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
      ((_sdbCursorImpl*)*cursor)->_attachConnection ( this ) ;
      ((_sdbCursorImpl*)*cursor)->_contextID = contextID ;
   done :
      if ( locked )
          unlock () ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::removeBackup ( const bson::BSONObj &options )
   {
      INT32 rc          = SDB_OK ;
      BOOLEAN result    = FALSE ;
      SINT64 contextID  = 0 ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_REMOVE_BACKUP ) ;
      {
      BSONObjIterator it ( options ) ;
      while ( it.more() )
      {
         ob.append ( it.next() ) ;
      }
      }
      newObj = ob.obj() ;
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     command.c_str(), 0, 0, 0, -1,
                                     newObj.objdata(), NULL, NULL, NULL,
                                     _endianConvert ) ;
      if ( rc )
      {
          goto done ;
      }
      lock () ;
      rc = _send ( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;
   done :
      return rc ;
   error :
      unlock () ;
      goto done ;
   }

   INT32 _sdbImpl::listTasks ( _sdbCursor **cursor,
                           const bson::BSONObj &condition,
                           const bson::BSONObj &selector,
                           const bson::BSONObj &orderBy,
                           const bson::BSONObj &hint)

   {
      return getList ( cursor, SDB_LIST_TASKS, condition, selector, orderBy ) ;
   }

   INT32 _sdbImpl::waitTasks ( const SINT64 *taskIDs,
                         SINT32 num )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObjBuilder bob ;
      BSONObjBuilder subBob ;
      BSONArrayBuilder bab ;
      BSONObj newObj ;
      BSONObj subObj ;
      INT32 i = 0 ;

      if ( !taskIDs || num < 0 )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      try
      {
         for ( i = 0 ; i < num; i++ )
         {
            bab.append(taskIDs[i]) ;
         }
         subBob.appendArray ( "$in", bab.arr() ) ;
         subObj = subBob.obj () ;
         bob.append ( FIELD_NAME_TASKID, subObj ) ;
         newObj = bob.obj () ;
      }
      catch (std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }

      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     CMD_ADMIN_PREFIX CMD_NAME_WAITTASK,
                                     0, 0, 0, -1,
                                     newObj.objdata(), NULL,
                                     NULL, NULL,
                                     _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::cancelTask ( SINT64 taskID,
                            BOOLEAN isAsync )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BOOLEAN result ;
      SINT64 contextID = 0 ;
      BSONObjBuilder it ;
      BSONObj newObj ;

      if ( taskID <= 0 )
      {
         rc = SDB_INVALIDARG ;
       goto error ;
      }

      try
      {
         it.appendIntOrLL ( FIELD_NAME_TASKID, taskID ) ;
         it.appendBool( FIELD_NAME_ASYNC, isAsync ) ;
         newObj = it.obj () ;
      }
      catch (std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = clientBuildQueryMsgCpp ( &_pSendBuffer, &_sendBufferSize,
                                     CMD_ADMIN_PREFIX CMD_NAME_CANCEL_TASK,
                                     0, 0, 0, -1,
                                     newObj.objdata(), NULL,
                                     NULL, NULL,
                                     _endianConvert ) ;
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
                                       contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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
      BOOLEAN result   = FALSE ;
      BOOLEAN locked   = FALSE ;
      SINT64 contextID = 0 ;
      BSONObjBuilder builder ;
      BSONObj newObj ;
      BSONObjIterator it ( options ) ;

      if ( !it.more() )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      while ( it.more() )
      {
         BSONElement ele = it.next() ;
         const CHAR * key = ele.fieldName() ;
         if ( 0 == strcmp( FIELD_NAME_PREFERED_INSTANCE, key ) )
         {
            switch ( ele.type() )
            {
               case String :
               {
                  try
                  {
                     INT32 value = PREFER_REPL_TYPE_MAX ;
                     const CHAR * str_value = ele.valuestrsafe() ;
                     if ( 0 == strcmp( "M", str_value ) ||
                          0 == strcmp( "m", str_value ) )
                     {
                        value = PREFER_REPL_MASTER ;
                     }
                     else if ( 0 == strcmp( "S", str_value ) ||
                               0 == strcmp( "s", str_value ) )
                     {
                        value = PREFER_REPL_SLAVE ;
                     }
                     else if ( 0 == strcmp( "A", str_value ) ||
                               0 == strcmp( "a", str_value ) )
                     {
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
                     rc = SDB_SYS ;
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

      rc = clientBuildQueryMsgCpp( &_pSendBuffer, &_sendBufferSize,
                                   CMD_ADMIN_PREFIX CMD_NAME_SETSESS_ATTR,
                                   0, 0, 0, -1, newObj.objdata(), NULL,
                                   NULL, NULL, _endianConvert ) ;
      if ( rc )
      {
         goto error ;
      }
      lock() ;
      locked = TRUE ;
      rc = _send( _pSendBuffer ) ;
      if ( rc )
      {
         goto error ;
      }
      rc = _recvExtract ( &_pReceiveBuffer, &_receiveBufferSize,
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }

      CHECK_RET_MSGHEADER( _pSendBuffer, _pReceiveBuffer, this ) ;

   done :
      if ( locked )
      {
         unlock() ;
      }
      return rc ;

   error :
      goto done ;
   }

   INT32 _sdbImpl::getSessionAttr ( BSONObj & attribute )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN gotAttribute = FALSE ;
      _sdbCursor * cursor = NULL ;

      if ( !isConnected() )
      {
         goto error ;
      }

      _getSessionAttrCache( attribute ) ;
      if ( !attribute.isEmpty() )
      {
         gotAttribute = TRUE ;
         goto done ;
      }

      rc = _runCommand( CMD_ADMIN_PREFIX CMD_NAME_GETSESS_ATTR,
                        NULL, NULL, NULL, NULL,
                        0, 0, 0, -1, &cursor ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

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
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      std::set<ossValuePtr>::iterator it ;
      std::set<ossValuePtr> cursors ;
      std::set<ossValuePtr> lobs ;

      rc = clientBuildKillAllContextsMsg( &_pSendBuffer, &_sendBufferSize, 0,
                                          _endianConvert ) ;
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

      cursors = _cursors ;
      for ( it = cursors.begin(); it != cursors.end(); ++it )
      {
         ((_sdbCursorImpl*)(*it))->_detachConnection () ;
         ((_sdbCursorImpl*)(*it))->_detachCollection () ;
         ((_sdbCursorImpl*)(*it))->_close () ;
      }
      _cursors.clear();
      lobs = _lobs ;
      for ( it = lobs.begin(); it != lobs.end(); ++it )
      {
         ((_sdbLobImpl*)(*it))->_detachConnection () ;
         ((_sdbLobImpl*)(*it))->_detachCollection () ;
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

   INT32 _sdbImpl::isValid( BOOLEAN *result )
   {
      INT32 rc = SDB_OK ;
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

   INT32 _sdbImpl::createDomain ( const CHAR *pDomainName,
                                  const bson::BSONObj &options,
                                  _sdbDomain **domain )
   {
      INT32 rc       = SDB_OK ;
      BOOLEAN result = FALSE ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_CREATE_DOMAIN ) ;
      if ( !pDomainName || ossStrlen ( pDomainName ) >
                               CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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

      rc = _runCommand ( command.c_str(), result, &newObj ) ;
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
      ((sdbDomainImpl*)*domain)->_setConnection ( this ) ;
      ((sdbDomainImpl*)*domain)->_setName ( pDomainName ) ;
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::dropDomain ( const CHAR *pDomainName )
   {
      INT32 rc       = SDB_OK ;
      BOOLEAN result = FALSE ;
      BSONObj newObj ;
      BSONObjBuilder ob ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_DROP_DOMAIN ) ;
      if ( !pDomainName || ossStrlen ( pDomainName ) >
                               CLIENT_COLLECTION_NAMESZ )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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

      rc = _runCommand ( command.c_str(), result, &newObj ) ;
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
      if ( !pDomainName || ossStrlen ( pDomainName ) > CLIENT_COLLECTION_NAMESZ
            || !domain )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
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
      rc = getList ( &cursor.pCursor, SDB_LIST_DOMAINS, newObj ) ;
      if ( rc )
      {
         goto error ;
      }
      if ( SDB_OK == ( rc = cursor.next( result ) ) )
      {
         *domain = (_sdbDomain*)( new(std::nothrow) sdbDomainImpl() ) ;
         if ( !(*domain) )
         {
            rc = SDB_OOM ;
            goto error ;
         }
         ((sdbDomainImpl*)*domain)->_setConnection ( this ) ;
         ((sdbDomainImpl*)*domain)->_setName ( pDomainName ) ;
      }
      else if ( SDB_DMS_EOC == rc )
      {
         rc = SDB_CAT_DOMAIN_NOT_EXIST ;
         goto done ;
      }
      else
      {
         goto error ;
      }

   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::listDomains ( _sdbCursor **cursor,
                       const bson::BSONObj &condition,
                       const bson::BSONObj &selector,
                       const bson::BSONObj &orderBy,
                       const bson::BSONObj &hint
                      )
   {
      INT32 rc = SDB_OK ;
      rc = getList ( cursor, SDB_LIST_DOMAINS,
                     condition, selector, orderBy ) ;
      if ( rc )
         goto error ;
   done :
      return rc ;
   error :
      goto done ;
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

      pDC = (_sdbDataCenter*)( new(std::nothrow) _sdbDataCenterImpl() ) ;
      if ( NULL == pDC )
      {
         rc = SDB_OOM ;
         goto error ;
      }

      ((sdbDataCenterImpl*)pDC)->_setConnection ( this ) ;

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

   INT32 _sdbImpl::syncDB( const bson::BSONObj &options )
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pCommand      = CMD_ADMIN_PREFIX CMD_NAME_SYNC_DB ;

      rc = _runCommand( pCommand, &options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::analyze ( const bson::BSONObj &options )
   {
      INT32 rc                  = SDB_OK ;
      const CHAR *pCommand      = CMD_ADMIN_PREFIX CMD_NAME_ANALYZE ;

      rc = _runCommand( pCommand, &options ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done :
      return rc ;
   error :
      goto done ;
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
      INT32 rc = SDB_OK ;

      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_FORCE_STEP_UP), &options ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::invalidateCache( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_INVALIDATE_CACHE),
                        &options ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::reloadConfig( const BSONObj &options )
   {
      INT32 rc = SDB_OK ;

      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_RELOAD_CONFIG), &options ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_UPDATE_CONFIG), &query ) ;
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_DELETE_CONFIG), &query ) ;
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_SET_PDLEVEL), &query ) ;
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
      SINT64 contextID = 0 ;
      BOOLEAN result = FALSE ;
      BOOLEAN locked = FALSE ;

      if( NULL == msg )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      rc = clientBuildTestMsg( &_pSendBuffer, &_sendBufferSize, msg, 0,
                               _endianConvert ) ;
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
                          contextID, result ) ;
      if ( rc )
      {
         goto error ;
      }
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_LOAD_COLLECTIONSPACE),
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_UNLOAD_COLLECTIONSPACE),
                        &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      rc = removeCachedObject( _tb, csName, false ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
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

   INT32 _sdbImpl::traceStart( UINT32 traceBufferSize, const CHAR* component,
                               const CHAR* breakpoint,
                               const vector<UINT32> &tidVec )
   {
      INT32 rc = SDB_OK ;
      BSONObj query ;

      try
      {
         BSONObjBuilder queryBuilder ;
         queryBuilder.append( FIELD_NAME_SIZE, traceBufferSize ) ;
         {
            BSONArrayBuilder componentBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_COMPONENTS ) ) ;

            rc = _traceStrtok( componentBuilder, component ) ;
            if( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }
         {
            BSONArrayBuilder breakpointBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_BREAKPOINTS ) ) ;

            rc = _traceStrtok( breakpointBuilder, breakpoint ) ;
            if( SDB_OK != rc )
            {
               rc = SDB_DRIVER_BSON_ERROR ;
               goto error ;
            }
         }

         {
            BSONArrayBuilder threadsBuilder(
               queryBuilder.subarrayStart( FIELD_NAME_THREADS ) ) ;
            for( vector< UINT32 >::const_iterator itr = tidVec.begin();
                 itr != tidVec.end(); itr++ )
            {
               threadsBuilder.append( *itr ) ;
            }
         }
         query = queryBuilder.obj() ;
      }
      catch( std::exception )
      {
         rc = SDB_DRIVER_BSON_ERROR ;
         goto error ;
      }
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_TRACE_START), &query ) ;
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_TRACE_STOP), &query ) ;
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
      INT32 rc = SDB_OK ;
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_TRACE_RESUME) ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _sdbImpl::traceStatus( _sdbCursor** cursor )
   {
      INT32 rc = SDB_OK ;
      _sdbCursor *pCursor = NULL ;
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_TRACE_STATUS),
                        NULL, NULL, NULL, NULL, 0, 0, 0, -1, &pCursor ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
      *cursor = pCursor ;
   done:
      return rc ;
   error:
      goto done ;
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
      rc = _runCommand( (CMD_ADMIN_PREFIX CMD_NAME_RENAME_COLLECTIONSPACE),
                        &query ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }

      rc = removeCachedObject( _tb, oldName, FALSE ) ;
      if( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

/*   INT32 _sdbImpl::modifyConfig ( INT32 nodeID,
                                  std::map<std::string,std::string> &config )
   {
      INT32 rc = SDB_OK ;
      bson nodeObj ;
      bson configObj ;
      BOOLEAN result = FALSE ;
      bson_init ( &nodeObj ) ;
      bson_init ( &configObj ) ;
      map<string,string>::iterator it ;
      string command = string ( CMD_ADMIN_PREFIX CMD_NAME_UPDATE_CONFIG ) ;

      rc = bson_append_int ( &nodeObj, CAT_NODEID_NAME, nodeID ) ;
      if ( rc )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      bson_finish ( &nodeObj ) ;

      for ( it = config.begin(); it != config.end(); ++it )
      {
         rc = bson_append_string ( &configObj,
                                   it->first.c_str(),
                                   it->second.c_str() ) ;
         if ( rc )
         {
            rc = SDB_SYS ;
            goto error ;
         }
      }
      bson_finish ( &configObj ) ;

      rc = _runCommand ( command.c_str(), result, &nodeObj, &configObj ) ;
      if ( rc )
      {
         goto error ;
      }
   done :
      bson_destroy ( &nodeObj ) ;
      bson_destroy ( &configObj ) ;
      return rc ;
   error :
      goto done ;
   }

   INT32 _sdbImpl::getConfig ( INT32 nodeID,
                               std::map<std::string,std::string> &config )
   {
      return SDB_OK ;
   }*/

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

}
