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

   Source File Name = utilCompressor.hpp

   Descriptive Name = Compressor for data compression and decompression.

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
#ifndef UTIL_COMPRESSOR__
#define UTIL_COMPRESSOR__

#include "core.hpp"
#include "oss.hpp"
#include "utilCompression.hpp"
#include "utilDictionary.hpp"

namespace engine
{

   #define UTIL_INVALID_DICT                 NULL

   typedef void * utilDictHandle ;

   #define UTIL_COMPRESSOR_DFT_MIN_RATIO     80
   #define UTIL_COMPRESSOR_DFT_LEVEL         UTIL_COMP_BEST_COMPRESSION

   struct _utilCompressStrategy
   {
      UINT8 _minRatio ;
      UTIL_COMPRESSION_LEVEL _level ;
   } ;
   typedef _utilCompressStrategy utilCompressStrategy ;

   /* This class provides compressor interfaces. */
   class _utilCompressor : public SDBObject
   {
   public:
      _utilCompressor( UTIL_COMPRESSOR_TYPE type )
      : _type( type )
      {
      }

      virtual ~_utilCompressor() {}

      virtual INT32 compressBound( UINT32 srcLen, UINT32 &maxCompressedLen,
                                   const utilDictHandle dictionary = NULL ) = 0 ;

      virtual INT32 compress( const CHAR *source, UINT32 sourceLen,
                              CHAR *dest, UINT32 &destLen,
                              const utilDictHandle dictionary = NULL,
                              const utilCompressStrategy *strategy = NULL ) = 0 ;

      virtual INT32 getUncompressedLen( const CHAR *source, UINT32 sourceLen,
                                        UINT32 &length) = 0 ;

      virtual INT32 decompress( const CHAR *source, UINT32 sourceLen,
                                CHAR *dest, UINT32 &destLen,
                                const utilDictHandle dictionary = NULL ) = 0 ;

      OSS_INLINE UTIL_COMPRESSOR_TYPE getType () const
      {
         return _type ;
      }

      virtual OSS_INLINE BOOLEAN needDictionay () const
      {
         return FALSE ;
      }

   private:
      UTIL_COMPRESSOR_TYPE _type ;
   } ;
   typedef _utilCompressor utilCompressor ;

   /*
    * Get the global compressor pointer. When that is done, you can use the
    * compressor to compress/decompress data.
    */
   utilCompressor* getCompressorByType( UTIL_COMPRESSOR_TYPE type ) ;

   /* Get the name of the compressor in string format. */
   const CHAR *utilCompressType2String( UINT8 type ) ;

   UTIL_COMPRESSOR_TYPE utilString2CompressType( const CHAR *pStr ) ;

}

#endif /* UTIL_COMPRESSOR__ */

