#ifndef UTIL_COMPRESSOR_LZ4__
#define UTIL_COMPRESSOR_LZ4__

#include "utilCompressor.hpp"
#include "lz4.h"

namespace engine
{
   class _utilCompressorLZ4 : public utilCompressor
   {
   public:
      _utilCompressorLZ4() ;
      ~_utilCompressorLZ4() ;

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
   typedef _utilCompressorLZ4 utilCompressorLZ4 ;
}

#endif /* UTIL_COMPRESSOR_LZ4__ */

