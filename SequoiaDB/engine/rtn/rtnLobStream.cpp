/*******************************************************************************

   Copyright (C) 2011-2018 SequoiaDB Ltd.

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

   Source File Name = rtnLobStream.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLobStream.hpp"
#include "pmdEDU.hpp"
#include "msgDef.h"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "rtnContext.hpp"

using namespace bson ;

#define ALLOC_MEM( needLen, len, buf, rc ) \
        do\
        {\
           if ( needLen <= len )\
           {\
           }\
           else if ( NULL == buf )\
           {\
              buf = ( CHAR * )SDB_OSS_MALLOC( needLen ) ;\
              if ( NULL == buf )\
              {\
                 rc = SDB_OOM ;\
              }\
              len = needLen ;\
           }\
           else\
           {\
              SDB_OSS_FREE( buf ) ;\
              buf = NULL ;\
              len = 0 ;\
              buf = ( CHAR * )SDB_OSS_MALLOC( needLen ) ;\
              if ( NULL == buf )\
              {\
                 rc = SDB_OOM ;\
              }\
              len = needLen ;\
           }\
        } while ( FALSE )

namespace engine
{

   static OSS_INLINE CHAR* _rtnLobOpName( INT32 mode )
   {
      switch( mode )
      {
      case SDB_LOB_MODE_CREATEONLY:
         return "LOB CREATE" ;
      case SDB_LOB_MODE_READ:
         return "LOB READ" ;
      case SDB_LOB_MODE_SHAREREAD:
         return "LOB SHAREREAD" ;
      case SDB_LOB_MODE_WRITE:
         return "LOB WRITE" ;
      case (SDB_LOB_MODE_WRITE | SDB_LOB_MODE_SHAREREAD):
         return "LOB WRITE | LOB SHAREREAD" ;
      case SDB_LOB_MODE_REMOVE:
         return "LOB REMOVE" ;
      case SDB_LOB_MODE_TRUNCATE:
         return "LOB TRUNCATE" ;
      default:
         SDB_ASSERT( FALSE, "Invalid mode" ) ;
         return "LOB UNKNOWN" ;
      }
   }

   _rtnLobStream::_rtnLobStream()
   :_uniqueId( -1 ),
    _dpsCB( NULL ),
    _opened( FALSE ),
    _mode( 0 ),
    _flags( 0 ),
    _lobPageSz( DMS_DO_NOT_CREATE_LOB ),
    _logarithmic( 0 ),
    _offset( 0 ),
    _hasPiecesInfo( FALSE ),
    _wholeLobLocked( FALSE ),
    _truncated( FALSE )
   {
      ossMemset( _fullName, 0, DMS_COLLECTION_SPACE_NAME_SZ +
                               DMS_COLLECTION_NAME_SZ + 2 ) ;
   }

   _rtnLobStream::~_rtnLobStream()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_OPEN, "_rtnLobStream::open" )
   INT32 _rtnLobStream::open( const CHAR *fullName,
                              const bson::OID &oid,
                              INT32 mode,
                              INT32 flags,
                              _rtnContextBase *context,
                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_OPEN ) ;

      ossMemcpy( _fullName, fullName, ossStrlen( fullName ) ) ;
      ossMemcpy( &_oid, &oid, sizeof( oid ) ) ;
      _mode = mode ;
      _flags = flags ;

      if ( !SDB_IS_VALID_LOB_MODE( mode ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid LOB mode: %d", mode ) ;
         goto error ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         _meta._createTime = ossGetCurrentMilliseconds() ;
         _meta._modificationTime = _meta._createTime ;
      }

      rc = _prepare( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to prepare to open lob[%s]"
                 " in collection[%s], rc:%d",
                 oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

      if ( SDB_IS_LOBREADONLY_MODE(mode) )
      {
         rc = _open4Read( cb ) ;
         /// AUDIT
         PD_AUDIT_OP_WITHNAME( AUDIT_DQL, _rtnLobOpName(mode), AUDIT_OBJ_CL,
                               getFullName(), rc,
                               "OID:%s, Length:%llu, CreateTime:%llu, ModificationTime:%llu",
                               getOID().toString().c_str(),
                               _meta._lobLen, _meta._createTime, _meta._modificationTime ) ;
      }
      else if ( SDB_HAS_LOBWRITE_MODE(mode) )
      {
         rc = _open4Write( cb ) ;
      }
      else if ( SDB_LOB_MODE_CREATEONLY == mode )
      {
         rc = _open4Create( cb ) ;
      }
      else if ( SDB_LOB_MODE_REMOVE == mode )
      {
         rc = _open4Remove( cb ) ;
      }
      else if ( SDB_LOB_MODE_TRUNCATE == mode )
      {
         rc = _open4Truncate( cb ) ;
      }
      else
      {
         PD_LOG( PDERROR, "unknown open mode:%d", mode ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob[%s], rc:%d",
                 oid.str().c_str(), rc ) ;
         goto error ;
      }

      /// get page size, must call before _meta2Obj
      rc = _getLobPageSize( _lobPageSz ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get page size of lob, rc:%d", rc ) ;
         goto error ;
      }

      if ( !ossIsPowerOf2( _lobPageSz, &_logarithmic ) )
      {
         PD_LOG( PDERROR, "Invalid page size:%d, it should be a power of 2",
                 _lobPageSz ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      /// if has context, need copy metaObj and data to context
      if ( context )
      {
         rc = _meta2Obj( _metaObj ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to get meta obj, rc:%d", rc ) ;
            goto error ;
         }
         rc = context->append( _metaObj ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to append meta data, rc:%d", rc ) ;
            goto error ;
         }
         /// add the data
         if ( _pool.getLastDataSize() > 0 &&
              ( _flags & FLG_LOBOPEN_WITH_RETURNDATA ) )
         {
            UINT32 readLen = 0 ;
            UINT32 poolSize = _pool.getLastDataSize() ;
            rc = _readFromPool( poolSize, context, cb, readLen ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to read from pool, rc:%d", rc ) ;
               goto error ;
            }
            _offset += readLen ;
         }
      }

      rc = _lw.init( _lobPageSz,
                     _meta._version >= DMS_LOB_META_MERGE_DATA_VERSION ?
                     TRUE : FALSE,
                     SDB_HAS_LOBWRITE_MODE(mode) ? FALSE : TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init stream window, rc:%d", rc ) ;
         goto error ;
      }

      _opened = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_OPEN, rc ) ;
      return rc ;
   error:
      closeWithException( cb ) ;
      goto done ;
   }

   INT32 _rtnLobStream::_meta2Obj( bson::BSONObj& obj ) const
   {
      INT32 rc = SDB_OK ;

      try
      {
         BSONObjBuilder builder ;
         /// we can get nothing when mode is create.
         builder.appendOID( FIELD_NAME_LOB_OID, (OID *)&_oid ) ;
         builder.append( FIELD_NAME_LOB_SIZE, _meta._lobLen ) ;
         builder.append( FIELD_NAME_LOB_PAGE_SIZE, _lobPageSz ) ;
         builder.append( FIELD_NAME_VERSION, (INT32)_meta._version ) ;
         builder.append( FIELD_NAME_LOB_CREATETIME, (INT64)_meta._createTime ) ;
         builder.append( FIELD_NAME_LOB_MODIFICATION_TIME, (INT64)_meta._modificationTime ) ;
         builder.append( FIELD_NAME_LOB_FLAG, (INT32)_meta._flag ) ;
         builder.append( FIELD_NAME_LOB_PIECESINFONUM, _meta._piecesInfoNum ) ;
         if ( _meta.hasPiecesInfo() )
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

   UINT32 _rtnLobStream::_getSequence( INT64 offset ) const
   {
      SDB_ASSERT( isOpened(), "not opened" ) ;

      return RTN_LOB_GET_SEQUENCE( offset,
                                   _meta._version >= DMS_LOB_META_MERGE_DATA_VERSION,
                                   _logarithmic ) ;
   }

   INT32 _rtnLobStream::getMetaData( bson::BSONObj &meta )
   {
      INT32 rc = SDB_OK ;

      rc = _meta2Obj( meta ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_CLOSE, "_rtnLobStream::close" )
   INT32 _rtnLobStream::close( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_CLOSE ) ;
      if ( !isOpened() )
      {
         goto done ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == _mode || SDB_HAS_LOBWRITE_MODE( _mode ) )
      {
         rc = _writeLobMeta( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close for create:%d", rc ) ;
            goto error ;
         }
      }
      else if ( SDB_LOB_MODE_REMOVE == _mode )
      {
         RTN_LOB_TUPLES tuples ;
         _rtnLobTuple tuple( 0, DMS_LOB_META_SEQUENCE, 0, NULL ) ;
         tuples.push_back( tuple ) ;
         rc = _removev( tuples, cb ) ;
         PD_AUDIT_OP_WITHNAME( AUDIT_DML, "LOB REMOVE", AUDIT_OBJ_CL,
                               getFullName(), rc, "OID:%s, Meta:%s",
                               getOID().toString().c_str(),
                               _metaObj.toString().c_str() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove meta data of lob:%d", rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "lob [%s] is removed",
                 getOID().str().c_str() ) ;
      }
      else if ( SDB_LOB_MODE_TRUNCATE == _mode && _truncated )
      {
         rc = _writeLobMeta( cb, FALSE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close for truncate:%d", rc ) ;
            goto error ;
         }
      }

      rc = _close( cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to close lob:%d", rc ) ;
         goto error ;
      }
      _opened = FALSE ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_CLOSE, rc ) ;
      return rc ;
   error:
      closeWithException( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_CLOSEWITHEXCEPTION, "_rtnLobStream::closeWithException" )
   INT32 _rtnLobStream::closeWithException( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_CLOSEWITHEXCEPTION ) ;
      if ( !isOpened() )
      {
         goto done ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == _mode )
      {
         PD_LOG( PDERROR, "Lob[%s] is closed with exception, rollback",
                 getOID().str().c_str() ) ;
         rc = _rollback( cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to rollback lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }
      }
      else if ( SDB_HAS_LOBWRITE_MODE(_mode) )
      {
         PD_LOG( PDERROR, "Lob[%s] is closed with exception, write meta data",
                 getOID().str().c_str() ) ;
         rc = _writeLobMeta( cb, TRUE ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write meta data of lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }
      }
      else if ( SDB_LOB_MODE_REMOVE == _mode )
      {
         PD_LOG( PDERROR, "Lob[%s] is closed with exception, invalidate meta data",
                 getOID().str().c_str() ) ;
         rc = _queryAndInvalidateMetaData( cb, _meta ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to invalidate lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }
      }
      else
      {
         PD_LOG( PDWARNING, "Lob[%s] is closed with exception, mode:0x%08x",
                 getOID().str().c_str(), _mode ) ;
      }

      _opened = FALSE ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_CLOSEWITHEXCEPTION, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT64 _rtnLobStream::_calculateLockedLobLen( INT64 lobLen,
                                                BOOLEAN wholeLobLocked,
                                                INT64 lockedEnd )
   {
      if ( wholeLobLocked )
      {
         return lobLen ;
      }

      return lobLen < lockedEnd ? lobLen : lockedEnd ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_WRITE, "_rtnLobStream::write" )
   INT32 _rtnLobStream::write ( UINT32 len,
                                const CHAR *buf,
                                _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_WRITE ) ;
      RTN_LOB_TUPLES tuples ;

      if ( !isOpened() )
      {
         PD_LOG( PDERROR, "lob[%s] is not opened yet",
                 _oid.str().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( SDB_LOB_MODE_CREATEONLY != _mode && !SDB_HAS_LOBWRITE_MODE(_mode) )
      {
         PD_LOG( PDERROR, "open mode[%d] does not support this operation",
                 _mode ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_IS_LOBSREADWRITE_MODE( _mode ) )
      {
         _pool.clear() ;
      }

      if ( SDB_HAS_LOBWRITE_MODE(_mode) && !_wholeLobLocked )
      {
         if ( !_sectionMgr.isEmpty() )
         {
            if ( !_sectionMgr.isContain( _offset, len ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Section is not locked before write:"
                       "offset=%lld,len=%d,mode=%d,rc=%d", _offset, len,
                       _mode, rc ) ;
               goto error ;
            }
         }
         else
         {
            // lock the whole lob
            rc = lock( cb, 0, OSS_SINT64_MAX ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to lock the whole lob, rc=%d", rc ) ;
               goto error ;
            }
         }
      }

      if ( SDB_HAS_LOBWRITE_MODE(_mode) && !_hasPiecesInfo )
      {
         UINT32 piece = _getSequence( _meta._lobLen - 1 ) ;
         rc = _lobPieces.addPieces( _rtnLobPieces(0, piece) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to add pieces, rc=%d", rc ) ;
            goto error ;
         }
         _hasPiecesInfo = TRUE ;
      }

      // re-array the data and try to get a complete piece.
      rc = _lw.prepare4Write( _offset, len, buf ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to add piece to window, rc:%d", rc ) ;
         goto error ;
      }

      // if we update offset after write,
      // some data will not be removed when rollback
      _offset += len ;
      // update lobLen immediately,
      // for _offset can be set to front position by seek
      _meta._lobLen = OSS_MAX( _meta._lobLen, _offset ) ;

      while ( _lw.getNextWriteSequences( tuples )  )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_INTERRUPT ;
            goto error ;
         }

         rc = _writeOrUpdateV( tuples, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
            goto error ;
         }

         tuples.clear() ;
      }

      _lw.cacheLastDataOrClearCache() ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_WRITE, rc ) ;
      return rc ;
   error:
      closeWithException( cb ) ;
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_READ, "_rtnLobStream::read" )
   INT32 _rtnLobStream::read( UINT32 len,
                              _rtnContextBase *context,
                              _pmdEDUCB *cb,
                              UINT32 &read )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_READ ) ;
      UINT32 readLen = 0 ;
      INT64 lockedEnd = -1 ;
      RTN_LOB_TUPLES tuples ;

      SDB_ASSERT( _meta.isDone(), "lob has not been completed yet" ) ;

      if ( !isOpened() )
      {
         PD_LOG( PDERROR, "lob[%s] is not opened yet",
                 _oid.str().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !SDB_IS_LOBREADONLY_MODE(_mode)
           && !SDB_IS_LOBSREADWRITE_MODE(_mode) )
      {
         PD_LOG( PDERROR, "open mode[%d] does not support this operation",
                 _mode ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( 0 == len )
      {
         goto done ;
         read = 0 ;
      }

      if ( _meta._lobLen <= _offset )
      {
         rc = SDB_EOF ;
         goto error ;
      }
      else if ( _offset + len > _meta._lobLen )
      {
         len = _meta._lobLen - _offset ;
      }

      if ( SDB_IS_LOBSREADWRITE_MODE(_mode) )
      {
         _rtnLobTuple tuple ;
         if ( _lw.getCachedData( tuple ) )
         {
            rc = _writeOrUpdate( tuple, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to clean lob[%s] write cache, rc:%d",
                       _oid.str().c_str(), rc ) ;
                goto error ;
            }
         }
      }

      if ( (SDB_LOB_MODE_SHAREREAD == _mode || SDB_IS_LOBSREADWRITE_MODE(_mode))
           && !_wholeLobLocked )
      {
         if ( !_sectionMgr.isEmpty() )
         {
            if ( !_sectionMgr.isContain( _offset, len, FALSE, &lockedEnd ) )
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Section is not locked before write:"
                       "offset=%lld,len=%d,mode=%d,rc=%d", _offset, len,
                       _mode, rc ) ;
               goto error ;
            }
         }
         else
         {
            // lock the whole lob
            rc = lock( cb, 0, OSS_SINT64_MAX ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to lock the whole lob, rc=%d", rc ) ;
               goto error ;
            }
         }
      }

      /// data may be cached.
      if ( _pool.match( _offset ) )
      {
         rc = _readFromPool( len, context, cb, readLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to read data from pool:%d", rc ) ;
            goto error ;
         }

         _offset += readLen ;
         goto done ;
      }

      /// clear cache when we can not get data from it.
      _pool.clear() ;

      if ( SDB_LOB_MODE_SHAREREAD == _mode
           || SDB_IS_LOBSREADWRITE_MODE(_mode) )
      {
         SDB_ASSERT( ( _wholeLobLocked && -1 == lockedEnd )
                     || ( !_wholeLobLocked && -1 != lockedEnd ), "lock check" ) ;
         INT64 lockedLobLen = _calculateLockedLobLen( _meta._lobLen,
                                                      _wholeLobLocked,
                                                      lockedEnd ) ;
         rc = _lw.prepare4Read( lockedLobLen, _offset, len, tuples ) ;
      }
      else
      {
         /// reset the read len of a suitable value
         rc = _lw.prepare4Read( _meta._lobLen, _offset, len, tuples ) ;
      }

      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to prepare to read:%d", rc ) ;
         goto error ;
      }

      rc = _readv( tuples, cb, _hasPiecesInfo ? &_lobPieces : NULL ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
         goto error ;
      }

      rc = _readFromPool( len, context, cb, readLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to read data from pool:%d", rc ) ;
         goto error ;
      }

      _offset += len ;
      read = readLen ;
   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_READ, rc ) ;
      return rc ;
   error:
      closeWithException( cb ) ;
      goto done ;
   }

   INT32 _rtnLobStream::getRTDetail( _pmdEDUCB *cb, bson::BSONObj &detail )
   {
      INT32 rc = SDB_OK ;
      if ( !isOpened() )
      {
         PD_LOG( PDERROR, "Lob[%s] is not opened yet", _oid.str().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = _getRTDetail( cb, detail ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to get lob detail:%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_LOCK, "_rtnLobStream::lock" )
   INT32 _rtnLobStream::lock( _pmdEDUCB *cb, INT64 offset, INT64 length )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_LOCK ) ;

      if ( offset < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid lock offset:%lld, rc=%d", offset, rc ) ;
         goto error ;
      }

      if ( 0 == length || length < -1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid lock length:%lld, rc=%d", length, rc ) ;
         goto error ;
      }

      if ( length > 0 && ( OSS_SINT64_MAX - offset - length ) < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Lock length overflowed, offset:%lld, length:%lld, rc=%d",
                 offset, length, rc ) ;
         goto error ;
      }

      // endlessly lock from offset
      if ( -1 == length )
      {
         // subtract offset to avoid section.end() overflow
         length = OSS_SINT64_MAX - offset ;
      }

      if ( _wholeLobLocked )
      {
         goto done ;
      }

      if ( _sectionMgr.isContain( offset, length ) )
      {
         // already locked
         goto done ;
      }

      rc = _lock( cb, offset, length ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to lock LOB[%s] in (offset:%lld,length:%lld),"
                 " rc=%d", _oid.str().c_str(), offset, length, rc ) ;
         goto error ;
      }

      rc = _sectionMgr.addSection( offset, length ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to add section:offset=%lld,"
                   "length=%lld,rc=%d", offset, length, rc ) ;

      if ( OSS_SINT64_MAX == length && 0 == offset )
      {
         _wholeLobLocked = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_LOCK, rc ) ;
      return rc ;
   error:
      if ( SDB_OP_INCOMPLETE == rc )
      {
         // if lock section can't be completely done,
         // we should kill the context to avoid incomplete lock sections
         closeWithException( cb ) ;
      }
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_SEEK, "_rtnLobStream::seek" )
   INT32 _rtnLobStream::seek( SINT64 offset, _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_SEEK ) ;

      SDB_ASSERT( offset >= 0, "invalid offset" ) ;

      if ( !isOpened() )
      {
         PD_LOG( PDERROR, "lob[%s] is not opened yet",
                 _oid.str().c_str() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !SDB_IS_LOBREADONLY_MODE(_mode) && !SDB_HAS_LOBWRITE_MODE(_mode)
           && SDB_LOB_MODE_CREATEONLY != _mode )
      {
         PD_LOG( PDERROR, "open mode[%d] does not support this operation",
                 _mode ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( offset >= _meta._lobLen && SDB_IS_LOBREADONLY_MODE(_mode) )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( SDB_LOB_MODE_CREATEONLY == _mode || SDB_HAS_LOBWRITE_MODE(_mode) )
      {
         if ( !_lw.continuous( offset ) )
         {
            _rtnLobTuple tuple ;
            // write last data
            if ( _lw.getCachedData( tuple ) )
            {
               rc = _writeOrUpdate( tuple, cb ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "failed to write lob[%s], rc:%d",
                          _oid.str().c_str(), rc ) ;
                   goto error ;
               }
            }

            if ( !_hasPiecesInfo )
            {
               UINT32 piece = _getSequence( _meta._lobLen - 1 ) ;
               rc = _lobPieces.addPieces( _rtnLobPieces(0, piece) ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to add pieces, rc=%d", rc ) ;
                  goto error ;
               }
               _hasPiecesInfo = TRUE ;
            }
         }
      }

      _offset = offset ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_SEEK, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_TRUNCATE, "_rtnLobStream::truncate" )
   INT32 _rtnLobStream::truncate( INT64 len,
                                  _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_TRUNCATE ) ;
      SDB_ASSERT( ( SDB_LOB_MODE_REMOVE == _mode && 0 == len ) ||
                  SDB_LOB_MODE_TRUNCATE == _mode,
                  "invalid mode or len" ) ;

      RTN_LOB_TUPLES tuples ;
      UINT32 lastPiece = 0 ;
      UINT32 startPiece = 0 ;
      UINT32 oneLoopNum = 0 ;

      if ( len < 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid truncate length: %lld", len ) ;
         goto error ;
      }

      if ( len >= _meta._lobLen )
      {
         // nothing to do
         goto done ;
      }

      lastPiece = ( _meta._lobLen > 0 ) ?
                  _getSequence( _meta._lobLen - 1 ) :
                  _getSequence( 0 ) ;
      startPiece = ( len > 0 ) ?
                   _getSequence( len - 1 ) :
                   _getSequence( 0 ) ;

      // do not remove start piece
      for ( UINT32 i = lastPiece ; i > startPiece ; i-- )
      {
         tuples.push_back( _rtnLobTuple( 0, i, 0, NULL ) ) ;
         ++oneLoopNum ;

         if ( 1000 == oneLoopNum )
         {
            rc = _removev( tuples, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to remove lob pieces, rc=%d", rc ) ;
               goto error ;
            }

            oneLoopNum = 0 ;
            tuples.clear() ;
         }
      }

      if ( !tuples.empty() )
      {
         rc = _removev( tuples, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to remove lob pieces, rc=%d", rc ) ;
            goto error ;
         }
         tuples.clear() ;
      }

      if ( SDB_LOB_MODE_TRUNCATE == _mode )
      {
         if ( _hasPiecesInfo && startPiece < lastPiece )
         {
            rc = _lobPieces.delPieces( _rtnLobPieces( startPiece + 1, lastPiece ) ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to delete pieces, rc=%d", rc ) ;
               goto error ;
            }
         }

         _meta._lobLen = len ;
         _truncated = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_TRUNCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_WRITEORUPDATE, "_rtnLobStream::_writeOrUpdate" )
   INT32 _rtnLobStream::_writeOrUpdate( const _rtnLobTuple &tuple,
                                        _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_WRITEORUPDATE ) ;

      if ( !_hasPiecesInfo )
      {
         rc = _write( tuple, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write lob piece, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         if ( !_lobPieces.hasPiece( tuple.tuple.columns.sequence ) )
         {
            BOOLEAN orUpdate = SDB_HAS_LOBWRITE_MODE(_mode) ? TRUE : FALSE ;
            rc = _write( tuple, cb, orUpdate ) ;
            if ( SDB_OK == rc )
            {
               rc = _lobPieces.addPiece( tuple.tuple.columns.sequence ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to add piece, rc=%d", rc ) ;
                  goto error ;
               }

               if ( _lobPieces.requiredMem() > DMS_LOB_META_PIECESINFO_MAX_LEN )
               {
                  rc = SDB_LOB_PIECESINFO_OVERFLOW ;
                  PD_LOG( PDERROR, "LOB pieces info require memory more than "\
                          "%d bytes, section num=%d, piecesInfo=%s",
                          DMS_LOB_META_PIECESINFO_MAX_LEN,
                          _lobPieces.sectionNum(), _lobPieces.toString().c_str() ) ;
                  goto error ;
               }
            }
            else
            {
               PD_LOG( PDERROR, "Failed to write lob piece, rc=%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            rc = _update( tuple, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to update lob piece, rc=%d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_WRITEORUPDATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM_WRITEORUPDATEV, "_rtnLobStream::_writeOrUpdateV" )
   INT32 _rtnLobStream::_writeOrUpdateV( RTN_LOB_TUPLES &tuples,
                                              _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM_WRITEORUPDATEV ) ;

      if ( !_hasPiecesInfo )
      {
         rc = _writev( tuples, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write lob pieces, rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         RTN_LOB_TUPLES updateTuples ;

         for ( RTN_LOB_TUPLES::iterator iter = tuples.begin() ;
               iter != tuples.end() ; )
         {
            _rtnLobTuple& tuple = *iter ;
            if ( _lobPieces.hasPiece( tuple.tuple.columns.sequence ) )
            {
               updateTuples.push_back( tuple ) ;
               iter = tuples.erase( iter ) ;
            }
            else
            {
               ++iter ;
            }
         }

         if ( !tuples.empty() )
         {
            BOOLEAN orUpdate = SDB_HAS_LOBWRITE_MODE(_mode) ? TRUE : FALSE ;
            rc = _writev( tuples, cb, orUpdate ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to write lob pieces, rc=%d", rc ) ;
               goto error ;
            }

            for ( RTN_LOB_TUPLES::const_iterator iter = tuples.begin() ;
                  iter != tuples.end() ; iter++ )
            {
               const _rtnLobTuple& tuple = *iter ;
               rc = _lobPieces.addPiece( tuple.tuple.columns.sequence ) ;
               if ( SDB_OK != rc )
               {
                  PD_LOG( PDERROR, "Failed to add piece, rc=%d", rc ) ;
                  goto error ;
               }

               if ( _lobPieces.requiredMem() > DMS_LOB_META_PIECESINFO_MAX_LEN )
               {
                  rc = SDB_LOB_PIECESINFO_OVERFLOW ;
                  PD_LOG( PDERROR, "LOB pieces info require memory more than "\
                          "%d bytes, section num=%d, piecesInfo=%s",
                          DMS_LOB_META_PIECESINFO_MAX_LEN,
                          _lobPieces.sectionNum(), _lobPieces.toString().c_str() ) ;
                  goto error ;
               }
            }
         }

         if ( !updateTuples.empty() )
         {
            rc = _updatev( updateTuples, cb ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "Failed to update lob pieces, rc=%d", rc ) ;
               goto error ;
            }
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM_WRITEORUPDATEV, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__READFROMPOOL, "_rtnLobStream::_readFromPool" )
   INT32 _rtnLobStream::_readFromPool( UINT32 len,
                                       _rtnContextBase *context,
                                       _pmdEDUCB *cb,
                                       UINT32 &readLen )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__READFROMPOOL ) ;
      const CHAR *data = NULL ;
      _MsgLobTuple tuple ;
      tuple.columns.len = len <= _pool.getLastDataSize() ?
                          len : _pool.getLastDataSize() ;
      tuple.columns.offset = _offset ;
      tuple.columns.sequence = 0 ; /// it is useless column now.
      UINT32 needLen = tuple.columns.len ;

      SDB_ASSERT( _pool.match( _offset ), "impossible" ) ;

      rc = context->appendObjs( tuple.data, sizeof( tuple.data ), 0 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to append data to context%d", rc ) ;
         goto error ;
      }

      while ( 0 < needLen )
      {
         UINT32 dataLen = 0 ;
         if( _pool.next( needLen, &data, dataLen ) )
         {
            needLen -= dataLen ;
            readLen += dataLen ;
            rc = context->appendObjs( data, dataLen, 0, FALSE ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to append data to context%d", rc ) ;
               goto error ;
            }
         }
         else
         {
            break ;
         }
      }

      SDB_ASSERT( readLen == tuple.columns.len, "impossible" ) ;
      rc = context->appendObjs( NULL, 0, 1 ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to append data to context, rc:%d", rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__READFROMPOOL, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__OPEN4CREATE, "_rtnLobStream::_open4Create" )
   INT32 _rtnLobStream::_open4Create( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__OPEN4CREATE ) ;
      BOOLEAN isNew = TRUE ;
      rc = _ensureLob( cb, _meta, isNew ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob[%s] in collection[%s], rc:%d",
                 _oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

      if ( !isNew )
      {
         PD_LOG( PDERROR, "Lob[%s] exists in collection[%s]",
                 _oid.str().c_str(), _fullName ) ;
         rc = SDB_FE ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "Lob[%s] in [%s] is created, wait to be completed",
              getOID().str().c_str(), _fullName ) ;

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__OPEN4CREATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__OPEN4READ, "_rtnLobStream::_open4Read" )
   INT32 _rtnLobStream::_open4Read( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__OPEN4READ ) ;

      rc = _queryLobMeta( cb, _meta, FALSE, &_lobPieces ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob[%s] in collection[%s], rc:%d",
                 _oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

      if ( _lobPieces.sectionNum() > 0 )
      {
         _hasPiecesInfo = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__OPEN4READ, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__OPEN4WRITE, "_rtnLobStream::_open4Write" )
   INT32 _rtnLobStream::_open4Write( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__OPEN4WRITE ) ;

      rc = _queryLobMeta( cb, _meta, FALSE, &_lobPieces ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob[%s] in collection[%s], rc:%d",
                 _oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

      if ( _lobPieces.sectionNum() > 0 )
      {
         _hasPiecesInfo = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__OPEN4WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__OPEN4REMOVE, "_rtnLobStream::_open4Remove" )
   INT32 _rtnLobStream::_open4Remove( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__OPEN4REMOVE ) ;

      rc = _queryLobMeta( cb, _meta, TRUE ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open lob[%s] in collection[%s], rc:%d",
                 _oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__OPEN4REMOVE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__OPEN4TRUNCATE, "_rtnLobStream::_open4Truncate" )
   INT32 _rtnLobStream::_open4Truncate( _pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__OPEN4TRUNCATE ) ;

      rc = _queryLobMeta( cb, _meta, FALSE, &_lobPieces ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to open lob[%s] in collection[%s], rc:%d",
                 _oid.str().c_str(), _fullName, rc ) ;
         goto error ;
      }

      if ( _lobPieces.sectionNum() > 0 )
      {
         _hasPiecesInfo = TRUE ;
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__OPEN4TRUNCATE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBSTREAM__WRITELOBMETA, "_rtnLobStream::_writeLobMeta" )
   INT32 _rtnLobStream::_writeLobMeta( _pmdEDUCB *cb, BOOLEAN withData )
   {
      INT32 rc = SDB_OK ;
      CHAR* buf = NULL ;
      INT32 piecesInfoSize = 0 ;
      _rtnLobTuple tuple ;
      PD_TRACE_ENTRY( SDB_RTNLOBSTREAM__WRITELOBMETA ) ;

      SDB_ASSERT( SDB_LOB_MODE_CREATEONLY == _mode
                  || SDB_HAS_LOBWRITE_MODE(_mode)
                  || SDB_LOB_MODE_TRUNCATE == _mode, "incorrect mode" ) ;

      // write last data
      if ( withData && _lw.getCachedData( tuple ) )
      {
         rc = _writeOrUpdate( tuple, cb ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to write lob[%s], rc:%d",
                    _oid.str().c_str(), rc ) ;
             goto error ;
         }
      }

      if ( _hasPiecesInfo )
      {
         piecesInfoSize = _lobPieces.requiredMem() ;
         if( piecesInfoSize > DMS_LOB_META_PIECESINFO_MAX_LEN )
         {
            PD_LOG( PDERROR, "LOB pieces info require memory more than %d "
                    "bytes, section num=%d, piecesInfo=%s",
                    DMS_LOB_META_PIECESINFO_MAX_LEN,
                    _lobPieces.sectionNum(), _lobPieces.toString().c_str() ) ;
            rc = SDB_LOB_PIECESINFO_OVERFLOW ;
            goto error ;
         }

         if ( piecesInfoSize > 0 && _lobPieces.sectionNum() == 1 )
         {
            UINT32 last = _getSequence( _meta._lobLen - 1 ) ;
            _rtnLobPieces pieces = _lobPieces.getSection( 0 ) ;
            if ( 0 == pieces.first && last == pieces.last )
            {
               // no skipped piece, so no need to save pieces info
               piecesInfoSize= 0 ;
               _meta._piecesInfoNum = 0 ;
               OSS_BIT_CLEAR(_meta._flag, DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ;
            }
         }
      }
      else
      {
         _meta._piecesInfoNum = 0 ;
         OSS_BIT_CLEAR(_meta._flag, DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ;
      }

      // write meta data
      // _meta._lobLen is already updated
      _meta._modificationTime = ossGetCurrentMilliseconds() ;
      _meta._status = DMS_LOB_COMPLETE ;
      if ( withData && _lw.getMetaPageData( tuple ) )
      {
         if ( piecesInfoSize > 0 )
         {
            CHAR* piecesInfoBuf = (CHAR*)tuple.data + DMS_LOB_META_LENGTH
                                  - piecesInfoSize ;

            rc = _lobPieces.saveTo( piecesInfoBuf, piecesInfoSize ) ;
            if ( SDB_OK != rc )
            {
               PD_LOG( PDERROR, "failed to save lob pieces info, rc=%d", rc ) ;
               goto error ;
            }

            _meta._piecesInfoNum = _lobPieces.sectionNum() ;
            _meta._flag |= DMS_LOB_META_FLAG_PIECESINFO_INSIDE ;
         }

         ossMemcpy( (CHAR*)tuple.data, (const CHAR*)&_meta,
                     sizeof( _meta ) ) ;
      }
      else if ( piecesInfoSize > 0 )
      {
         SDB_ASSERT( NULL == buf, "impossible" ) ;

         buf = (CHAR*) SDB_OSS_MALLOC( DMS_LOB_META_LENGTH ) ;
         if ( NULL == buf )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "failed to alloc memory for lob pieces info, rc=%d", rc ) ;
            goto error ;
         }

         _meta._piecesInfoNum = _lobPieces.sectionNum() ;
         _meta._flag |= DMS_LOB_META_FLAG_PIECESINFO_INSIDE ;

         ossMemcpy( buf, (const CHAR*)&_meta, sizeof( _meta ) ) ;

         CHAR* piecesInfoBuf = buf + DMS_LOB_META_LENGTH
                               - piecesInfoSize ;

         rc = _lobPieces.saveTo( piecesInfoBuf, piecesInfoSize ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to save lob pieces info, rc=%d", rc ) ;
            goto error ;
         }

         tuple.tuple.columns.len = DMS_LOB_META_LENGTH ;
         tuple.tuple.columns.sequence = DMS_LOB_META_SEQUENCE ;
         tuple.tuple.columns.offset = 0 ;
         tuple.data = ( const CHAR* )buf ;
      }
      else
      {
         SDB_ASSERT( !_meta.hasPiecesInfo(), "invalid piecesinfo flag" ) ;
         SDB_ASSERT( 0 == _meta._piecesInfoNum, "invalid piecesinfo num" ) ;
         tuple.tuple.columns.len = sizeof( _meta ) ;
         tuple.tuple.columns.sequence = DMS_LOB_META_SEQUENCE ;
         tuple.tuple.columns.offset = 0 ;
         tuple.data = ( const CHAR* )&_meta ;
      }

      rc = _completeLob( tuple, cb ) ;
      PD_AUDIT_OP_WITHNAME( AUDIT_DML,
                            _rtnLobOpName(_mode),
                            AUDIT_OBJ_CL,
                            getFullName(), rc, "OID:%s, Length:%llu",
                            getOID().toString().c_str(),
                            _meta._lobLen ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to complete lob:%d", rc ) ;
         goto error ;
      }

      PD_LOG( PDDEBUG, "lob [%s] is closed, len:%lld",
              getOID().str().c_str(), _offset ) ;

   done:
      SAFE_OSS_FREE( buf ) ;
      PD_TRACE_EXITRC( SDB_RTNLOBSTREAM__WRITELOBMETA, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

