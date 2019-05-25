#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "utilCompressorLZ4.hpp"

namespace engine
{
    _utilCompressorLZ4::_utilCompressorLZ4()
      : _utilCompressor( UTIL_COMPRESSOR_LZ4 )
    {
    }

    _utilCompressorLZ4::~_utilCompressorLZ4()
    {
    }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORLZ4_COMPRESSBOUND, "_utilCompressorLZ4::compressBound" )
   INT32 _utilCompressorLZ4::compressBound( UINT32 srcLen,
                                            UINT32 &maxCompressedLen,
                                            const utilDictHandle dictionary )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORLZ4_COMPRESSBOUND ) ;
      UINT32 maxLen = 0 ;
      SDB_ASSERT( NULL == dictionary, "Dictionary should be NULL" ) ;
      /* 4 bytes are reserved for the original(uncompressed) size. */
      maxLen = (UINT32)LZ4_COMPRESSBOUND( srcLen ) + sizeof(UINT32) ;
      PD_CHECK( maxLen > 0, SDB_UTIL_COMPRESS_BUFF_SMALL, error, PDERROR,
                "Compression buffer too small, expected: u, actual: %u",
                maxLen, maxCompressedLen ) ;

      maxCompressedLen = maxLen ;

   done:
      PD_TRACE_EXIT( SDB__UTILCOMPRESSORLZ4_COMPRESSBOUND ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORLZ4_COMPRESS, "_utilCompressorLZ4::compress" )
   INT32 _utilCompressorLZ4::compress( const CHAR *source, UINT32 sourceLen,
                                       CHAR *dest, UINT32 &destLen,
                                       const utilDictHandle dictionary,
                                       const utilCompressStrategy *strategy )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORLZ4_COMPRESS );
      UINT32 compressedSize = 0 ;
      INT32 acceleration = 0 ;
      UINT32 maxExpectedLen = 0 ;
      UINT32 maxLen = (UINT32)LZ4_COMPRESSBOUND( sourceLen ) + sizeof(UINT32) ;

      (void)dictionary ;

      PD_CHECK( destLen >= maxLen, SDB_UTIL_COMPRESS_BUFF_SMALL,
                error, PDERROR,
                "Compression buffer too small, expected: %u, actual: %u",
                maxLen, destLen ) ;

      if ( strategy )
      {
         acceleration = UTIL_COMP_BEST_COMPRESSION - strategy->_level + 1 ;
         maxExpectedLen = (UINT32)( (UINT64)sourceLen *
                                    strategy->_minRatio / 100 ) ;
      }
      else
      {
         acceleration = 1 ;
         maxExpectedLen = (UINT32)( (UINT64)sourceLen *
                                    UTIL_COMPRESSOR_DFT_MIN_RATIO / 100 ) ;
      }

      /* The first 4 bytes of the output always contain the original length. */
      compressedSize = LZ4_compress_fast( source, dest + sizeof(UINT32),
                                          sourceLen, destLen - sizeof(UINT32),
                                          acceleration ) ;
      SDB_ASSERT( compressedSize > 0, "LZ4 compression failed unexpected" ) ;
      if ( compressedSize > maxExpectedLen )
      {
         rc = SDB_UTIL_COMPRESS_ABORT ;
         PD_LOG( PDDEBUG, "Compression abort as it dosen't meet the ratio "
                 "requirement, rc: %d", rc ) ;
         goto error ;
      }

      *(UINT32*)dest = sourceLen ;
      destLen = compressedSize + sizeof(UINT32) ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORLZ4_COMPRESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORLZ4_GETUNCOMPRESSEDLEN, "_utilCompressorLZ4::getUncompressedLen" )
   INT32 _utilCompressorLZ4::getUncompressedLen( const CHAR *source,
                                                 UINT32 sourceLen,
                                                 UINT32 &length )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORLZ4_GETUNCOMPRESSEDLEN ) ;

      length = *(UINT32*)source ;

      PD_TRACE_EXIT( SDB__UTILCOMPRESSORLZ4_GETUNCOMPRESSEDLEN ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORLZ4_DECOMPRESS, "_utilCompressorLZ4::decompress" )
   INT32 _utilCompressorLZ4::decompress( const CHAR *source, UINT32 sourceLen,
                                         CHAR *dest, UINT32 &destLen,
                                         const utilDictHandle dictionary )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORLZ4_DECOMPRESS ) ;
      UINT32 uncompressLen = *(UINT32*)source ;
      UINT32 actualLen = 0 ;

      (void)dictionary ;

      PD_CHECK( destLen >= uncompressLen, SDB_UTIL_DECOMPRESS_BUFF_SMALL,
                error, PDERROR,
                "Decompression buffer too small, expected: %u, actual: %u",
                uncompressLen, destLen ) ;

      actualLen = LZ4_decompress_fast( source + sizeof(UINT32),
                                       dest, uncompressLen ) ;
      if ( actualLen != sourceLen - sizeof(UINT32) )
      {
         PD_LOG( PDERROR, "Actual length[%d] is not the same with source[%d]",
                 actualLen, sourceLen - sizeof(UINT32) ) ;
         rc = SDB_SYS ;
         goto error ;
      }
      destLen = uncompressLen ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORLZ4_DECOMPRESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

