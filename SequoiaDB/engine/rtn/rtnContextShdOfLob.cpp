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

   Source File Name = rtnContextShdOfLob.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnContextShdOfLob.hpp"
#include "rtnLobStream.hpp"
#include "rtnLobMetaCache.hpp"
#include "pmdEDU.hpp"
#include "rtnTrace.hpp"
#include "rtnLob.hpp"
#include "clsMgr.hpp"
#include "ossUtil.hpp"
#include "rtnLobMetricsSubmitor.hpp"

using namespace bson ;

namespace engine
{
   RTN_CTX_AUTO_REGISTER(_rtnContextShdOfLob, RTN_CONTEXT_SHARD_OF_LOB, "SHARD_OF_LOB")

   _rtnContextShdOfLob::_rtnContextShdOfLob( INT64 contextID, UINT64 eduID )
   :_rtnContextBase( contextID, eduID ),
    _mode( SDB_LOB_MODE_READ ),
    _flags( 0 ),
    _isMainShd( FALSE ),
    _w( 1 ),
    _version( 0 ),
    _dpsCB( NULL ),
    _closeWithException( TRUE ),
    _buf( NULL ),
    _bufLen( 0 ),
    _accessInfo( NULL ),
    _su( NULL ),
    _suLogicalID( DMS_INVALID_LOGICCSID ),
    _mbContext( NULL ),
    _dmsCB( NULL ),
    _reopened( FALSE ),
    _isMetaWrote( FALSE ),
    _opType( MON_LOB_OP_NONE )
   {
      _pData = NULL ;
      _dataLen = 0 ;
      _offset = 0 ;
      _hasSubmitMetric = FALSE ;
      _totalDeltaMonApp.reset() ;
   }

   _rtnContextShdOfLob::~_rtnContextShdOfLob()
   {
      _pmdEDUCB *cb = pmdGetThreadEDUCB() ;

      if ( _closeWithException )
      {
         if ( SDB_LOB_MODE_CREATEONLY == _mode )
         {
            SDB_ASSERT( cb->getID() == eduID(), "impossible" ) ;
            _rollback( cb ) ;
         }
         else if ( SDB_LOB_MODE_REMOVE == _mode && _isMainShd )
         {
            rtnQueryAndInvalidateLob( _getRealCLName(),
                                      _oid, cb, _w,
                                      _dpsCB, _meta,
                                      _su, _mbContext ) ;
         }
         else
         {
            PD_LOG( PDWARNING, "Lob[%s] is closed with exception, mode:0x%08x",
                    getOID().str().c_str(), _mode ) ;
         }
      }

      close( cb ) ;

      SAFE_OSS_FREE( _buf ) ;
   }

   _dmsStorageUnit* _rtnContextShdOfLob::getSU ()
   {
      return _su ;
   }

   BOOLEAN _rtnContextShdOfLob::isWrite() const
   {
      return SDB_IS_LOBREADONLY_MODE( _mode ) ? FALSE : TRUE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_OPEN, "_rtnContextShdOfLob::open" )
   INT32 _rtnContextShdOfLob::open( const BSONObj &lob,
                                    SINT32 flag,
                                    SINT32 version,
                                    SINT16 w,
                                    SDB_DPSCB *dpsCB,
                                    _pmdEDUCB *cb,
                                    const CHAR **data,
                                    UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_OPEN ) ;
      _dmsCB = sdbGetDMSCB() ;
      const CHAR *clName = NULL ;
      dmsStorageUnitID suID = DMS_INVALID_SUID ;
      _utilSectionMgr sectionMgr ;
      BOOLEAN writeDMS = FALSE ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;

      rc = _parseOpenArgs( lob, sectionMgr ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Invalid open arguments: %s, rc=%d",
                 lob.toString().c_str(), rc ) ;
         goto error ;
      }

      // Check writable first
      if ( !SDB_IS_LOBREADONLY_MODE( _mode ) )
      {
         rc = _dmsCB->writable( cb ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "database is not writable, rc = %d", rc ) ;
            goto error ;
         }
         writeDMS = TRUE ;
      }

      rc = rtnResolveCollectionNameAndLock( _getRealCLName(),
                                            _dmsCB, &_su,
                                            &clName, suID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get cs lock: %s, rc: %d",
                 _getRealCLName(), rc ) ;
         goto error ;
      }
      _suLogicalID = _su->LogicalCSID() ;

      /// get mb context
      rc = _su->data()->getMBContext( &_mbContext, clName, -1 ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to get collection[%s] mb context, rc: %d",
                 _getRealCLName(), rc ) ;
         goto error ;
      }

      _w = w ;
      _dpsCB = dpsCB ;
      _version = version ;
      _flags = flag ;

      if ( _isMainShd )
      {
         rc = _getAccessPrivilege() ;
         if ( SDB_OK != rc )
         {
            if ( SDB_LOB_IS_IN_USE == rc && SDB_LOB_MODE_CREATEONLY == _mode )
            {
               _dmsLobMeta meta ;
               INT32 rcTmp = rtnGetLobMetaData( _getRealCLName(), _oid, cb,
                                                meta, _su, _mbContext ) ;
               if ( SDB_OK == rcTmp && meta.isDone() )
               {
                  // try to create an exist lob, return SDB_FE
                  rc = SDB_FE ;
               }
            }
            PD_LOG( PDERROR, "Failed to get lob privilege:cl=%s,oid=%s,rc=%d",
                    _getRealCLName(), _oid.toString().c_str(), rc ) ;
            goto error ;
         }
      }

      rc = _open( cb, sectionMgr, data, read ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob:%d", rc ) ;
         goto error ;
      }
      _isOpened = TRUE ;
      _hitEnd = FALSE ;

      if ( _isMainShd && _reopened )
      {
         PD_LOG( PDEVENT, "Reopened main shard" ) ;
      }

      PD_LOG( PDDEBUG, "Open SHARD_LOB context on [%s/%s], mode [%d/%s]",
              getFullName(), _getRealCLName(), _mode, rtnLobOpName( _mode ) ) ;

      // monitor Lob operation count
      _increaseLobOpCount( cb ) ;

   done:
      /// write down
      if ( writeDMS )
      {
         _dmsCB->writeDown( cb ) ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_OPEN, rc ) ;
      return rc ;
   error:
      // only when fail to open context,
      // we reset lob operation type to None
      _opType = MON_LOB_OP_NONE ;
      close( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__WRITE, "_rtnContextShdOfLob::_write" )
   INT32 _rtnContextShdOfLob::_write( UINT32 sequence,
                                      UINT32 offset,
                                      UINT32 len,
                                      const CHAR *data,
                                      _pmdEDUCB *cb,
                                      BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__WRITE ) ;
      BOOLEAN updated = FALSE ;

      if ( orUpdate )
      {
         rc = rtnWriteOrUpdateLob( _getRealCLName(),
                                   _oid, sequence,
                                   offset, len, data, cb,
                                   _w, _dpsCB, _su, _mbContext,
                                   &updated ) ;
      }
      else
      {
         rc = rtnWriteLob( _getRealCLName(),
                           _oid, sequence,
                           offset, len, data, cb,
                           _w, _dpsCB, _su, _mbContext ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
         goto error ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == _mode )
      {
         if ( !updated )
         {
            _written.insert( sequence ) ;
         }

         if ( DMS_LOB_META_SEQUENCE == sequence )
         {
            _isMetaWrote = TRUE ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_WRITE, "_rtnContextShdOfLob::write" )
   INT32 _rtnContextShdOfLob::write( UINT32 sequence,
                                     UINT32 offset,
                                     UINT32 len,
                                     const CHAR *data,
                                     _pmdEDUCB *cb,
                                     BOOLEAN orUpdate )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_WRITE ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;

      rc = _write( sequence, offset, len, data, cb, orUpdate ) ;
      if ( SDB_OK == rc )
      {
         // monitor lob bytes write by client
         RTN_MON_LOB_BYTES_COUNT_INC( pMonAppCB, MON_LOB_WRITE_BYTES, len ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_WRITE, rc ) ;
      return rc ;
   }

   UINT32 _rtnContextShdOfLob::_getDataLen( UINT32 sequence,
                                            UINT32 offset,
                                            UINT32 len )
   {
      UINT32 dataLen = 0 ;
      if ( DMS_LOB_META_SEQUENCE == sequence )
      {
         // we are in meta page, let's calculate the lob data size
         // inside this page
         if ( offset + len <= DMS_LOB_META_LENGTH )
         {
            dataLen = 0 ;
         }
         else if ( offset <= DMS_LOB_META_LENGTH &&
                   ( offset + len ) > DMS_LOB_META_LENGTH )
         {
            dataLen = offset + len - DMS_LOB_META_LENGTH ;
         }
         else if ( offset > DMS_LOB_META_LENGTH )
         {
            dataLen = len ;
         }
         else
         {
            SDB_ASSERT( FALSE, "impossible" ) ;
         }
      }
      else
      {
         dataLen = len ;
      }
      return dataLen ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_UPDATE, "_rtnContextShdOfLob::update" )
   INT32 _rtnContextShdOfLob::update( UINT32 sequence,
                                      UINT32 offset,
                                      UINT32 len,
                                      const CHAR *data,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN accessInfoLocked = FALSE ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_UPDATE ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      BOOLEAN writeDMS = FALSE ;
      UINT32 dataLen = 0 ;

      // monitor lob bytes write by client
      dataLen = _getDataLen( sequence, offset, len ) ;
      RTN_MON_LOB_BYTES_COUNT_INC( pMonAppCB, MON_LOB_WRITE_BYTES, dataLen ) ;

      rc = _dmsCB->writable( cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Database is not writable[%d]", rc ) ;
      writeDMS = TRUE ;

      if ( DMS_LOB_META_SEQUENCE == sequence
           && SDB_HAS_LOBWRITE_MODE(_mode)
           && 0 == offset && len >= sizeof(_dmsLobMeta) )
      {
         _rtnLobMetaCache newCache ;
         _rtnLobMetaCache* metaCache = NULL ;
         const _dmsLobMeta* meta = (const _dmsLobMeta*)data ;
         if ( meta->hasPiecesInfo() && len < DMS_LOB_META_LENGTH )
         {
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid lob meta data length:%d, rc=%d",
                    len, rc ) ;
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

            if ( newCache.lobMeta()->hasPiecesInfo() )
            {
               if ( len < DMS_LOB_META_LENGTH )
               {
                  data = (const CHAR*)newCache.lobMeta() ;
                  len = DMS_LOB_META_LENGTH ;
               }
               else
               {
                  ossMemcpy( (void*)data, newCache.lobMeta(), DMS_LOB_META_LENGTH ) ;
               }
            }
            else
            {
               ossMemcpy( (void*)data, newCache.lobMeta(), sizeof( _dmsLobMeta ) ) ;
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

         rc = rtnUpdateLob( _getRealCLName(),
                            _oid, sequence,
                            offset, len, data, cb,
                            _w, _dpsCB, _su, _mbContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
            goto error ;
         }

         _accessInfo->unlock() ;
         accessInfoLocked = FALSE ;
      }
      else if ( DMS_LOB_META_SEQUENCE == sequence &&
                SDB_LOB_MODE_CREATEONLY == _mode &&
                0 == offset && !_isMetaWrote )
      {
         // Before SequoiaDB 3.0, we send UPDATE message to
         // complete lob when close lob in CREATEONLY mode.
         // And in 3.0.1 we also send UPDATE message,
         // in order to compatible with version<3.0.
         // But actually we should WRITE the meta sequence.
         rc = _write( sequence, offset, len, data, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob:%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         rc = rtnUpdateLob( _getRealCLName(),
                            _oid, sequence,
                            offset, len, data, cb,
                            _w, _dpsCB, _su, _mbContext ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to update lob:%d", rc ) ;
            goto error ;
         }
      }

   done:
      if ( accessInfoLocked )
      {
         _accessInfo->unlock() ;
      }

      if ( writeDMS )
      {
         _dmsCB->writeDown( cb ) ;
      }

      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_UPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextShdOfLob::_prepareData( _pmdEDUCB *cb )
   {
      return SDB_OK ;
   }

   void _rtnContextShdOfLob::_toString( stringstream &ss )
   {
      ss << ",Name:" << _fullName.c_str() ;
      if ( !_subCLName.empty() )
      {
         ss << ",SubCLName:" << _subCLName.c_str() ;
      }

      ss << ",OID:" << _oid.toString().c_str()
         << ",Mode:" << _mode
         << ",IsMainShard:" << _isMainShd
         << ",BuffLen:" << _bufLen ;
   }

   INT32 _rtnContextShdOfLob::_parseOpenArgs( const bson::BSONObj &lob,
                                              _utilSectionMgr &sectionMgr )
   {
      INT32 rc = SDB_OK ;

      BSONElement ele = lob.getField( FIELD_NAME_LOB_OPEN_MODE ) ;
      if ( NumberInt != ele.type() )
      {
         PD_LOG( PDERROR, "invalid mode type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _mode = ele.Int() ;

      if ( !SDB_IS_VALID_LOB_MODE( _mode ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid LOB mode: %d", _mode ) ;
         goto error ;
      }

      ele = lob.getField( FIELD_NAME_COLLECTION ) ;
      if ( String != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid full name type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _fullName = ele.String() ;

      ele = lob.getField( FIELD_NAME_SUBCLNAME ) ;
      if ( String == ele.type() )
      {
         _subCLName = ele.String() ;
      }

      ele = lob.getField( FIELD_NAME_LOB_OID ) ;
      if ( jstOID != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid oid type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _oid = ele.OID() ;

      ele = lob.getField( FIELD_NAME_LOB_IS_MAIN_SHD ) ;
      if ( Bool != ele.type() )
      {
         PD_LOG( PDERROR, "Invalid \"isMainShd\" type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }
      _isMainShd = ele.Bool() ;

      ele = lob.getField( FIELD_NAME_LOB_REOPENED ) ;
      if ( Bool == ele.type() )
      {
         _reopened = ele.Bool() ;
      }
      else if ( !ele.eoo() )
      {
         PD_LOG( PDERROR, "invalid \"Reopened\" type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      ele = lob.getField( FIELD_NAME_LOB_META_DATA ) ;
      if ( Object == ele.type() )
      {
         _metaObj = ele.embeddedObject() ;
      }
      else if ( !ele.eoo() )
      {
         PD_LOG( PDERROR, "invalid meta obj type:%d", ele.type() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( _isMainShd && SDB_LOB_MODE_CREATEONLY == _mode )
      {
         ele = lob.getField( FIELD_NAME_LOB_CREATETIME ) ;
         if ( NumberLong == ele.type() )
         {
            _meta._createTime = ele.Long() ;
            _meta._modificationTime = _meta._createTime ;
         }
         else if ( !ele.eoo() )
         {
            PD_LOG( PDERROR, "invalid CreateTime type:%d", ele.type() ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            _meta._createTime = ossGetCurrentMilliseconds() ;
            _meta._modificationTime = _meta._createTime ;
         }
      }

      if ( _isMainShd && _reopened )
      {
         rc = sectionMgr.fromBSONObj( lob ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to parse sections from obj,rc=%d",
                      rc ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__GETACCESSPRIVILEGE, "_rtnContextShdOfLob::_getAccessPrivilege" )
   INT32 _rtnContextShdOfLob::_getAccessPrivilege()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__GETACCESSPRIVILEGE ) ;

      for ( INT32 i = 0 ; i < RTN_LOB_ACCESS_PRIVILEGE_RETRY_TIMES ; i++ )
      {
         rc = sdbGetRTNCB()->getLobAccessManager()->getAccessPrivilege(
                  _getRealCLName(), _oid, _mode, contextID(), &_accessInfo ) ;
         if ( SDB_OK == rc )
         {
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
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__GETACCESSPRIVILEGE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__OPEN, "_rtnContextShdOfLob::_open" )
   INT32 _rtnContextShdOfLob::_open( _pmdEDUCB *cb, _utilSectionMgr &sectionMgr,
                                     const CHAR **data, UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN accessInfoLocked = FALSE ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__OPEN ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( _isMainShd &&
           ( SDB_IS_LOBREADONLY_MODE( _mode ) ||
             SDB_LOB_MODE_TRUNCATE == _mode ) )
      {
         UINT32 readLen = 0 ;
         UINT32 len = _su->getLobPageSize() ;
         dmsLobRecord record ;

         // monitor operation
         _opType |= SDB_IS_LOBREADONLY_MODE( _mode ) ?
                    MON_LOB_OP_GET : MON_LOB_OP_DELETE ;

         // SDB_LOB_MODE_SHAREREAD can't open withData because data should be
         // locked before read data.
         BOOLEAN withData = ( SDB_LOB_MODE_READ == _mode && !_reopened ) ?
                            TRUE : FALSE ;

         record.set( &_oid, DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

         rc = _extendBuf( len * 2 ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to extend buf[%u], rc:%d", len * 2, rc ) ;
            goto error ;
         }
         /// read the whole page
         rc = _su->lob()->read( record, _mbContext, cb, _buf + len, readLen ) ;
         if ( SDB_OK == rc )
         {
            if ( readLen < sizeof( _meta ) )
            {
               PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                       "meta size[%u]", getOID().str().c_str(),
                       sizeof( _meta ) ) ;
               rc = SDB_SYS ;
               goto error ;
            }
            /// copy data
            ossMemcpy( (void*)&_meta, _buf + len, sizeof( _meta ) ) ;
            if ( !_meta.isDone() )
            {
               PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                       getOID().str().c_str(), _meta.toString().c_str() ) ;
               rc = SDB_LOB_IS_NOT_AVAILABLE ;
               goto error ;
            }

            if ( _meta.hasPiecesInfo() )
            {
               INT32 length = _meta._piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
               const CHAR* pieces = (const CHAR*)
                  ( _buf + len + DMS_LOB_META_LENGTH - length ) ;

               rc = _lobPieces.readFrom( pieces, length ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDINFO, "Failed to read pieces info of Lob[%s]",
                          getOID().str().c_str() ) ;
                  goto error ;
               }
            }

            /// if meta page has data
            if ( withData &&
                 _meta._version >= DMS_LOB_META_MERGE_DATA_VERSION &&
                 _meta._lobLen > 0 &&
                 readLen > DMS_LOB_META_LENGTH )
            {
               _pData = _buf + len + DMS_LOB_META_LENGTH ;
               _dataLen = readLen - DMS_LOB_META_LENGTH ;
               if ( _dataLen > _meta._lobLen )
               {
                  _dataLen = _meta._lobLen ;
               }
               _offset = 0 ;
               /// add msg tuple
               _pData -= sizeof( MsgLobTuple ) ;
               _dataLen += sizeof( MsgLobTuple ) ;
               MsgLobTuple *rt = (MsgLobTuple*)_pData ;
               rt->columns.sequence = DMS_LOB_META_SEQUENCE ;
               rt->columns.len = _dataLen - sizeof( MsgLobTuple ) ;
               rt->columns.offset = DMS_LOB_META_LENGTH ;
               /// monitor lob bytes read by client
               RTN_MON_LOB_BYTES_COUNT_INC( pMonAppCB, MON_LOB_READ_BYTES,
                                            rt->columns.len ) ;
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
      else if ( _isMainShd && SDB_HAS_LOBWRITE_MODE(_mode) )
      {
         _rtnLobMetaCache* metaCache = NULL ;
         const _dmsLobMeta* meta = NULL ;
         SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;

         // monitor operation
         _opType |= MON_LOB_OP_PUT ;
         if ( SDB_IS_LOBSREADWRITE_MODE( _mode ) )
         {
            _opType |= MON_LOB_OP_GET ;
         }

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

            if ( _reopened )
            {
               metaCache->setNeedMerge( TRUE ) ;
            }
         }
         else
         {
            metaCache->setNeedMerge( TRUE ) ;
         }

         meta = metaCache->lobMeta() ;
         if ( NULL == meta )
         {
            UINT32 readLen = 0 ;
            UINT32 len = DMS_LOB_META_LENGTH ;
            dmsLobRecord record ;

            record.set( &_oid, DMS_LOB_META_SEQUENCE, 0, len, NULL ) ;

            rc = _extendBuf( len ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to extend buf[%u], rc:%d", len, rc ) ;
               goto error ;
            }

            rc = _su->lob()->read( record, _mbContext, cb, _buf, readLen ) ;
            if ( SDB_OK == rc )
            {
               if ( readLen < sizeof( _meta ) )
               {
                  PD_LOG( PDERROR, "Read lob[%s]'s meta page len is less than "
                          "meta size[%u]", getOID().str().c_str(),
                          sizeof( _meta ) ) ;
                  rc = SDB_SYS ;
                  goto error ;
               }

               ossMemcpy( (void*)&_meta, _buf, sizeof( _meta ) ) ;

               if ( !_meta.isDone() )
               {
                  PD_LOG( PDINFO, "Lob[%s] meta[%s] is not available",
                          getOID().str().c_str(), _meta.toString().c_str() ) ;
                  rc = SDB_LOB_IS_NOT_AVAILABLE ;
                  goto error ;
               }

               if ( _meta.hasPiecesInfo() )
               {
                  INT32 length = _meta._piecesInfoNum * (INT32)sizeof( _rtnLobPieces ) ;
                  const CHAR* pieces = (const CHAR*)
                     ( _buf + DMS_LOB_META_LENGTH - length ) ;

                  rc = _lobPieces.readFrom( pieces, length ) ;
                  if ( SDB_OK != rc )
                  {
                     PD_LOG( PDINFO, "Failed to read pieces info of Lob[%s]",
                             getOID().str().c_str() ) ;
                     goto error ;
                  }
               }

               rc = metaCache->cache( *(_dmsLobMeta*)_buf ) ;
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
            ossMemcpy( &_meta, meta, sizeof( _meta ) ) ;

            if ( _meta.hasPiecesInfo() )
            {
               INT32 length = _meta._piecesInfoNum *
                              (INT32)sizeof( _rtnLobPieces ) ;
               rc = _lobPieces.readFrom( (CHAR*)meta + DMS_LOB_META_LENGTH - length,
                                         length ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to read lob pieces info, rc:%d", rc ) ;
                  goto error ;
               }
            }
         }

         _accessInfo->unlock() ;
         accessInfoLocked = FALSE ;

         if ( !sectionMgr.isEmpty() )
         {
            for ( _utilSectionMgr::const_iterator it = sectionMgr.begin() ;
                  it != sectionMgr.end() ; it++ )
            {
               rc = lock( cb, it->begin(), it->length() ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to lock lob sections, rc:%d", rc ) ;
                  goto error ;
               }
            }
         }
      }
      else if ( SDB_LOB_MODE_CREATEONLY == _mode && _isMainShd )
      {
         _dmsLobMeta meta ;
         _opType |= MON_LOB_OP_PUT ;

         rc = rtnGetLobMetaData( _getRealCLName(),
                                 _oid, cb, meta,
                                 _su, _mbContext ) ;
         if ( SDB_FNE == rc )
         {
            rc = SDB_OK ;
         }
         else if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get lob meta data:%d", rc ) ;
            goto error ;
         }
         else
         {
            rc = SDB_FE ;
            PD_LOG( PDERROR, "Lob[%s] exists", _oid.str().c_str() ) ;
            goto error ;
         }
      }
      else if ( _isMainShd && SDB_LOB_MODE_REMOVE == _mode )
      {
         _opType |= MON_LOB_OP_DELETE ;

         rc = rtnGetLobMetaData( _getRealCLName(),
                                 _oid, cb, _meta,
                                 _su, _mbContext, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to get lob meta data:%d", rc ) ;
            goto error ;
         }
      }

      if ( _isMainShd )
      {
         rc = _meta2Obj( _metaObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to build meta obj:%d", rc ) ;
            goto error ;
         }

         if ( _pData )
         {
            if ( _flags & FLG_LOBOPEN_WITH_RETURNDATA )
            {
               UINT32 tmpLen = _metaObj.objsize() ;
               tmpLen = ossAlign4( tmpLen ) ;
               _pData -= tmpLen ;
               _dataLen += tmpLen ;
               ossMemcpy( (CHAR*)_pData, _metaObj.objdata(),
                          _metaObj.objsize() ) ;
               *data = _pData ;
               read = _dataLen ;

               _pData = NULL ;
               _dataLen = 0 ;
               goto done ;
            }
            else
            {
                ossMemmove( _buf, _pData, _dataLen ) ;
                _pData = _buf ;
            }
         }
      }

      /// we need to send back pagesize when this node
      ///  is not the main shard.
      *data = _metaObj.objdata() ;
      read = _metaObj.objsize() ;

   done:
      if ( accessInfoLocked )
      {
         _accessInfo->unlock() ;
      }
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__OPEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_GETLOBRTDETAIL, "_rtnContextShdOfLob::getLobRTDetail" )
   INT32 _rtnContextShdOfLob::getLobRTDetail( _pmdEDUCB *cb, BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      BSONObjBuilder builder ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_GETLOBRTDETAIL ) ;

      if ( !_isMainShd || NULL == _accessInfo )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Runtime detail is in main shard only!" ) ;
         goto error ;
      }

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
         builder.append( FIELD_NAME_CONTEXTID, contextID() ) ;
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
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_GETLOBRTDETAIL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_READV, "_rtnContextShdOfLob::readv" )
   INT32 _rtnContextShdOfLob::readv( const MsgLobTuple *tuples,
                                     UINT32 cnt,
                                     _pmdEDUCB *cb,
                                     const CHAR **data,
                                     UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_READV ) ;
      SDB_ASSERT( NULL != tuples && 0 < cnt, "can not be null" ) ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;
      UINT32 totalRead = 0 ;
      UINT32 i = 0 ;
      UINT32 len = 0 ;

      /// calc total buff size
      for ( i = 0 ; i < cnt ; ++i )
      {
         const MsgLobTuple &t = tuples[i] ;
         len += sizeof( MsgLobTuple ) ;
         len += t.columns.len ;
      }

      /// extend buf
      rc = _extendBuf( len ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Failed to extend buf[%u], rc: %d", len, rc ) ;
         goto error ;
      }

      /// read all tuples
      for ( i = 0; i < cnt; ++i )
      {
         UINT32 onceRead = 0 ;
         CHAR *dataOfTuple = NULL ;
         MsgLobTuple *rt = NULL ;
         const MsgLobTuple &t = tuples[i] ;

         dataOfTuple = _buf + totalRead ;
         rt = ( MsgLobTuple * )dataOfTuple ;
         dataOfTuple += sizeof( MsgLobTuple ) ;

         if ( _pData && _dataLen > 0 &&
              rt->columns.sequence == t.columns.sequence &&
              rt->columns.offset == t.columns.offset )
         {
            onceRead = rt->columns.len ;
         }
         else
         {
            rc = rtnReadLob( _getRealCLName(),
                             _oid, t.columns.sequence,
                             t.columns.offset, t.columns.len,
                             cb, dataOfTuple, onceRead,
                             _su, _mbContext ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to read lob[%s:%d], rc:%d",
                       _oid.str().c_str(), t.columns.sequence, rc ) ;
               goto error ;
            }
            rt->columns.sequence = t.columns.sequence ;
            rt->columns.offset = t.columns.offset ;
            rt->columns.len = onceRead ;
         }
         _pData = NULL ;
         _dataLen = 0 ;
         onceRead += sizeof( MsgLobTuple ) ; /// | tuple | data | tuple | data |
         totalRead += onceRead ;

         // monitor lob bytes read by client
         RTN_MON_LOB_BYTES_COUNT_INC( pMonAppCB, MON_LOB_READ_BYTES,
                                      rt->columns.len ) ;
      }

      *data = _buf ;
      read = totalRead ;
   done:
      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_READV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextShdOfLob::remove( UINT32 sequence,
                                      _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;

      rc = rtnRemoveLobPiece( _getRealCLName(),
                                _oid, sequence, cb,
                                _w, _dpsCB, _su, _mbContext ) ;
      if ( rc )
      {
         goto error ;
      }
done:
      return rc ;
error:
   goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_LOCK, "_rtnContextShdOfLob::lock" )
   INT32 _rtnContextShdOfLob::lock( _pmdEDUCB *cb,
                                    INT64 offset,
                                    INT64 length )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN locked = FALSE ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_LOCK ) ;
      rtnLobMetricsSubmitor submitor( cb, this ) ;

      SDB_ASSERT( NULL != _accessInfo, "_accessInfo is null" ) ;
      SDB_ASSERT( length > 0, "length <= 0" ) ;

      _accessInfo->lock() ;
      locked = TRUE ;

      rc = _accessInfo->lockSection( _mode, offset, length, contextID() ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to lock section[offset: %lld, length: %lld, "
                 "accessId: %lld],cl=%s,lob=%s,rc=%d", offset, length,
                 contextID(), _getRealCLName(), _oid.toString().c_str(), rc ) ;
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

      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB_LOCK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB_CLOSE, "_rtnContextShdOfLob::close" )
   INT32 _rtnContextShdOfLob::close( _pmdEDUCB *cb )
   {
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB_CLOSE ) ;

      _isOpened = FALSE ;
      _closeWithException = FALSE ;
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( NULL != _accessInfo )
      {
         sdbGetRTNCB()->getLobAccessManager()->releaseAccessPrivilege(
            _getRealCLName(), _oid, _mode, contextID() ) ;
         _accessInfo = NULL ;
      }

      if ( !_hasSubmitMetric )
      {
         // no matter error happen or not, we will still
         // submit the change from session to others
         if ( pMonAppCB && pMonAppCB->mondbcb )
         {
            // submit the change to database snapshot
            pMonAppCB->mondbcb->incMetrics( _totalDeltaMonApp ) ;
         }
         if ( pMonAppCB && pMonAppCB->getSvcTaskInfo() )
         {
            // submit the change to task snapshot
            pMonAppCB->getSvcTaskInfo()->incMetrics( _totalDeltaMonApp ) ;
         }
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
               _mbContext->mbStat()->_crudCB.incMetrics( _totalDeltaMonApp ) ;
            }
            if ( doLock )
            {
               _mbContext->mbUnlock() ;
            }
         }
         _hasSubmitMetric = TRUE ;
      }

      if ( _mbContext && _su )
      {
         // release mbContext
         _su->data()->releaseMBContext( _mbContext ) ;
         _mbContext = NULL ;
      }
      if ( _su && _dmsCB )
      {
         _dmsCB->suUnlock( _su->CSID() ) ;
         _su = NULL ;
      }

      PD_TRACE_EXIT( SDB__RTNCONTEXTSHDOFLOB_CLOSE ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK, "_rtnContextShdOfLob::_rollback" )
   INT32 _rtnContextShdOfLob::_rollback( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK ) ;
      UINT64 sucNum = 0 ;
      ossPoolSet<UINT32>::reverse_iterator itr = _written.rbegin() ;
      for ( ; itr != _written.rend(); ++itr )
      {
         if ( !sdbGetReplCB()->primaryIsMe() )
         {
            PD_LOG( PDERROR, "we are not primary any more, stop to rollback" ) ;
            break ;
         }

         rc = rtnRemoveLobPiece( _getRealCLName(),
                                 _oid, *itr, cb,
                                 _w, _dpsCB, _su, _mbContext, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove piece[%d] of lob, rc:%d",
                    *itr, rc ) ;
            if ( SDB_DMS_CS_DELETING == rc )
            {
               break ;
            }
            rc = SDB_OK ;
            /// do not goto error. try to rollback all pieces.
         }
         else
         {
            ++sucNum ;
         }
      }

      PD_LOG( PDEVENT, "rollback[%s]: we removed %d pieces, failed:%d",
              _oid.str().c_str(),
              _written.size(),  _written.size() - sucNum ) ;
      _written.clear() ;

      PD_TRACE_EXITRC( SDB__RTNCONTEXTSHDOFLOB__ROLLBACK, rc ) ;
      return rc ;
   }

   INT32 _rtnContextShdOfLob::_meta2Obj( bson::BSONObj &obj )
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         builder.append( FIELD_NAME_LOB_SIZE, (INT64)_meta._lobLen ) ;
         builder.append( FIELD_NAME_LOB_PAGE_SIZE,
                         NULL != _su ? _su->getLobPageSize() : 0 ) ;
         builder.append( FIELD_NAME_VERSION, (INT32)_meta._version ) ;
         builder.append( FIELD_NAME_LOB_CREATETIME, (INT64)_meta._createTime ) ;
         if ( 0 == _meta._modificationTime )
         {
            _meta._modificationTime = _meta._createTime ;
         }
         builder.append( FIELD_NAME_LOB_MODIFICATION_TIME,
                         (INT64)_meta._modificationTime ) ;
         builder.append( FIELD_NAME_LOB_FLAG, (INT32)_meta._flag ) ;
         builder.append( FIELD_NAME_LOB_PIECESINFONUM, _meta._piecesInfoNum ) ;
         if ( _meta.hasPiecesInfo() &&
              ( SDB_IS_LOBREADONLY_MODE(_mode) || SDB_HAS_LOBWRITE_MODE(_mode)
                || SDB_LOB_MODE_TRUNCATE == _mode ) )
         {
            SDB_ASSERT( !_lobPieces.empty(), "empty pieces info" ) ;
            BSONArray array ;
            rc = _lobPieces.saveTo( array ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to build pieces info array, rc=%d", rc ) ;
               goto error ;
            }

            builder.append( FIELD_NAME_LOB_PIECESINFO, array ) ;
         }
         obj = builder.obj() ;
      }
      catch ( std::exception& e )
      {
         rc = SDB_SYS ;
         PD_LOG( PDERROR, "unexpected exception happened: %s", e.what() ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnContextShdOfLob::_extendBuf( UINT32 len )
   {
      INT32 rc = SDB_OK ;
      if ( _bufLen < len )
      {
         len = ossRoundUpToMultipleX( len, _su->getLobPageSize() ) ;
         CHAR *buf = _buf ;
         _buf = ( CHAR * )SDB_OSS_REALLOC( _buf, len ) ;
         if ( NULL == _buf )
         {
            PD_LOG( PDERROR, "failed to allocate mem." ) ;
            rc = SDB_OOM ;
            _buf = buf ;
            goto error ;
         }
         _bufLen = len ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   const CHAR *_rtnContextShdOfLob::_getRealCLName()
   {
      if ( !_subCLName.empty() )
      {
         return _subCLName.c_str() ;
      }
      else
      {
         return _fullName.c_str() ;
      }
   }

   void _rtnContextShdOfLob::onSubmit( const monAppCB &delta )
   {
      _totalDeltaMonApp += delta ;
      getMonCB()->incMetrics( delta ) ;
   }

   void _rtnContextShdOfLob::_increaseLobOpCount( _pmdEDUCB *cb )
   {
      monAppCB *pMonAppCB = cb ? cb->getMonAppCB() : NULL ;

      if ( _isMainShd && _opType )
      {
         if ( _opType & MON_LOB_OP_GET )
         {
            RTN_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_GET, 1 ) ;
         }
         if ( _opType & MON_LOB_OP_PUT )
         {
            RTN_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_PUT, 1 ) ;
         }
         if ( _opType & MON_LOB_OP_DELETE )
         {
            RTN_MON_LOB_OP_COUNT_INC( pMonAppCB, MON_LOB_DELETE, 1 ) ;
         }
      }
   }

}

