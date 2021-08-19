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

   Source File Name = utilZlibStream.cpp

   Descriptive Name = Zlib compression stream

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/8/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilZlibStream.hpp"
#include "ossMem.hpp"
#include "pd.hpp"
#include "zlib.h"

namespace engine
{
   void* _zlib_alloc( void* opaque, UINT32 items, UINT32 size )
   {
      (void)opaque ;
      return SDB_OSS_MALLOC( items * size ) ;
   }

   void _zlib_free( void* opaque, void* address )
   {
      (void)opaque ;
      SDB_OSS_FREE( address ) ;
   }
   
   utilZlibInStream::utilZlibInStream()
      : _zstream( NULL ),
        _zbuf( NULL ),
        _zbufSize( UTIL_STREAM_DEFAULT_BUFFER_SIZE ),
        _inited( FALSE ),
        _read( TRUE ),
        _end( FALSE )
   {
   }

   utilZlibInStream::~utilZlibInStream()
   {
      if ( NULL != _zbuf )
      {
         SAFE_OSS_FREE( _zbuf ) ;
      }

      if ( NULL != _zstream )
      {
         inflateEnd( _zstream ) ;
         SAFE_OSS_FREE( _zstream ) ;
      }
   }

   INT32 utilZlibInStream::init( utilInStream& upstream, INT32 bufSize )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( !_inited, "inited" ) ;

      _zbuf = (CHAR*)SDB_OSS_MALLOC( bufSize ) ;
      if ( NULL == _zbuf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc zlib instream buffer, rc=%d", rc ) ;
         goto error ;
      }
      _zbufSize = bufSize ;

      _zstream = (z_stream*)SDB_OSS_MALLOC( sizeof( z_stream ) ) ;
      if ( NULL == _zstream )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc z_stream, rc=%d", rc ) ;
         goto error ;
      }

      _zstream->zalloc = _zlib_alloc ;
      _zstream->zfree = _zlib_free ;
      _zstream->opaque = NULL ;
      _zstream->next_in = NULL ;
      _zstream->avail_in = 0 ;

      rc = inflateInit( _zstream ) ;
      if ( Z_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init z_stream, zlib error=%d", rc ) ;
         rc = SDB_UTIL_COMPRESS_INIT_FAIL ;
         goto error;
      }

      _upstream = &upstream ;
      _inited = TRUE ;

   done:
      return rc ;
   error:
      SAFE_OSS_FREE( _zstream ) ;
      SAFE_OSS_FREE( _zbuf ) ;
      goto done ;
   }
   
   INT32 utilZlibInStream::read( CHAR* buf, INT64 bufLen, INT64& readSize )
   {
      INT32 rc = SDB_OK ;
      INT64 rsize = 0 ;

      SDB_ASSERT( NULL != buf, "buf can't be NULL" ) ;
      SDB_ASSERT( bufLen > 0, "bufLen should >0" ) ;
      SDB_ASSERT( _inited, "not init" ) ;
      SDB_ASSERT( NULL != _zbuf, "_zbuf can't be NULL" ) ;
      SDB_ASSERT( _zbufSize > 0, "_zbufSize should >0" ) ;

      if ( _end )
      {
         rc = SDB_EOF ;
         goto done ;
      }

      for ( ;; )
      {
         if ( _read )
         {
            rc = _upstream->read( _zbuf, _zbufSize, rsize ) ;
            if ( SDB_OK != rc )
            {
               if ( SDB_EOF == rc )
               {
                  goto done ;
               }

               PD_LOG( PDERROR, "Failed to read from upstream, rc=%d", rc ) ;
               goto error ;
            }

            _zstream->next_in = (Bytef*)_zbuf ;
            _zstream->avail_in = rsize ;
         }

         _zstream->next_out = (Bytef*)buf ;
         _zstream->avail_out = bufLen ;

         rc = inflate( _zstream, Z_NO_FLUSH ) ;
         if ( Z_BUF_ERROR == rc )
         {
            _read = TRUE;
            rc = SDB_OK;
            continue;
         }
         else if ( Z_OK != rc && Z_STREAM_END != rc )
         {
            PD_LOG( PDERROR, "Failed to inflate, zlib error=%d", rc ) ;
            rc = SDB_UTIL_DECOMPRESS_FAIL ;
            goto error ;
         }

         if ( _zstream->avail_out == 0 )
         {
            _read = FALSE ;
         }
         else
         {
            _read = TRUE ;
         }

         if ( Z_STREAM_END == rc )
         {
            _end = TRUE ;
            rc = SDB_OK ;
         }

         readSize = bufLen - _zstream->avail_out ;
         SDB_ASSERT( 0 != readSize, "readSize == 0" ) ;
         break;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilZlibInStream::close()
   {
      INT32 rc = SDB_OK ;

      rc = _upstream->close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close upstream, rc=%d", rc ) ;
         goto error ;
      }

      inflateEnd( _zstream ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   utilZlibOutStream::utilZlibOutStream()
      : _zstream( NULL ),
        _level( UTIL_COMP_BALANCE ),
        _zbuf( NULL ),
        _zbufSize( UTIL_STREAM_DEFAULT_BUFFER_SIZE ),
        _inited( FALSE ),
        _finished( FALSE ),
        _dirty( FALSE )
   {
   }

   utilZlibOutStream::~utilZlibOutStream()
   {
      if ( NULL != _zbuf )
      {
         SAFE_OSS_FREE( _zbuf ) ;
      }

      if ( NULL != _zstream )
      {
         deflateEnd( _zstream ) ;
         SAFE_OSS_FREE( _zstream ) ;
      }
   }

   INT32 utilZlibOutStream::init( utilOutStream& downstream,
                                  UTIL_COMPRESSION_LEVEL level, INT32 bufSize )
   {
      INT32 rc = SDB_OK ;
      INT32 zlevel = 0 ;

      SDB_ASSERT( !_inited, "inited" ) ;

      _zbuf = (CHAR*)SDB_OSS_MALLOC( bufSize ) ;
      if ( NULL == _zbuf )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc zlib outstream buffer, rc=%d", rc ) ;
         goto error ;
      }
      _zbufSize = bufSize ;

      switch( level )
      {
      case UTIL_COMP_BALANCE:
         zlevel = Z_DEFAULT_COMPRESSION ;
         break ;
      case UTIL_COMP_BEST_COMPRESSION:
         zlevel = Z_BEST_COMPRESSION ;
         break ;
      case UTIL_COMP_BEST_SPEED:
         zlevel = Z_BEST_SPEED ;
         break ;
      default:
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid compression level: %d", level ) ;
         goto error ;
      }
      _level = level ;

      _zstream = (z_stream*)SDB_OSS_MALLOC( sizeof( z_stream ) ) ;
      if ( NULL == _zstream )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to malloc z_stream, rc=%d", rc ) ;
         goto error ;
      }

      _zstream->zalloc = _zlib_alloc ;
      _zstream->zfree = _zlib_free ;
      _zstream->opaque = NULL ;
      _zstream->data_type = Z_BINARY ;

      rc = deflateInit( _zstream, zlevel ) ;
      if ( Z_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init z_stream, zlib error=%d", rc ) ;
         rc = SDB_UTIL_COMPRESS_INIT_FAIL ;
         goto error;
      }

      _downstream = &downstream ;
      _inited = TRUE ;

   done:
      return rc ;
   error:
      SAFE_OSS_FREE( _zstream ) ;
      SAFE_OSS_FREE( _zbuf ) ;
      goto done ;
   }
   
   INT32 utilZlibOutStream::write( const CHAR* buf, INT64 bufLen )
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( NULL != buf, "buf can't be NULL" ) ;
      SDB_ASSERT( bufLen > 0, "bufLen should >0" ) ;
      SDB_ASSERT( _inited, "not init" ) ;
      SDB_ASSERT( NULL != _zbuf, "_zbuf can't be NULL" ) ;
      SDB_ASSERT( _zbufSize > 0, "_zbufSize should >0" ) ;
      SDB_ASSERT( !_finished, "finished" ) ;

      _zstream->next_in = (Bytef*)buf ;
      _zstream->avail_in = bufLen ;
      _zstream->next_out = (Bytef*)_zbuf ;
      _zstream->avail_out = 0 ;

      while ( 0 == _zstream->avail_out )
      {
         // Reset the output buffer for multiple loops
         _zstream->next_out = (Bytef*)_zbuf ;
         _zstream->avail_out = _zbufSize ;

         rc = deflate( _zstream, Z_NO_FLUSH ) ;
         if ( Z_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to deflate, zlib error=%d", rc ) ;
            rc = SDB_UTIL_COMPRESS_FAIL ;
            goto error;
         }

         if ( _zstream->avail_out == (uInt)_zbufSize )
         {
            rc = SDB_OK ;
            break ;
         }

         SDB_ASSERT( (uInt)_zbufSize > _zstream->avail_out, "avail_out < bufSize" ) ;

         rc = _downstream->write( _zbuf, _zbufSize - _zstream->avail_out ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write compressed data, rc=%d", rc ) ;
            goto error ;
         }
      }

      _dirty = TRUE ;

   done:
      return rc ;
   error:
      deflateEnd( _zstream ) ;
      goto done ;
   }
   
   INT32 utilZlibOutStream::flush()
   {
      INT32 rc = SDB_OK ;

      rc = _downstream->flush() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to flush data, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 utilZlibOutStream::finish()
   {
      INT32 rc = SDB_OK ;

      SDB_ASSERT( _inited, "not init" ) ;
      SDB_ASSERT( NULL != _zbuf, "_zbuf can't be NULL" ) ;
      SDB_ASSERT( _zbufSize > 0, "_zbufSize should >0" ) ;

      if ( !_dirty )
      {
         goto done ;
      }

      _zstream->next_in = NULL ;
      _zstream->avail_in = 0 ;
      _zstream->next_out = (Bytef*)_zbuf ;

      for ( ;; )
      {
         // Reset the output buffer for multiple loops
         _zstream->next_out = (Bytef*)_zbuf ;
         _zstream->avail_out = _zbufSize ;

         rc = deflate( _zstream, Z_FINISH ) ;
         if ( Z_OK != rc && Z_STREAM_END != rc  )
         {
            PD_LOG( PDERROR, "Failed to finish deflate, zlib error=%d", rc ) ;
            rc = SDB_UTIL_COMPRESS_FAIL ;
            goto error;
         }

         if ( _zstream->avail_out == (uInt)_zbufSize )
         {
            SDB_ASSERT( Z_STREAM_END == rc, "rc != Z_STREAM_END" ) ;
            rc = SDB_OK ;
            break ;
         }

         SDB_ASSERT( (uInt)_zbufSize > _zstream->avail_out, "avail_out < bufSize" ) ;

         rc = _downstream->write( _zbuf, _zbufSize - _zstream->avail_out ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to write compressed data, rc=%d", rc ) ;
            goto error ;
         }

         if ( Z_STREAM_END == rc )
         {
            rc = SDB_OK ;
            break ;
         }
      }

      rc = _downstream->flush() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to flush data, rc=%d", rc ) ;
         goto error ;
      }

   done:
      _dirty = FALSE ;
      _finished = TRUE ;
      deflateEnd( _zstream ) ;
      return rc ;
   error:
      goto done ;
   }
   
   INT32 utilZlibOutStream::close()
   {
      INT32 rc = SDB_OK ;

      if ( _dirty )
      {
         rc = finish() ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "Failed to finish zlib out stream, rc=%d", rc ) ;
            goto error ;
         }
      }

      rc = flush() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to flush downstream, rc=%d", rc ) ;
         goto error ;
      }

      rc = _downstream->close() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to close downstream, rc=%d", rc ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   utilZlibStreamCompressor::utilZlibStreamCompressor()
   {
   }

   utilZlibStreamCompressor::~utilZlibStreamCompressor()
   {
   }

   INT32 utilZlibStreamCompressor::init( INT32 bufSize )
   {
      INT32 rc = SDB_OK;

      rc = _stream.init( bufSize ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to init stream, rc=%d", rc ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }
   
   INT32 utilZlibStreamCompressor::compress( utilInStream& in, utilOutStream& out,
                                               UTIL_COMPRESSION_LEVEL level )
   {
      utilZlibOutStream zlibOut ;
      INT32 rc = SDB_OK ;

      rc = zlibOut.init( out, level ) ;
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to init zlib outstream, rc=%d", rc ) ;
         goto error ;
      }

      rc = _stream.copy( in, zlibOut ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to stream for zlib compress, rc=%d", rc ) ;
         goto error ;
      }

      rc = zlibOut.finish() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to finish zlib outstream, rc=%d", rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }
   
   INT32 utilZlibStreamCompressor::uncompress( utilInStream& in, utilOutStream& out )
   {
      utilZlibInStream zlibIn ;
      INT32 rc = SDB_OK ;

      rc = zlibIn.init( in ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to init zlib instream, rc=%d", rc ) ;
         goto error;
      }

      rc = _stream.copy( zlibIn, out ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to stream for zlib uncompress, rc=%d", rc ) ;
         goto error ;
      }

      rc = out.flush() ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to flush outstream, rc=%d", rc ) ;
         goto error ;
      }
      
   done:
      return rc ;
   error:
      goto done ;
   }
}

