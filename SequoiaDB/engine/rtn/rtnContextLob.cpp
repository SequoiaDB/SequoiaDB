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

   Source File Name = rtnContextLob.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnContextLob.hpp"
#include "pmd.hpp"
#include "rtnLobStream.hpp"
#include "rtnLobFetcher.hpp"
#include "rtnTrace.hpp"

using namespace bson ;

namespace engine
{
   /*
      _rtnContextLob implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextLob, RTN_CONTEXT_LOB, "LOB")

   _rtnContextLob::_rtnContextLob( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _stream( NULL ),
    _suLogicalID( DMS_INVALID_LOGICCSID ),
    _offset( -1 ),
    _readLen( 0 )
   {
   }

   _rtnContextLob::~_rtnContextLob()
   {
      if ( NULL != _stream && _stream->isOpened() )
      {
         pmdEDUCB *cb = pmdGetThreadEDUCB() ;
         _stream->closeWithException( cb ) ;
      }
      if ( _stream )
      {
         SDB_OSS_DEL _stream ;
         _stream = NULL ;
      }
   }

   _dmsStorageUnit* _rtnContextLob::getSU()
   {
      return NULL == _stream ? NULL : _stream->getSU() ;
   }

   BOOLEAN _rtnContextLob::isWrite() const
   {
      return ( NULL != _stream ) ?
             ( !( _stream->isReadonly() ) ) :
             ( FALSE ) ;
   }

   const CHAR *_rtnContextLob::getProcessName() const
   {
      return ( NULL != _stream && NULL != _stream->getFullName() ) ?
             ( _stream->getFullName() ) : ( "" ) ;
   }

   INT32 _rtnContextLob::_createLobID( bson::OID &oid )
   {
      INT32 rc = SDB_OK ;
      time_t localTime ;
      time_t utcTime ;
      _utilLobID lobId ;
      BYTE oidArray[UTIL_LOBID_ARRAY_LEN] = { 0 } ;
      _MsgRouteID routeId = pmdGetNodeID() ;

      if ( MSG_INVALID_ROUTEID == routeId.value )
      {
         rc = SDB_INVALID_ROUTEID ;
         PD_LOG( PDERROR, "Route id must be exist when create lob ID:rc=%d",
                 rc ) ;
         goto error ;
      }

      // init lobId with UTC timezone to avoid timezone issue
      localTime = ossGetCurrentMilliseconds() / 1000 ;
      ossTimeLocalToUTCInSameDate( localTime, utcTime ) ;
      rc = lobId.init( (INT64)utcTime, routeId.columns.nodeID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create lob id:rc=%d", rc ) ;

      rc = lobId.toByteArray( oidArray, UTIL_LOBID_ARRAY_LEN ) ;
      if ( SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to get Byte array from lodId[%s]",
                 lobId.toString().c_str(), rc ) ;
         goto error ;
      }

      oid.init( oidArray, UTIL_LOBID_ARRAY_LEN ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_OPEN, "_rtnContextLob::open" )
   INT32 _rtnContextLob::open( const BSONObj &lob,
                               INT32 flags,
                               _pmdEDUCB *cb,
                               SDB_DPSCB *dpsCB,
                               _rtnLobStream *pStream )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_OPEN ) ;
      BSONElement mode ;
      bson::OID oid ;
      BSONElement oidEle ;
      BSONElement fullName ;

      SDB_ASSERT( pStream, "Stream can't be NULL" ) ;
      if ( !pStream )
      {
         rc = SDB_SYS ;
         goto error ;
      }
      _stream = pStream ;

      fullName = lob.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != fullName.type() )
      {
         PD_LOG( PDERROR, "can not find collection name in lob[%s]",
                 lob.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      mode = lob.getField( FIELD_NAME_LOB_OPEN_MODE ) ;
      if ( NumberInt != mode.type() )
      {
         PD_LOG( PDERROR, "invalid mode in meta bsonobj:%s",
                 lob.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      oidEle = lob.getField( FIELD_NAME_LOB_OID ) ;
      if ( EOO == oidEle.type() && SDB_LOB_MODE_CREATEONLY == mode.numberInt() )
      {
         rc = _createLobID( oid ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to create lob id:rc=%d", rc ) ;
      }
      else if ( jstOID == oidEle.type() )
      {
         oid = oidEle.OID() ;
      }
      else
      {
         PD_LOG( PDERROR, "invalid oid in meta bsonobj:%s",
                 lob.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _stream->setUniqueId( contextID() ) ;
      _stream->setDPSCB( dpsCB ) ;

      rc = _stream->open( fullName.valuestr(),
                          oid,
                          mode.Int(),
                          flags,
                          this,
                          cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob stream:%d", rc ) ;
         goto error ;
      }

      if ( NULL != _stream->getSU() )
      {
         _suLogicalID = _stream->getSU()->LogicalCSID() ;
      }

      _isOpened = TRUE ;
      _hitEnd = FALSE ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_WRITE, "_rtnContextLob::write" )
   INT32 _rtnContextLob::write( UINT32 len,
                                const CHAR *buf,
                                INT64 lobOffset,
                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_WRITE ) ;
      SDB_ASSERT( NULL != _stream, "can not be null" ) ;

      if ( -1 != lobOffset && lobOffset != _stream->curOffset() )
      {
         rc = _stream->seek( lobOffset, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to seek lob:%d", rc ) ;
            goto error ;
         }
      }

      if ( SDB_IS_LOBSREADWRITE_MODE(_stream->mode()) )
      {
         _empty() ;  /// clear data in context.
      }

      rc = _stream->write( len, buf, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_LOCK, "_rtnContextLob::lock" )
   INT32 _rtnContextLob::lock( _pmdEDUCB *cb,
                   INT64 offset,
                   INT64 length )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_LOCK ) ;
      SDB_ASSERT( NULL != _stream, "can not be null" ) ;

      rc = _stream->lock( cb, offset, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to lock lob:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_LOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_GETRTDETAIL, "_rtnContextLob::getRTDetail" )
   INT32 _rtnContextLob::getRTDetail( _pmdEDUCB *cb, BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_GETRTDETAIL ) ;
      SDB_ASSERT( NULL != _stream, "can not be null" ) ;

      rc = _stream->getRTDetail( cb, detail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob detail:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_GETRTDETAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_GETLOBMETADATA, "_rtnContextLob::getLobMetaData" )
   INT32 _rtnContextLob::getLobMetaData( BSONObj &meta )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_GETLOBMETADATA ) ;
      SDB_ASSERT( NULL != _stream, "can not be null" ) ;
      rc = _stream->getMetaData( meta ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to get lob meta data:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_GETLOBMETADATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextLob::mode() const
   {
      if ( NULL != _stream )
      {
         return _stream->mode() ;
      }
      else
      {
         return 0 ;
      }
   }

   void _rtnContextLob::getErrorInfo( INT32 rc,
                                      pmdEDUCB *cb,
                                      rtnContextBuf &buffObj )
   {
      if ( _stream )
      {
         _stream->getErrorInfo( rc,  cb, &buffObj ) ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_READ, "_rtnContextLob::read" )
   INT32 _rtnContextLob::read( UINT32 len,
                               SINT64 offset,
                               _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_READ ) ;

      _readLen = len ;
      _offset = offset ;
      if ( -1 != _offset && _offset != _stream->curOffset() )
      {
         _empty() ;  /// clear data in context.
         rc = _stream->seek( _offset, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to seek lob:%d", rc ) ;
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB_CLOSE, "_rtnContextLob::close" )
   INT32 _rtnContextLob::close( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB_CLOSE ) ;

      rc = _stream->close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB_CLOSE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLOB__PREPAGEDATA, "_rtnContextLob::_prepareData" )
   INT32 _rtnContextLob::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLOB__PREPAGEDATA ) ;
      UINT32 read = 0 ;
      if ( 0 == _readLen )
      {
         goto done ;
      }

      rc = _stream->read( _readLen, this, cb, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob:%d", rc ) ;
         goto error ;
      }

      if ( read < _readLen )
      {
         _readLen = DMS_PAGE_SIZE512K ;
         _offset += read ;
      }
      else
      {
         _readLen = 0 ;
         _offset = -1 ;
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLOB__PREPAGEDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextLob::_toString( stringstream &ss )
   {
      if ( _stream )
      {
         ss << ",Name:" << _stream->getFullName()
            << ",OID:" << _stream->getOID().toString().c_str()
            << ",CurOffset:" << _stream->curOffset()
            << ",IsReadOnly:" << _stream->isReadonly() ;
      }
   }

   const CHAR* _rtnContextLob::name() const
   {
      return "LOB" ;
   }

   void _rtnContextLob::onSubmit( const monAppCB &delta )
   {
      getMonCB()->incMetrics( delta ) ;
   }

   /*
      _rtnContextLobFetcher implement
   */

   RTN_CTX_AUTO_REGISTER(_rtnContextLobFetcher, RTN_CONTEXT_LOB_FETCHER, "LOB_FETCHER")

   _rtnContextLobFetcher::_rtnContextLobFetcher( INT64 contextID,
                                                 UINT64 eduID )
   :rtnContextBase( contextID, eduID )
   {
      _pFetcher = NULL ;
      _suLogicalID = DMS_INVALID_LOGICCSID ;
   }

   _rtnContextLobFetcher::~_rtnContextLobFetcher()
   {
      if ( _pFetcher )
      {
         _pFetcher->close() ;
      }
      _pFetcher = NULL ;
   }

   INT32 _rtnContextLobFetcher::open( rtnLobFetcher *pFetcher,
                                      const CHAR *fullName,
                                      BOOLEAN onlyMetaPage )
   {
      INT32 rc = SDB_OK ;

      _pFetcher = pFetcher ;
      if ( _pFetcher )
      {
         rc = _pFetcher->init( fullName, onlyMetaPage ) ;
         if ( SDB_OK == rc )
         {
            if ( NULL != _pFetcher->getSu() )
            {
               _suLogicalID = _pFetcher->getSu()->LogicalCSID() ;
            }
         }
      }
      else
      {
         rc = SDB_SYS ;
      }

      return rc ;
   }

   INT32 _rtnContextLobFetcher::getMore( INT32 maxNumToReturn,
                                         rtnContextBuf &buffObj,
                                         _pmdEDUCB *cb )
   {
      /// not used for call
      return SDB_SYS ;
   }

   rtnLobFetcher* _rtnContextLobFetcher::getLobFetcher()
   {
      return _pFetcher ;
   }

   const CHAR* _rtnContextLobFetcher::name() const
   {
      return "LOB_FETCHER" ;
   }

   RTN_CONTEXT_TYPE _rtnContextLobFetcher::getType () const
   {
      return RTN_CONTEXT_LOB_FETCHER ;
   }

   _dmsStorageUnit* _rtnContextLobFetcher::getSU ()
   {
      if ( _pFetcher )
      {
         return _pFetcher->getSu() ;
      }
      return NULL ;
   }

   void _rtnContextLobFetcher::_toString( stringstream &ss )
   {
      if ( _pFetcher )
      {
         auto position = _pFetcher->toBeFetched();
         ss << ",CollectionName:" << _pFetcher->collectionName()
            << ",HitEnd:" << _pFetcher->hitEnd()
            << ",Position:"
            << "oid:" << position.first
            << "sequence:" << position.second ;
      }
   }

}

