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

   Source File Name = rtnContextListLob.cpp

   Descriptive Name = N/A

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/19/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnContextListLob.hpp"
#include "rtnTrace.hpp"
#include "rtnLob.hpp"
#include "rtnLobPieces.hpp"
#include "rtnLobMetricsSubmitor.hpp"
#include "dmsCB.hpp"

using namespace bson ;

namespace engine
{
   RTN_CTX_AUTO_REGISTER(_rtnContextListLob, RTN_CONTEXT_LIST_LOB, "LIST_LOB") ;

   _rtnContextListLob::_rtnContextListLob( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _suLogicalID( DMS_INVALID_LOGICCSID ),
    _buf( NULL ),
    _bufLen( 0 ),
    _fetchLobHead( TRUE ),
    _skip( 0 ),
    _returnNum( -1 )
   {
      _totalDeltaMonApp.reset() ;
   }

   _rtnContextListLob::~_rtnContextListLob()
   {
   	pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      _close( cb ) ;

      if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _buf = NULL ;
         _bufLen = 0 ;
      }
   }

   _dmsStorageUnit* _rtnContextListLob::getSU()
   {
      return _fetcher.getSu() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB_OPEN, "_rtnContextListLob::open" )
   INT32 _rtnContextListLob::open( const BSONObj &query,
                                   const BSONObj &selector, const BSONObj &hint,
                                   INT64 skip, INT64 returnNum, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB_OPEN ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      BSONElement fullName ;

      _query = query.getOwned() ;
      _selector = selector.getOwned() ;
      _hint = hint.getOwned() ;
      _skip = skip ;
      _returnNum = returnNum ;

      if ( !_selector.isEmpty() )
      {
         rc = _selectorParser.loadPattern ( _selector ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to load selector pattern[%s], rc=%d",
                      _selector.toString().c_str(), rc ) ;
      }

      rc = _matchTree.loadPattern( _query ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to load matchTree pattern[%s], rc=%d",
                   _query.toString().c_str(), rc ) ;

      fullName = _hint.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != fullName.type() )
      {
         PD_LOG( PDERROR, "invalid collection name in hint:%s",
                 _hint.toString( FALSE, TRUE ).c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _fetchLobHead = _hint.getField( FIELD_NAME_LOB_LIST_PIECES_MODE ).eoo() ;

      rc = _fetcher.init( fullName.valuestr(), _fetchLobHead ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init lob fetcher:%d", rc ) ;
         goto error ;
      }
      if ( NULL != _fetcher.getSu() )
      {
         _suLogicalID = _fetcher.getSu()->LogicalCSID() ;
      }

      DMS_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_LIST, 1 ) ;

      _fullName.assign( fullName.valuestr() ) ;

      _isOpened = TRUE ;
      _hitEnd = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB_OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextListLob::onSubmit( const monAppCB & delta )
   {
      _totalDeltaMonApp += delta ;
      getMonCB()->incMetrics( delta ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA, "_rtnContextListLob::_prepareData" )
   INT32 _rtnContextListLob::_prepareData( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA ) ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      BSONObj obj ;
      INT32 returnObjNum = 0 ;

      dmsMBContext *mbContext = _fetcher.getMBContext() ;
      std::unique_ptr< ILobCursor > cursor ;
      std::shared_ptr< ILob > lobPtr ;
      rc = mbContext->getCollPtr()->getLobPtr( lobPtr ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get lob storage ptr, rc: %d", rc ) ;

      rc = lobPtr->list( cb, cursor ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to open cursor to list lob, rc: %d", rc ) ;

      SDB_ASSERT( cursor->isOpened(), "Cursor must be opened" ) ;

      while ( returnObjNum < 1000 && 0 != _returnNum )
      {
         BOOLEAN isMatch = FALSE ;
         rc = _fetchLobHead ? _getMetaInfo( cb, obj ) : _getSequenceInfo( cb, obj ) ;
         if ( SDB_OK == rc )
         {
            rc = _matchTree.matches( obj, isMatch ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to matches obj[%s]:rc=%d", obj.toString().c_str(),
                         rc ) ;
            if ( isMatch )
            {
               BSONObj selObj ;
               if ( _skip > 0 )
               {
                  --_skip ;
                  continue ;
               }

               if ( _returnNum > 0 )
               {
                  --_returnNum ;
               }

               if ( _selectorParser.isInitialized() )
               {
                  rc = _selectorParser.select( obj, selObj ) ;
                  PD_RC_CHECK( rc, PDERROR, "Failed to select obj[%s]:rc=%d",
                               obj.toString().c_str(), rc ) ;
               }
               else
               {
                  selObj = obj ;
               }

               rc = append( selObj ) ;
               PD_RC_CHECK( rc, PDERROR, "Failed to append data to context:%d", rc ) ;
               returnObjNum++ ;
            }
         }
         else if ( SDB_DMS_EOC == rc )
         {
            _hitEnd = TRUE ;
            goto error ;
         }
         else if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
         {
            // Ignore the concurrency during deleteLob and listLobs operations
            continue ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get lob data:%d", rc ) ;
            goto error ;
         }

         if ( 0 == _returnNum )
         {
            _hitEnd = TRUE ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__PREPAGEDATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnContextListLob::_toString( stringstream &ss )
   {
      ss << ",Name:" << _fullName.c_str()
         << ",BuffLen:" << _bufLen ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__GETMETAINFO, "_rtnContextListLob::_getMetaInfo" )
   INT32 _rtnContextListLob::_getMetaInfo( _pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__GETMETAINFO ) ;
      _dmsLobInfoOnPage info ;
      UINT32 read = 0 ;
      const _dmsLobMeta *meta = NULL ;
      UINT64 modificationTime = 0 ;
      BSONObjBuilder builder ;

      rc = _fetcher.fetch( cb, info ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "failed to fetch lob:%d", rc ) ;
         }
         goto error ;
      }

      rc = _reallocate( info._len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to reallocate buf:%d", rc ) ;
         goto error ;
      }

      rc = rtnReadLob( _fullName.c_str(), info._oid, info._sequence, 0,
                       info._len, cb, _buf, read, _fetcher.getSu(),
                       _fetcher.getMBContext() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob[%s], rc:%d",
                 info._oid.str().c_str(), rc ) ;
         goto error ;
      }

      SDB_ASSERT( read == info._len, "impossible" ) ;

      meta = ( const _dmsLobMeta* )_buf ;
      modificationTime = meta->_modificationTime ;
      if ( 0 == modificationTime )
      {
         modificationTime = meta->_createTime ;
      }

      builder.append( FIELD_NAME_LOB_SIZE, meta->_lobLen ) ;
      builder.appendOID( FIELD_NAME_LOB_OID, &( info._oid ) ) ;
      builder.appendTimestamp( FIELD_NAME_LOB_CREATETIME,
                               meta->_createTime,
                               (meta->_createTime - ( meta->_createTime / 1000 * 1000 ) ) * 1000) ;
      builder.appendTimestamp( FIELD_NAME_LOB_MODIFICATION_TIME,
                               modificationTime,
                               (modificationTime - ( modificationTime / 1000 * 1000 ) ) * 1000) ;
      builder.appendBool( FIELD_NAME_LOB_AVAILABLE, meta->isDone() ) ;
#ifdef _DEBUG
      builder.appendBool( FIELD_NAME_LOB_HAS_PIECESINFO, meta->hasPiecesInfo() ) ;
      if ( meta->hasPiecesInfo() && info._len >= DMS_LOB_META_LENGTH )
      {
         BSONArray array ;
         _rtnLobPiecesInfo piecesInfo ;

         INT32 length = meta->_piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
         const CHAR* piecesInfoBuf = (const CHAR*)
                                     ( _buf + DMS_LOB_META_LENGTH - length ) ;

         rc = piecesInfo.readFrom( piecesInfoBuf, length ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read pieces info of lob[%s], rc:%d",
                    info._oid.str().c_str(), rc ) ;
            goto error ;
         }

         rc = piecesInfo.saveTo( array ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to save pieces info of lob[%s], rc:%d",
                    info._oid.str().c_str(), rc ) ;
            goto error ;
         }

         builder.append( FIELD_NAME_LOB_PIECESINFONUM, meta->_piecesInfoNum ) ;
         builder.appendArray( FIELD_NAME_LOB_PIECESINFO, array ) ;
      }
#endif
      obj = builder.obj() ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__GETMETAINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO, "_rtnContextListLob::_getSequenceInfo" )
   INT32 _rtnContextListLob::_getSequenceInfo( _pmdEDUCB *cb, BSONObj &obj )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO ) ;
      _dmsLobInfoOnPage info ;
      BSONObjBuilder builder ;

      rc = _fetcher.fetch( cb, info, NULL ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_DMS_EOC != rc )
         {
            PD_LOG( PDERROR, "failed to fetch lob:%d", rc ) ;
         }
         goto error ;
      }

      builder.appendOID( FIELD_NAME_LOB_OID, &( info._oid ) ) ;
      builder.append( "Sequence", info._sequence ) ;
      obj = builder.obj() ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__GETSEQUENCEINFO, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextListLob::_reallocate( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      if ( len <= _bufLen )
      {
         goto done ;
      }
      else if ( NULL != _buf )
      {
         SDB_OSS_FREE( _buf ) ;
         _bufLen = 0 ;
         _buf = NULL ;
      }

      _buf = ( CHAR * )SDB_OSS_MALLOC( len ) ;
      if ( NULL == _buf )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }
   done:
      return rc;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTLISTLOB__CLOSE, "_rtnContextListLob::_close" )
   void _rtnContextListLob::_close( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCONTEXTLISTLOB__CLOSE ) ;

      INT32 rc = SDB_OK ;
      dmsStorageUnitID suID ;
      _dmsStorageUnit *su = NULL ;
      _dmsMBContext *mbContext = NULL ;
      SDB_DMSCB *dmsCB = pmdGetKRCB()->getDMSCB() ;
      const CHAR *clName = NULL ;
      _monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( _fullName.empty() )
      {
         goto done ;
      }

      // submit snapshot changes to sdb/svctask monitor
      if ( pMonAppCB && pMonAppCB->mondbcb )
      {
         pMonAppCB->mondbcb->incMetrics( _totalDeltaMonApp ) ;
      }
      if ( pMonAppCB && pMonAppCB->getSvcTaskInfo() )
      {
         pMonAppCB->getSvcTaskInfo()->incMetrics( _totalDeltaMonApp ) ;
      }

      // get MBContext, and submit snapshot changes to cl monitor
      rc = rtnResolveCollectionNameAndLock( _fullName.c_str(), dmsCB,
                                            &su, &clName, suID ) ;
      if ( SDB_OK != rc || !su )
      {
         PD_LOG( PDERROR, "Resolve collection[%s] failed, rc: %d",
                 _fullName.c_str(), rc ) ;
         goto error ;
      }

      rc = su->data()->getMBContext( &mbContext, clName, SHARED ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Get collection[%s] mb-context failed, rc: %d",
                 clName, rc ) ;
         goto error ;
      }

      if ( mbContext && mbContext->mbStat() )
      {
         // submit to cl
         mbContext->mbStat()->_crudCB.incMetrics( _totalDeltaMonApp ) ;
      }

      _isOpened = FALSE ;

   done:
      if ( NULL != mbContext && NULL != su )
      {
         su->data()->releaseMBContext( mbContext ) ;
         mbContext = NULL ;
      }
      if ( NULL != su )
      {
         dmsCB->suUnlock ( suID ) ;
         su = NULL ;
         suID = DMS_INVALID_CS ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTLISTLOB__CLOSE, rc ) ;
      return ;
   error:
      goto done ;
   }
}

