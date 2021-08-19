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

   Source File Name = utilZlibStream.hpp

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
#ifndef UTIL_ZLIB_STREAM_HPP_
#define UTIL_ZLIB_STREAM_HPP_

#include "utilCompressionStream.hpp"

extern "C" struct z_stream_s ;

namespace engine
{

   class utilZlibInStream: public utilCompressionInStream
   {
   public:
      utilZlibInStream() ;
      ~utilZlibInStream() ;

   public:
      INT32 init( utilInStream& upstream,
                  INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 read( CHAR* buf, INT64 bufLen, INT64& readSize ) ;
      INT32 close() ;

   private:
      z_stream_s* _zstream ;
      CHAR*       _zbuf ;
      INT32       _zbufSize ;
      BOOLEAN     _inited ;
      BOOLEAN     _read ;
      BOOLEAN     _end ;
   } ;

   class utilZlibOutStream: public utilCompressionOutStream
   {
   public:
      utilZlibOutStream() ;
      ~utilZlibOutStream() ;

   public:
      INT32 init( utilOutStream& downstream, 
                  UTIL_COMPRESSION_LEVEL level = UTIL_COMP_BALANCE,
                  INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 write( const CHAR* buf, INT64 bufLen ) ;
      INT32 flush() ;
      INT32 finish() ;
      INT32 close() ;

   private:
      z_stream_s* _zstream ;
      UTIL_COMPRESSION_LEVEL _level ;
      CHAR*       _zbuf ;
      INT32       _zbufSize ;
      BOOLEAN     _inited ;
      BOOLEAN     _finished ;
      BOOLEAN     _dirty ;
   } ;

   class utilZlibStreamCompressor: public SDBObject
   {
   public:
      utilZlibStreamCompressor() ;
      ~utilZlibStreamCompressor() ;

   public:
      INT32 init( INT32 bufSize = UTIL_STREAM_DEFAULT_BUFFER_SIZE ) ;
      INT32 compress( utilInStream& in, utilOutStream& out,
                      UTIL_COMPRESSION_LEVEL level = UTIL_COMP_BALANCE ) ;
      INT32 uncompress( utilInStream& in, utilOutStream& out ) ;

   private:
      utilStream  _stream ;
   } ;
}

#endif /* UTIL_ZLIB_STREAM_HPP_ */
