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

   Source File Name = rtnLocalLobStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLocalLobStream.hpp"
#include "rtnTrace.hpp"
#include "dmsStorageUnit.hpp"
#include "dmsCB.hpp"
#include "rtn.hpp"
#include "rtnLob.hpp"
#include "rtnLobAccessManager.hpp"
#include "rtnLobMetricsSubmitor.hpp"

using namespace bson ;

namespace engine
{
   class _rtnDMSWritableAssist : public SDBObject
   {
   public:
      _rtnDMSWritableAssist( _pmdEDUCB *cb )
      {
         _cb = cb ;
         _isWriteDMS = FALSE ;
      }

      ~_rtnDMSWritableAssist()
      {
         if ( _isWriteDMS )
         {
            sdbGetDMSCB()->writeDown( _cb ) ;
            _isWriteDMS = FALSE ;
         }
      }

   public:
      INT32 writable()
      {
         SDB_ASSERT( FALSE == _isWriteDMS, "WriteDMS must be FALSE" ) ;
         INT32 rc = SDB_OK ;
         _SDB_DMSCB *dmsCB = sdbGetDMSCB() ;

         rc = dmsCB->writable( _cb ) ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
         _isWriteDMS= TRUE ;

      done:
         return rc ;
      error:
         goto done ;
      }

   private:
      BOOLEAN _isWriteDMS ;
      _pmdEDUCB *_cb ;
   } ;

   _rtnLocalLobStream::_rtnLocalLobStream()
   :_mbContext( NULL ),
    _su( NULL ),
    _dmsCB( NULL ),
    _accessInfo( NULL ),
    _hasLobPrivilege( FALSE )
   {
   }

   _rtnLocalLobStream::~_rtnLocalLobStream()
   {
      pmdEDUCB *cb = pmdGetThreadEDUCB() ;
      _closeInner( cb ) ;
   }

   void _rtnLocalLobStream::_closeInner( _pmdEDUCB *cb )
   {
      if ( _hasLobPrivilege )
      {
         sdbGetRTNCB()->getLobAccessManager()->releaseAccessPrivilege(
            getFullName(), getOID(), _getMode(), uniqueId() ) ;
         _hasLobPrivilege = FALSE ;
      }
      if ( _mbContext && _su )
      {
         // release mbContext
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( _su && _dmsCB )
      {
         sdbGetDMSCB()->suUnlock ( _su->CSID() ) ;
         _su = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__PREPARE, "_rtnLocalLobStream::_prepare" )
   INT32 _rtnLocalLobStream::_prepare( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__PREPARE ) ;
      dmsStorageUnitID suID = DMS_INVALID_CS ;
      const CHAR *clName = NULL ;
      _dmsCB = sdbGetDMSCB() ;
      _rtnDMSWritableAssist dmsAssist( cb ) ;

      if ( !SDB_IS_LOBREADONLY_MODE( mode() ))
      {
         rc = dmsAssist.writable() ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;
      }

      rc = rtnResolveCollectionNameAndLock( getFullName(), _dmsCB,
                                            &_su, &clName, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to resolve collection name:%s",
                 getFullName() ) ;
         goto error ;
      }

      rc = _su->data()->getMBContext( &_mbContext, clName, -1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to resolve collection name:%s",
                 clName ) ;
         goto error ;
      }

      rc = _getAccessPrivilege( getFullName(), getOID(), mode() ) ;
      if ( SDB_OK != rc )
      {
         if ( SDB_LOB_IS_IN_USE == rc && SDB_LOB_MODE_CREATEONLY == mode() )
         {
            _dmsLobMeta meta ;
            INT32 rcTmp = rtnGetLobMetaData( getFullName(), getOID(), cb,
                                             meta, _su, _mbContext ) ;
            if ( SDB_OK == rcTmp && meta.isDone() )
            {
               // try to create an exist lob, return SDB_FE
               rc = SDB_FE ;
            }
         }

         PD_LOG( PDERROR, "Failed to get lob privilege:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__PREPARE, rc ) ;
      return rc ;
   error:
      _closeInner( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__GETACCESSPRIVILEGE, "_rtnLocalLobStream::_getAccessPrivilege" )
   INT32 _rtnLocalLobStream::_getAccessPrivilege( const CHAR *fullName,
                                                  const bson::OID &oid,
                                                  INT32 mode )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__GETACCESSPRIVILEGE ) ;

      for ( INT32 i = 0 ; i < RTN_LOB_ACCESS_PRIVILEGE_RETRY_TIMES ; i++ )
      {
         rc = sdbGetRTNCB()->getLobAccessManager()->getAccessPrivilege(
                  fullName, oid, mode, uniqueId(), &_accessInfo ) ;
         if ( SDB_OK == rc )
         {
            _hasLobPrivilege = TRUE ;
            break ;
         }
         else if ( SDB_LOB_IS_IN_USE == rc )
         {
            ossSleepmillis( RTN_LOB_ACCESS_PRIVILEGE_RETRY_INTERVAL ) ;
            continue ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to get lob privilege:%d", rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__GETACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA, "_rtnLocalLobStream::_queryLobMeta" )
   INT32 _rtnLocalLobStream::_queryLobMeta( _pmdEDUCB *cb,
                                            _dmsLobMeta &meta,
                                            BOOLEAN allowUncompleted,
                                            _rtnLobPiecesInfo* piecesInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA ) ;
      UINT32 len = _su->getLobPageSize() ;
      UINT32 readLen = 0 ;
      CHAR *buf = NULL ;
      dmsLobRecord record ;

      if ( SDB_HAS_LOBWRITE_MODE(_getMode()) )
      {
         rc = _queryLobMeta4Write( cb, meta, piecesInfo ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to query meta for write, rc: %d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      _getPool().clear() ;

      rc = _getPool().allocate( len, &buf ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to alloc buffer[%u], rc: %d",
                 len, rc ) ;
         goto error ;
      }

      record.set( &getOID(), DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

      /// read whole the meta page
      rc = _su->lob()->read( record, _mbContext, cb, buf, readLen ) ;
      if ( SDB_OK == rc )
      {
         if ( readLen < sizeof( _dmsLobMeta ) )
         {
            PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                    "meta size[%u]", getOID().str().c_str(),
                    sizeof( _dmsLobMeta ) ) ;
            rc = SDB_SYS ;
            goto error ;
         }
         /// copy data
         ossMemcpy( (void*)&meta, buf, sizeof( meta ) ) ;
         if ( !meta.isDone() && !allowUncompleted )
         {
            PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                    getOID().str().c_str(), meta.toString().c_str() ) ;
            rc = SDB_LOB_IS_NOT_AVAILABLE ;
            goto error ;
         }

         if ( 0 == meta._modificationTime )
         {
            meta._modificationTime = meta._createTime ;
         }

         /// if meta page has data, push the data to pool
         if ( meta._version >= DMS_LOB_META_MERGE_DATA_VERSION &&
              meta._lobLen > 0 &&
              readLen > DMS_LOB_META_LENGTH )
         {
            rc = _getPool().push( buf + DMS_LOB_META_LENGTH,
                                  ( meta._lobLen <= readLen-DMS_LOB_META_LENGTH ?
                                  meta._lobLen : readLen-DMS_LOB_META_LENGTH ),
                                  0 ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to push data to pool, rc:%d", rc ) ;
               goto error ;
            }
            _getPool().pushDone() ;
         }

         if ( NULL != piecesInfo &&
              meta.hasPiecesInfo() )
         {
            INT32 length = meta._piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
            const CHAR* piecesInfoBuf = (const CHAR*)
                                        ( buf + DMS_LOB_META_LENGTH - length ) ;

            rc = piecesInfo->readFrom( piecesInfoBuf, length ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read pieces info from meta, rc:%d", rc ) ;
               goto error ;
            }
         }

         goto done ;
      }
      else
      {
         if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
         {
            rc = SDB_FNE ;
         }
         else if ( SDB_FNE != rc )
         {
            PD_LOG( PDERROR, "Failed to get meta of lob, rc:%d", rc ) ;
         }
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__QUERYLOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_queryLobMeta4Write( _pmdEDUCB *cb,
                                   _dmsLobMeta &meta,
                                   _rtnLobPiecesInfo* piecesInfo )
   {
      INT32 rc = SDB_OK ;
      _rtnLobMetaCache* metaCache = NULL ;
      const _dmsLobMeta* cachedMeta = NULL ;
      BOOLEAN accessInfoLocked = FALSE ;
      SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;
      SDB_ASSERT( SDB_HAS_LOBWRITE_MODE(_getMode()),
                  "should be write mode" ) ;

      _accessInfo->lock() ;
      accessInfoLocked = TRUE ;

      metaCache = _accessInfo->getMetaCache() ;
      if ( NULL == metaCache )
      {
         metaCache = SDB_OSS_NEW _rtnLobMetaCache() ;
         if ( NULL == metaCache )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to new _rtnLobMetaCache, rc:%d", rc ) ;
            goto error ;
         }
         _accessInfo->setMetaCache( metaCache ) ;
      }
      else
      {
         metaCache->setNeedMerge( TRUE ) ;
      }

      cachedMeta = metaCache->lobMeta() ;
      if ( NULL == cachedMeta )
      {
         UINT32 readLen = 0 ;
         UINT32 len = DMS_LOB_META_LENGTH ;
         CHAR* buf = NULL ;
         dmsLobRecord record ;

         _getPool().clear() ;

         rc = _getPool().allocate( len, &buf ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to alloc buffer[%u], rc: %d",
                    len, rc ) ;
            goto error ;
         }

         record.set( &getOID(), DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

         rc = _su->lob()->read( record, _mbContext, cb, buf, readLen ) ;
         if ( SDB_OK == rc )
         {
            if ( readLen < sizeof( meta ) )
            {
               PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                       "meta size[%u]", getOID().str().c_str(),
                       sizeof( meta ) ) ;
               rc = SDB_SYS ;
               goto error ;
            }

            ossMemcpy( &meta, buf, sizeof( meta ) ) ;

            if ( !meta.isDone() )
            {
               PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                       getOID().str().c_str(), meta.toString().c_str() ) ;
               rc = SDB_LOB_IS_NOT_AVAILABLE ;
               goto error ;
            }

            if ( meta.hasPiecesInfo() && NULL != piecesInfo )
            {
               INT32 length = meta._piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
               const CHAR* pieces = (const CHAR*)
                  ( buf + DMS_LOB_META_LENGTH - length ) ;

               rc = piecesInfo->readFrom( pieces, length ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDINFO, "Failed to read pieces info of Lob[%s]",
                          getOID().str().c_str() ) ;
                  goto error ;
               }
            }

            rc = metaCache->cache( *(_dmsLobMeta*)buf ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDINFO, "Failed to cache meta data of Lob[%s]",
                       getOID().str().c_str() ) ;
               goto error ;
            }
         }
         else
         {
            if ( SDB_LOB_SEQUENCE_NOT_EXIST == rc )
            {
               rc = SDB_FNE ;
            }
            else if ( SDB_FNE != rc )
            {
               PD_LOG( PDERROR, "Failed to get meta of lob, rc:%d", rc ) ;
            }
            goto error ;
         }
      }
      else
      {
         ossMemcpy( &meta, cachedMeta, sizeof( meta ) ) ;

         if ( meta.hasPiecesInfo() && NULL != piecesInfo )
         {
            INT32 length = meta._piecesInfoNum *
                           (INT32)sizeof( _rtnLobPieces ) ;
            rc = piecesInfo->readFrom( (CHAR*)cachedMeta + DMS_LOB_META_LENGTH - length,
                                      length ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to read lob pieces info, rc:%d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      if ( accessInfoLocked )
      {
         _accessInfo->unlock() ;
         accessInfoLocked = FALSE ;
      }
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__ENSURELOB, "_rtnLocalLobStream::_ensureLob" )
   INT32 _rtnLocalLobStream::_ensureLob( _pmdEDUCB *cb,
                                         _dmsLobMeta &meta,
                                         BOOLEAN &isNew )
   {
      INT32 rc = SDB_OK ;
      _dmsLobMeta tmpMeta ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__ENSURELOB ) ;

      rc = _su->lob()->getLobMeta( getOID(), _mbContext,
                                   cb, tmpMeta ) ;
      if ( SDB_OK == rc )
      {
         if ( !tmpMeta.isDone() )
         {
            PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                    getOID().str().c_str(), tmpMeta.toString().c_str() ) ;
            rc = SDB_LOB_IS_NOT_AVAILABLE ;
            goto error ;
         }
         isNew = FALSE ;
         goto done ;
      }
      else if ( SDB_FNE != rc )
      {
         PD_LOG( PDERROR, "Failed to get meta of lob, rc:%d", rc ) ;
         goto error ;
      }
      else
      {
         rc = SDB_OK ;
         isNew = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__ENSURELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__COMPLETELOB, "_rtnLocalLobStream::_completeLob" )
   INT32 _rtnLocalLobStream::_completeLob( const _rtnLobTuple &tuple,
                                           _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__COMPLETELOB ) ;
      _rtnDMSWritableAssist dmsAssist( cb ) ;
      if ( SDB_LOB_MODE_CREATEONLY == _getMode() )
      {
         rc = dmsAssist.writable() ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

         rc = _write( tuple, cb ) ;
      }
      else if ( SDB_HAS_LOBWRITE_MODE(_getMode())
                || SDB_LOB_MODE_TRUNCATE == _getMode() )
      {
         rc = dmsAssist.writable() ;
         PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

         rc = _update( tuple, cb ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__COMPLETELOB, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_getLobPageSize( INT32 &pageSize )
   {
      SDB_ASSERT( NULL != _su, "can not be null" ) ;
      pageSize = _su->getLobPageSize() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__WRITE, "_rtnLocalLobStream::_write" )
   INT32 _rtnLocalLobStream::_write( const _rtnLobTuple &tuple,
                                     _pmdEDUCB *cb, BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__WRITE ) ;
      _dmsLobRecord record ;
      _rtnDMSWritableAssist assist( cb ) ;

      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      record.set( &getOID(),
                  tuple.tuple.columns.sequence,
                  tuple.tuple.columns.offset,
                  tuple.tuple.columns.len,
                  tuple.data ) ;

      // write or update to dms
      if ( orUpdate )
      {
         rc = _su->lob()->writeOrUpdate( record, _mbContext, cb,
                                         _getDPSCB() ) ;
      }
      else
      {
         rc = _su->lob()->write( record, _mbContext, cb,
                                 _getDPSCB() ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob[%s],"
                 "sequence:%d, rc:%d", record._oid->str().c_str(),
                 record._sequence, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__WRITEV, "_rtnLocalLobStream::_writev" )
   INT32 _rtnLocalLobStream::_writev( const RTN_LOB_TUPLES &tuples,
                                      _pmdEDUCB *cb, BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__WRITEV ) ;
      _rtnDMSWritableAssist assist( cb ) ;
      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         SDB_ASSERT( !itr->empty(), "can not be empty" ) ;
         if ( cb->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         rc = _write( *itr, cb, orUpdate ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__WRITEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__UPDATE, "_rtnLocalLobStream::_update" )
   INT32 _rtnLocalLobStream::_update( const _rtnLobTuple &tuple,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN accessInfoLocked = FALSE ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__UPDATE ) ;

      dmsLobRecord record ;
      const MsgLobTuple &t = tuple.tuple ;
      CHAR* buf = NULL ;
      _rtnDMSWritableAssist assist( cb ) ;

      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      record.set( &getOID(), t.columns.sequence, t.columns.offset,
                  t.columns.len, ( const CHAR * )tuple.data ) ;

      if ( DMS_LOB_META_SEQUENCE == t.columns.sequence &&
           SDB_HAS_LOBWRITE_MODE(_getMode()) &&
           0 == t.columns.offset &&
           t.columns.len >= sizeof(_dmsLobMeta) )
      {
         _rtnLobMetaCache* metaCache = NULL ;
         const _dmsLobMeta* meta = (const _dmsLobMeta*)tuple.data ;
         if ( meta->hasPiecesInfo() && t.columns.len < DMS_LOB_META_LENGTH )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid lob meta data length:%d, rc=%d",
                    t.columns.len, rc ) ;
            goto error ;
         }

         SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;

         _accessInfo->lock() ;
         accessInfoLocked = TRUE ;

         metaCache = _accessInfo->getMetaCache() ;
         SDB_ASSERT( NULL != metaCache, "metaCache is null" ) ;
         SDB_ASSERT( NULL != metaCache->lobMeta(), "lob meta cache is null" ) ;

         if ( metaCache->needMerge() )
         {
            _rtnLobMetaCache newCache ;
            rc = newCache.cache( *(metaCache->lobMeta()) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to cache lob meta, rc=%d", rc ) ;
               goto error ;
            }

            rc = newCache.merge( *meta, _su->getLobPageSize() ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to merge lob meta data, rc=%d", rc ) ;
               goto error ;
            }

            SDB_ASSERT( NULL != newCache.lobMeta(), "new lob meta cache is null" ) ;

            rc = metaCache->cache( *( newCache.lobMeta() ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to cache lob meta, rc=%d", rc ) ;
               goto error ;
            }

            buf = (CHAR*)SDB_OSS_MALLOC( record._dataLen > DMS_LOB_META_LENGTH ?
                                         record._dataLen : DMS_LOB_META_LENGTH ) ;
            if ( NULL == buf )
            {
               rc = SDB_OOM ;
               PD_LOG( PDERROR, "Failed to malloc buf, rc=%d", rc ) ;
               goto error ;
            }
            ossMemset( buf, 0, DMS_LOB_META_LENGTH ) ;

            // copy record data to buf,
            // because in local stream,
            // tuple.data actually is rtnLobStream->_meta
            // we can't directly override its memory,
            // especially when newCache has pieces info,
            // it will override memory after rtnLobStream->_meta
            ossMemcpy( buf, record._data, record._dataLen ) ;
            record._data = (const CHAR*)buf ;

            if ( newCache.lobMeta()->hasPiecesInfo() )
            {
               ossMemcpy( (void*)record._data, newCache.lobMeta(), DMS_LOB_META_LENGTH ) ;
               if ( record._dataLen < DMS_LOB_META_LENGTH )
               {
                  record._dataLen = DMS_LOB_META_LENGTH ;
               }
            }
            else
            {
               ossMemcpy( (void*)record._data, newCache.lobMeta(), sizeof( _dmsLobMeta ) ) ;
            }
         }
         else
         {
            rc = metaCache->cache( *meta ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to cache lob meta, rc=%d", rc ) ;
               goto error ;
            }
         }

         rc = _su->lob()->update( record, _mbContext, cb, _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update to lob:%d", rc ) ;
            goto error ;
         }

         _accessInfo->unlock() ;
         accessInfoLocked = FALSE ;
      }
      else
      {
         rc = _su->lob()->update( record, _mbContext, cb, _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update to lob:%d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( accessInfoLocked )
      {
         _accessInfo->unlock() ;
      }
      SAFE_OSS_FREE( buf ) ;
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__UPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__UPDATEV, "_rtnLocalLobStream::_updatev" )
   INT32 _rtnLocalLobStream::_updatev( const RTN_LOB_TUPLES &tuples,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__UPDATEV ) ;

      _rtnDMSWritableAssist assist( cb ) ;
      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         SDB_ASSERT( !itr->empty(), "can not be empty" ) ;
         if ( cb->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         rc = _update( *itr, cb ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__UPDATEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__READV, "_rtnLocalLobStream::_readv" )
   INT32 _rtnLocalLobStream::_readv( const RTN_LOB_TUPLES &tuples,
                                     _pmdEDUCB *cb,
                                     const _rtnLobPiecesInfo* piecesInfo )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__READV ) ;
      SDB_ASSERT( !tuples.empty(), "can not be empty" ) ;

      CHAR *buf = NULL ;
      SINT64 readSize = 0 ;
      INT32 pageSize =  _su->getLobPageSize() ;
      UINT32 needLen = 0 ;
      _getPool().clear() ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         needLen += itr->tuple.columns.len ;
      }

      rc = _getPool().allocate( needLen, &buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to allocate buf:%d", rc ) ;
         goto error ;
      }

      for ( RTN_LOB_TUPLES::const_iterator iter = tuples.begin() ;
            iter != tuples.end() ;
            ++iter )
      {
         const _rtnLobTuple& t = *iter ;

         if ( NULL != piecesInfo &&
              !piecesInfo->hasPiece( t.tuple.columns.sequence ) )
         {
            ossMemset( buf + readSize, 0, t.tuple.columns.len ) ;
         }
         else
         {
            rc = _read( t, cb, buf + readSize ) ;
            if ( SDB_OK != rc )
            {
               goto error ;
            }
         }
         readSize += t.tuple.columns.len ;
      }
      SDB_ASSERT( readSize == needLen, "impossible" ) ;

      rc = _getPool().push( buf, readSize,
                            RTN_LOB_GET_OFFSET_OF_LOB(
                                pageSize,
                                tuples.begin()->tuple.columns.sequence,
                                tuples.begin()->tuple.columns.offset,
                                _getMeta()._version >= DMS_LOB_META_MERGE_DATA_VERSION ) ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to push data to pool:%d", rc ) ;
         goto error ;
      }

      _getPool().pushDone() ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__READV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_read( const _rtnLobTuple &tuple,
                                    _pmdEDUCB *cb,
                                    CHAR *buf )
   {
      INT32 rc = SDB_OK ;
      UINT32 len = 0 ;
      dmsLobRecord record ;
      record.set( &getOID(),
                  tuple.tuple.columns.sequence,
                  tuple.tuple.columns.offset,
                  tuple.tuple.columns.len,
                  tuple.data ) ;
      rc = _su->lob()->read( record, _mbContext, cb,
                             buf, len ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob[%s], sequence[%d], rc:%d",
                 record._oid->str().c_str(), record._sequence, rc ) ;
         goto error ;
      }

      SDB_ASSERT( len == record._dataLen, "impossible" ) ;
   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__ROLLBACK, "_rtnLocalLobStream::_rollback" )
   INT32 _rtnLocalLobStream::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__ROLLBACK ) ;
      dmsLobRecord piece ;
      _rtnDMSWritableAssist assist( cb ) ;
      INT32 num = _getSequence( curOffset() ) ;

      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      while ( 0 < num )
      {
         --num ;
         piece.set( &getOID(), num, 0, 0, NULL ) ;
         rc = _su->lob()->remove( piece, _mbContext, cb,
                                  _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to remove lob[%s],"
                    "sequence:%d, rc:%d", piece._oid->str().c_str(),
                    piece._sequence, rc ) ;
            if ( SDB_LOB_SEQUENCE_NOT_EXIST != rc )
            {
               /// include SDB_DMS_CS_DELETING
               goto error ;
            }
            rc = SDB_OK ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__ROLLBACK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA, "_rtnLocalLobStream::_queryAndInvalidateMetaData" )
   INT32 _rtnLocalLobStream::_queryAndInvalidateMetaData( _pmdEDUCB *cb,
                                                          _dmsLobMeta &meta )
   {
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA ) ;
      INT32 rc = SDB_OK ;
      _rtnDMSWritableAssist assist( cb ) ;

      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      rc = rtnQueryAndInvalidateLob( getFullName(), getOID(),
                                     cb, 1, _getDPSCB(), meta,
                                     NULL, NULL, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to invalidate lob:%d", rc ) ;
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__QUERYANDINVALIDATEMETADATA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__LOCK, "_rtnLocalLobStream::_lock" )
   INT32 _rtnLocalLobStream::_lock( _pmdEDUCB *cb,
                             INT64 offset,
                             INT64 length )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__LOCK ) ;

      if ( -1 == uniqueId() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "invalid unique id, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;

      _accessInfo->lock() ;
      locked = TRUE ;

      rc = _accessInfo->lockSection( _getMode(), offset, length, uniqueId() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to lock section[%lld, %lld, %lld], rc=%d",
                 offset, length, uniqueId(), rc ) ;
         goto error ;
      }

      _accessInfo->unlock() ;
      locked = FALSE ;

   done:
      if ( locked )
      {
         _accessInfo->unlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__LOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_getRTDetail( _pmdEDUCB *cb,
                                           bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BSONObjBuilder builder ;

      if ( -1 == uniqueId() )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "invalid unique id, rc=%d", rc ) ;
         goto error ;
      }

      SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;

      _accessInfo->lock() ;
      locked = TRUE ;

      rc = _accessInfo->toBSONObjBuilder( builder ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get accessInfo:rc=%d", rc ) ;
         goto error ;
      }

      _accessInfo->unlock() ;
      locked = FALSE ;

      try
      {
         builder.append( FIELD_NAME_CONTEXTID, uniqueId() ) ;
         detail = builder.obj() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to build obj, occur unexpected "
                 "error:%s", e.what() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

   done:
      if ( locked )
      {
         _accessInfo->unlock() ;
         locked = FALSE ;
      }
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__LOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLocalLobStream::_close( _pmdEDUCB *cb )
   {
      _closeInner( cb ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOCALLOBSTREAM__REMOVEV, "_rtnLocalLobStream::_removev" )
   INT32 _rtnLocalLobStream::_removev( const RTN_LOB_TUPLES &tuples,
                                       _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOCALLOBSTREAM__REMOVEV ) ;
      dmsLobRecord record ;
      _rtnDMSWritableAssist assist( cb ) ;

      rc = assist.writable() ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable, rc = %d", rc ) ;

      for ( RTN_LOB_TUPLES::const_iterator itr = tuples.begin() ;
            itr != tuples.end() ;
            ++itr )
      {
         record.set( &getOID(),
                     itr->tuple.columns.sequence,
                     itr->tuple.columns.offset,
                     itr->tuple.columns.len,
                     itr->data ) ;
         rc = _su->lob()->remove( record, _mbContext, cb,
                                  _getDPSCB() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove lob[%s],"
                    "sequence:%d, rc:%d", record._oid->str().c_str(),
                    record._sequence, rc ) ;
            goto error ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOCALLOBSTREAM__REMOVEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _rtnLocalLobStream::_onIncreaseMetrics( const monAppCB &delta )
   {
      if ( _mbContext && _mbContext->mbStat() )
      {
         BOOLEAN doLock = FALSE ;
         BOOLEAN isOk = TRUE ;
         // test and get mb lock before submitting
         if ( !_mbContext->isMBLock() )
         {
            INT32 rc = _mbContext->mbLock( SHARED ) ;
            if ( SDB_OK == rc )
            {
               doLock = TRUE ;
            }
            else
            {
               PD_LOG( PDWARNING, "Failed to lock mb context, rc: %d", rc ) ;
               isOk = FALSE ;
            }
         }
         // submit the change to cl snapshot
         if ( isOk )
         {
            _mbContext->mbStat()->_crudCB.incMetrics( delta ) ;
         }
         if ( doLock )
         {
            _mbContext->mbUnlock() ;
         }
      }
   }

}

