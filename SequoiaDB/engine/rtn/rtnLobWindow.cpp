/*******************************************************************************

   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobWindow.cpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/31/2014  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnLobWindow.hpp"
#include "rtnTrace.hpp"
#include "pdTrace.hpp"

namespace engine
{
const UINT32 RTN_MIN_READ_LEN = DMS_PAGE_SIZE512K ;
const UINT32 RTN_MAX_READ_LEN = DMS_PAGE_SIZE128K * 512 ;      /// 64MB

   _rtnLobWindow::_rtnLobWindow()
   :_pageSize( DMS_DO_NOT_CREATE_LOB ),
    _logarithmic( 0 ),
    _mergeMeta( FALSE ),
    _metaPageDataCached( FALSE ),
    _curOffset( 0 ),
    _pool( NULL ),
    _cachedSz( 0 ),
    _metaSize( 0 )
   {
   }

   _rtnLobWindow::~_rtnLobWindow()
   {
      if ( NULL != _pool )
      {
         SDB_OSS_FREE( _pool ) ;
         _pool = NULL ; 
      }
   }

   BOOLEAN _rtnLobWindow::_hasData() const
   {
      if ( _cachedSz > 0 )
      {
         return TRUE ;
      }

      if ( !_writeData.empty() )
      {
         return TRUE ;
      }

      return FALSE ;
   }

   UINT32 _rtnLobWindow::_getCurDataPageSize() const
   {
      if ( !_mergeMeta || _curOffset >= _pageSize - DMS_LOB_META_LENGTH )
      {
         return _pageSize ;
      }
      return _pageSize - DMS_LOB_META_LENGTH ;
   }

   INT32 _rtnLobWindow::init( INT32 pageSize,
                              BOOLEAN mergeMeta,
                              BOOLEAN metaPageDataCached )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( DMS_DO_NOT_CREATE_LOB < pageSize,
                  "invalid arguments" ) ;

      SDB_ASSERT( _writeData.empty(), "impossible" ) ;

      if ( !ossIsPowerOf2( pageSize, &_logarithmic ) )
      {
         PD_LOG( PDERROR, "Invalid page size:%d, it should be a power of 2",
                 pageSize ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      _pool = ( CHAR * )SDB_OSS_MALLOC( pageSize * 2 ) ;
      if ( NULL == _pool )
      {
         PD_LOG( PDERROR, "failed to allocate mem." ) ;
         rc = SDB_OOM ;
         goto error ;
      }

      _pageSize = pageSize ;
      _mergeMeta = mergeMeta ;
      _metaPageDataCached = metaPageDataCached ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnLobWindow::continuous( INT64 offset ) const
   {
      if ( offset == _curOffset + _cachedSz )
      {
         return TRUE ;
      }
      else
      {
         return FALSE ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_PREPARE4WRITE, "_rtnLobWindow::prepare4Write" )
   INT32 _rtnLobWindow::prepare4Write( INT64 offset, UINT32 len,
                                       const CHAR *data )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_PREPARE4WRITE ) ;
      SDB_ASSERT( 0 <= offset && NULL != data, "invalid arguments" ) ;
      SDB_ASSERT( _writeData.empty(), "the last write has not been done" ) ;

      if ( !continuous( offset ) )
      {
         if ( _hasData() )
         {
            PD_LOG( PDERROR, "Invalid offset:%lld, current offset:%lld"
                    ", seek happened when still has data",
                    offset, _curOffset ) ;
            rc = SDB_INVALIDARG ;
            goto error ;
         }
         else
         {
            _curOffset = offset ;
         }
      }

      if ( _metaPageDataCached && _mergeMeta &&
           _curOffset < _pageSize - DMS_LOB_META_LENGTH )
      {
         UINT32 lastLen = _pageSize - DMS_LOB_META_LENGTH - _curOffset ;
         CHAR *pCurMeta = _pool + _pageSize + DMS_LOB_META_LENGTH + _curOffset ;

         if ( len <= lastLen )
         {
            ossMemcpy( pCurMeta, data, len ) ;
            _metaSize = OSS_MAX( _metaSize, _curOffset + len ) ;
            _curOffset += len ;
            goto done ;
         }
         else
         {
            ossMemcpy( pCurMeta, data, lastLen ) ;
            _metaSize = _curOffset + lastLen ;
            SDB_ASSERT( _pageSize - DMS_LOB_META_LENGTH == _metaSize,
                        "meta size must be pagesize - metalen" ) ;
            _curOffset += lastLen ;
            SDB_ASSERT( _pageSize - DMS_LOB_META_LENGTH == _curOffset,
                        "Cur offset must be pageSize - metaLen" ) ;
            len -= lastLen ;
            data += lastLen ;
         }
      }

      if ( 0 == _cachedSz )
      {
         _writeData.tuple.columns.offset = _curOffset ;
         _writeData.tuple.columns.len = len ;
         _writeData.data = data ;
      }
      else
      {
         UINT32 curOffsetInPage = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                                  _mergeMeta,
                                                                  _pageSize ) ;
         UINT32 curPageRemainSize = _pageSize - curOffsetInPage - (UINT32)_cachedSz ;

         SDB_ASSERT( (UINT32)_cachedSz + curOffsetInPage < (UINT32)_pageSize, "impossible" ) ;

         UINT32 mvSize = curPageRemainSize <= len ? curPageRemainSize : len ;
         ossMemcpy( _pool + curOffsetInPage + _cachedSz, data, mvSize ) ;
         _cachedSz += mvSize ;
         if ( len > mvSize )
         {
            _writeData.tuple.columns.offset = _curOffset + _cachedSz ;
            _writeData.tuple.columns.len = len - mvSize ;
            _writeData.data = data + mvSize ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB_RTNLOBWINDOW_PREPARE4WRITE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES, "_rtnLobWindow::getNextWriteSequences" )
   BOOLEAN _rtnLobWindow::getNextWriteSequences( RTN_LOB_TUPLES &tuples )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES ) ;
      _rtnLobTuple tuple ;
      BOOLEAN hasNext = FALSE ;
      while ( _getNextWriteSequence( tuple ) )
      {
         tuples.push_back( tuple ) ;
         hasNext = TRUE ;
      }

      PD_TRACE_EXIT( SDB_RTNLOBWINDOW_GETNEXTWRITESEQUENCES ) ;
      return hasNext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE, "_rtnLobWindow::_getNextWriteSequence" )
   BOOLEAN _rtnLobWindow::_getNextWriteSequence( _rtnLobTuple &tuple )
   {
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE ) ;
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;

      UINT32 curOffsetInPage = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                               _mergeMeta,
                                                               _pageSize ) ;
      UINT32 curPageRemainSize = _pageSize - curOffsetInPage ;

      if ( 0 == _cachedSz )
      {
         if ( curPageRemainSize <= _writeData.tuple.columns.len && !_writeData.empty() )
         {
            SDB_ASSERT( _curOffset == _writeData.tuple.columns.offset, "incorrect offset" ) ;
            t.columns.len = curPageRemainSize ;
            t.columns.offset = curOffsetInPage ;
            t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                       _mergeMeta,
                                                       _logarithmic ) ;
            tuple.data = _writeData.data ;

            _writeData.tuple.columns.len -= curPageRemainSize ;
            _writeData.tuple.columns.offset += curPageRemainSize ;
            _writeData.data += curPageRemainSize ;
            if ( 0 == _writeData.tuple.columns.len )
            {
               _writeData.clear() ;
            }

            _curOffset += curPageRemainSize ;
            hasNext = TRUE ;
         }
         else
         {
            goto done ;
         }
      }
      else if ( curPageRemainSize == (UINT32)_cachedSz )
      {
         t.columns.len = _cachedSz ;
         t.columns.offset = curOffsetInPage ;
         t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                    _mergeMeta,
                                                    _logarithmic ) ;
         tuple.data = _pool + curOffsetInPage ;

         _curOffset += _cachedSz ;
         hasNext = TRUE ;

         _cachedSz = 0 ;
      }
      else
      {
         SDB_ASSERT( _writeData.empty(), "should be joined before" ) ;
      }

   done:
      PD_TRACE_EXIT( SDB_RTNLOBWINDOW__GETNEXTWRITESEQUENCE ) ;
      return hasNext ;
   }

   void _rtnLobWindow::cacheLastDataOrClearCache()
   {
      if ( !_writeData.empty() )
      {
         UINT32 curOffsetInPage = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                                  _mergeMeta,
                                                                  _pageSize ) ;

         SDB_ASSERT( curOffsetInPage + (UINT32)_cachedSz + _writeData.tuple.columns.len < (UINT32)_pageSize,
                     "Write data len must less than data page remain size" ) ;
         SDB_ASSERT( 0 == _cachedSz, "Cached size must be 0" ) ;
         SDB_ASSERT( _curOffset == _writeData.tuple.columns.offset, "incorrect offset" ) ;

         ossMemcpy( _pool + curOffsetInPage + _cachedSz, _writeData.data,
                    _writeData.tuple.columns.len ) ;
         _cachedSz += _writeData.tuple.columns.len ;
         _writeData.clear() ;
      }
   }

   BOOLEAN _rtnLobWindow::getCachedData( _rtnLobTuple &tuple )
   {
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;
      if ( 0 == _cachedSz )
      {
         goto done ;
      }

      t.columns.len = _cachedSz ;
      t.columns.sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                 _mergeMeta,
                                                 _logarithmic ) ;
      t.columns.offset = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                         _mergeMeta,
                                                         _pageSize ) ;
      tuple.data = _pool + t.columns.offset ;

      SDB_ASSERT( t.columns.offset + _cachedSz < _pageSize,
                  "incorrect cached data" ) ;

      _curOffset += _cachedSz ;
      hasNext = TRUE ;

      _cachedSz = 0 ;

   done:
      return hasNext ;
   }

   BOOLEAN _rtnLobWindow::getMetaPageData( _rtnLobTuple &tuple )
   {
      BOOLEAN hasNext = FALSE ;
      MsgLobTuple &t = tuple.tuple ;

      if ( 0 == _metaSize )
      {
         goto done ;
      }

      t.columns.len = _metaSize + DMS_LOB_META_LENGTH ;
      t.columns.sequence = DMS_LOB_META_SEQUENCE ;
      t.columns.offset = 0 ;
      tuple.data = _pool + _pageSize ;

      SDB_ASSERT( DMS_LOB_META_LENGTH + _metaSize <= _pageSize,
                  "incorrect meta size" ) ;

      hasNext = TRUE ;
      _metaSize = 0 ;

   done:
      return hasNext ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB_RTNLOBWINDOW_PREPARE4READ, "_rtnLobWindow::_rtnLobWindow::prepare4Read" )
   INT32 _rtnLobWindow::prepare4Read( INT64 lobLen,
                                      INT64 offset,
                                      UINT32 len,
                                      RTN_LOB_TUPLES &tuples )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB_RTNLOBWINDOW_PREPARE4READ ) ;
      SDB_ASSERT( offset < lobLen, "impossible" ) ;
      UINT32 totalRead = 0 ;
      _curOffset = offset ;
      UINT32 maxLen = RTN_MAX_READ_LEN <= ( lobLen - offset ) ?
                      RTN_MAX_READ_LEN : ( lobLen - offset ) ;
      UINT32 needRead = len <= RTN_MIN_READ_LEN ?
                        RTN_MIN_READ_LEN : len ;
      tuples.clear() ;

      while ( _curOffset < lobLen &&
              totalRead < needRead &&
              totalRead < maxLen ) 
      {
         UINT32 offsetOfTuple = RTN_LOB_GET_OFFSET_IN_SEQUENCE( _curOffset,
                                                                _mergeMeta,
                                                                _pageSize ) ;
         UINT32 lenOfTuple = _pageSize - offsetOfTuple ;
         if ( ( lobLen - _curOffset ) < lenOfTuple )
         {
            lenOfTuple = lobLen - _curOffset ;
         }

         if ( 0 == lenOfTuple )
         {
            break ;
         }

         UINT32 sequence = RTN_LOB_GET_SEQUENCE( _curOffset,
                                                 _mergeMeta,
                                                 _logarithmic ) ;

         _curOffset += lenOfTuple ;
         totalRead += lenOfTuple ;

         tuples.push_back( _rtnLobTuple( lenOfTuple,
                                         sequence,
                                         offsetOfTuple,
                                         NULL ) ) ;
      }

      PD_TRACE_EXITRC( SDB_RTNLOBWINDOW_PREPARE4READ, rc ) ;
      return rc ;
   }

}

