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

   Source File Name = utilCompressorZlib.hpp

   Descriptive Name = Zlib Compressor for data compression and decompression.

   When/how to use: this program may be used to compress/decompress data. This
   file contains the interfaces provided by compressors.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSOR_ZLIB__
#define UTIL_COMPRESSOR_ZLIB__

#include "utilCompressor.hpp"
#include "zlib.h"

namespace engine
{
   class _utilCompressorZlib : public utilCompressor
   {
   public:
      _utilCompressorZlib();
      ~_utilCompressorZlib();

   public:
      INT32 compressBound( UINT32 srcLen, UINT32 &maxCompressedLen,
                           const utilDictHandle dictionary = NULL ) ;

      INT32 compress( const CHAR *source, UINT32 sourceLen,
                      CHAR *dest, UINT32 &destLen,
                      const utilDictHandle dictionary = NULL,
                      const utilCompressStrategy *strategy = NULL ) ;

      INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                UINT32 &length ) ;

      INT32 decompress( const CHAR *source, UINT32 sourceLen,
                        CHAR *dest, UINT32 &destLen,
                        const utilDictHandle dictionary = NULL ) ;
   };
   typedef _utilCompressorZlib utilCompressorZlib ;
}

#endif /* UTIL_COMPRESSOR_ZLIB__ */

