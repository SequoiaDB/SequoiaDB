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

