/*******************************************************************************

   Copyright (C) 2011-2017 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = rtnLobMetaCache.cpp

   Descriptive Name = LOB meta cache

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/22/2017  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobMetaCache.hpp"
#include "pd.hpp"

namespace engine
{
   _rtnLobMetaCache::_rtnLobMetaCache()
      : _metaBuf( NULL ),
        _needMerge( FALSE )
   {
   }

   _rtnLobMetaCache::~_rtnLobMetaCache()
   {
      SAFE_OSS_FREE( _metaBuf ) ;
   }

   INT32 _rtnLobMetaCache::cache( const _dmsLobMeta& meta )
   {
      INT32 rc = SDB_OK ;

      if ( NULL == _metaBuf )
      {
         _metaBuf = (CHAR*)SDB_OSS_MALLOC( DMS_LOB_META_LENGTH ) ;
         if ( NULL == _metaBuf )
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to malloc lob meta cache, rc=%d", rc ) ;
            goto error ;
         }
         ossMemset( _metaBuf, 0, DMS_LOB_META_LENGTH ) ;
      }

      if ( meta.hasPiecesInfo() )
      {
         ossMemcpy( _metaBuf, &meta, DMS_LOB_META_LENGTH ) ;
      }
      else
      {
         ossMemcpy( _metaBuf, &meta, sizeof( _dmsLobMeta ) ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnLobMetaCache::merge( const _dmsLobMeta& meta, INT32 lobPageSize )
   {
      INT32 rc = SDB_OK ;
      _dmsLobMeta* cachedMeta = NULL ;
      UINT32 logarithmic = 0 ;
      BOOLEAN mergeData = FALSE ;
      _rtnLobPiecesInfo info ;

      if ( NULL == _metaBuf )
      {
         rc = cache( meta ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to cache lob meta, rc=%d", rc ) ;
            goto error ;
         }
         goto done ;
      }

      if ( !ossIsPowerOf2( (UINT32)lobPageSize, &logarithmic ) )
      {
         PD_LOG( PDERROR, "Invalid page size:%d, "\
                          "it should be a power of 2",
                          lobPageSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      cachedMeta = (_dmsLobMeta*)_metaBuf ;

      if ( meta._version != cachedMeta->_version )
      {
         PD_LOG( PDERROR, "Invalid lob meta version, "\
                          "cached:%u, merged:%u",
                          cachedMeta->_version, meta._version ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      mergeData = cachedMeta->_version >= DMS_LOB_META_MERGE_DATA_VERSION ;

      if ( cachedMeta->hasPiecesInfo() )
      {
         INT32 cachedLen = cachedMeta->_piecesInfoNum *
                           (INT32)sizeof( _rtnLobPieces ) ;
         rc = info.readFrom( _metaBuf + DMS_LOB_META_LENGTH - cachedLen,
                             cachedLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read lob pieces info from cache, "\
                             "rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT32 last = RTN_LOB_GET_SEQUENCE( cachedMeta->_lobLen - 1,
                                             mergeData,
                                             logarithmic ) ;
         rc = info.addPieces( _rtnLobPieces( 0, last ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to add lob pieces, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( meta.hasPiecesInfo() )
      {
         INT32 mergeLen = meta._piecesInfoNum * 
                          (INT32)sizeof( _rtnLobPieces ) ;
         rc = info.mergeFrom( (CHAR*)&meta + DMS_LOB_META_LENGTH - mergeLen,
                              mergeLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to read lob pieces info from cache, "\
                             "rc=%d", rc ) ;
            goto error ;
         }
      }
      else
      {
         UINT32 last = RTN_LOB_GET_SEQUENCE( meta._lobLen - 1,
                                             mergeData,
                                             logarithmic ) ;
         rc = info.addPieces( _rtnLobPieces( 0, last ) ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to add lob pieces, rc=%d", rc ) ;
            goto error ;
         }
      }

      if ( info.requiredMem() > DMS_LOB_META_PIECESINFO_MAX_LEN )
      {
         rc = SDB_LOB_PIECESINFO_OVERFLOW ;
         PD_LOG( PDERROR, "LOB pieces info require memory more than %d bytes, "\
                          "section num=%d, piecesInfo=%s",
                          DMS_LOB_META_PIECESINFO_MAX_LEN,
                          info.sectionNum(), info.toString().c_str() ) ;
         goto error ;
      }

      if ( info.sectionNum() == 1 )
      {
         UINT32 last = RTN_LOB_GET_SEQUENCE(
                           OSS_MAX( cachedMeta->_lobLen, meta._lobLen ) - 1,
                           mergeData,
                           logarithmic ) ;
         _rtnLobPieces pieces = info.getSection( 0 ) ;
         if ( 0 == pieces.first && last == pieces.last )
         {
            info.clear() ;
         }
      }

      if ( info.empty() )
      {
         cachedMeta->_flag |= meta._flag ;
         OSS_BIT_CLEAR( cachedMeta->_flag, DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ;
         cachedMeta->_piecesInfoNum = 0 ;
      }
      else
      {
         INT32 newLen = info.requiredMem() ;
         rc = info.saveTo( _metaBuf + DMS_LOB_META_LENGTH - newLen, newLen ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to save lob pieces info, rc=%d", rc ) ;
            goto error ;
         }
         cachedMeta->_flag |= meta._flag ;
         OSS_BIT_SET( cachedMeta->_flag, DMS_LOB_META_FLAG_PIECESINFO_INSIDE ) ;
         cachedMeta->_piecesInfoNum = info.sectionNum() ;
      }

      cachedMeta->_lobLen = OSS_MAX( cachedMeta->_lobLen, meta._lobLen ) ;
      cachedMeta->_modificationTime = OSS_MAX(
         cachedMeta->_modificationTime, meta._modificationTime ) ;

   done:
      return rc ;
   error:
      goto done ;
   }
}

