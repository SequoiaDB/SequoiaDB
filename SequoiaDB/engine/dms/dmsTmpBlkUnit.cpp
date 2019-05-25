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

   Source File Name = dmsTmpBlkUnit.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-formatted
   versions of OSS component. This file contains declares for IO operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "dmsTmpBlkUnit.hpp"

#include <sstream>

namespace engine
{
   _dmsTmpBlk::_dmsTmpBlk()
   :_begin(0),
    _size(0),
    _read(0)
   {

   }

   _dmsTmpBlk::~_dmsTmpBlk()
   {

   }

   std::string _dmsTmpBlk::toString()const
   {
      std::stringstream ss ;
      ss << "offset:" << _begin << ", size:" << _size
         << ", read:" << _read ;
      return ss.str() ;
   }

   _dmsTmpBlkUnit::_dmsTmpBlkUnit()
   :_totalSize(0),
    _opened(0)
   {

   }

   _dmsTmpBlkUnit::~_dmsTmpBlkUnit()
   {
      _removeFile() ;
   }

   INT32 _dmsTmpBlkUnit::openFile( const CHAR *path,
                                   const DMS_TMP_FILE_ID &id )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != path, "impossible" ) ;
      std::stringstream ss ;
      ss << path << OSS_FILE_SEP << DMS_TMP_BLK_FILE_BEGIN << id ;
      _fullPath = ss.str() ;
      if ( OSS_MAX_PATHSIZE < _fullPath.size() )
      {
         PD_LOG( PDERROR, "file path is too long:%s",
                 _fullPath.c_str() ) ;
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      rc = ossOpen( _fullPath.c_str(),
                    OSS_READWRITE|OSS_EXCLUSIVE|OSS_REPLACE,
                    OSS_RU|OSS_WU|OSS_RG,
                    _file ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to open file:%s, rc=%d",
                 _fullPath.c_str(), rc ) ;
         goto error ;
      }

      _opened = TRUE ;
   done:
      return rc ;
   error:
      _fullPath.clear() ;
      goto done ;
   }

   INT32 _dmsTmpBlkUnit::_removeFile()
   {
      INT32 rc = SDB_OK ;

      if ( _opened )
      {
         rc = ossClose( _file ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to close file:%s, rc=%d",
                    _fullPath.c_str(), rc ) ;
         }

         rc = ossDelete( _fullPath.c_str() ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "failed to remove file:%s, rc=%d",
                    _fullPath.c_str(), rc ) ;
            goto error ;
         }

         _fullPath.clear() ;
         _opened = FALSE ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsTmpBlkUnit::seek( const UINT64 &offset )
   {
      INT32 rc = SDB_OK ;
      if ( _totalSize < offset )
      {
         PD_LOG( PDERROR, "failed to seek to[%lld]"
                 ", totalSize[%lld]", offset, _totalSize ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      rc = ossSeek( &_file, offset, OSS_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek to begin of file:%d", rc ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsTmpBlkUnit::write( const void *buf,
                                UINT64 size,
                                BOOLEAN extendSize )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( NULL != buf && 0 != size, "impossible" ) ;
      SINT64 needToW = size ;
      UINT64 written = 0 ;
      SINT64 writeOnce = 0 ;
      SDB_ASSERT( 0 < needToW, "impossible" ) ;

      while ( 0 < needToW )
      {
         writeOnce = 0 ;
         rc = ossWrite( &_file, (const CHAR *)buf + written,
                        needToW, &writeOnce ) ;
         if ( rc && SDB_INTERRUPT != rc )
         {
            PD_LOG( PDERROR, "failed to write to file:%d", rc ) ;
            goto error ;
         }
         rc = SDB_OK ;

         needToW -= writeOnce ;
         written += writeOnce ;
      }

      SDB_ASSERT( size == written, "impossible" ) ;
      if ( extendSize )
      {
         _totalSize += size ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsTmpBlkUnit::buildBlk( const UINT64 &offset,
                                   const UINT64 &size,
                                   dmsTmpBlk &blk )
   {
      INT32 rc = SDB_OK ;
      SDB_ASSERT( offset  + size <= _totalSize, "impossible" ) ;
      SDB_ASSERT( 0 != size, "impossible" ) ;
      blk.reset() ;

      if ( _totalSize < offset + size )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "out of file:"
                 "totalSize[%lld], offset[%lld], size[%d]",
                 _totalSize, offset, size ) ;
         goto error ;

      }

      blk._begin = offset ;
      blk._size = size ;
      blk._read = 0 ;
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _dmsTmpBlkUnit::read( dmsTmpBlk &blk, UINT64 size,
                               void *buf, UINT64 &got )
   {
      SDB_ASSERT( NULL != buf && 0 != size, "impossible" ) ;
      INT32 rc = SDB_OK ;
      INT64 toRead = blk._size - blk._read < size ?
                     blk._size - blk._read : size ;
      SDB_ASSERT( 0 <= toRead, "impossible" ) ;
      UINT64 hasRead = 0 ;
      INT64 readOnce = 0 ;

      rc = ossSeek( &_file, blk._begin + blk._read, OSS_SEEK_SET ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to seek to begin of file:%d", rc ) ;
         goto error ;
      }

      while ( 0 < toRead )
      {
         readOnce = 0 ;
         rc = ossRead( &_file, ( CHAR * )buf + hasRead,
                       toRead, &readOnce ) ;
         if ( rc && SDB_INTERRUPT != rc )
         {
            PD_LOG( PDERROR, "failed to read blk:%d", rc ) ;
            goto error ;
         }

         toRead -= readOnce ;
         blk._read += readOnce ;
         hasRead += readOnce ;
         rc = SDB_OK ;
      }

      got = hasRead ;

      if ( 0 == hasRead )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }
}

