#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"
#include "utilCompressorZlib.hpp"

namespace engine
{
   _utilCompressorZlib::_utilCompressorZlib()
      : _utilCompressor( UTIL_COMPRESSOR_ZLIB )
   {
   }

   _utilCompressorZlib::~_utilCompressorZlib()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORZLIB_COMPRESSBOUND, "_utilCompressorZlib::compressBound" )
   INT32 _utilCompressorZlib::compressBound( UINT32 srcLen,
                                             UINT32 &maxCompressedLen,
                                             const utilDictHandle dictionary )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORZLIB_COMPRESSBOUND ) ;
      SDB_ASSERT( srcLen > 0, "Source length should be greater than 0" ) ;
      SDB_ASSERT( NULL == dictionary, "Dictionary should be NULL" ) ;
      /*
       * deflateBound may return a conservative value with the first parameter
       * to be NULL.
       */
      maxCompressedLen = deflateBound( NULL, srcLen ) + sizeof( UINT32 ) ;

      PD_TRACE_EXIT( SDB__UTILCOMPRESSORZLIB_COMPRESSBOUND ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORZLIB_COMPRESS, "_utilCompressorZlib::compress" )
   INT32 _utilCompressorZlib::compress( const CHAR *source, UINT32 sourceLen,
                                        CHAR *dest, UINT32 &destLen,
                                        const utilDictHandle dictionary,
                                        const utilCompressStrategy *strategy )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORZLIB_COMPRESS ) ;
      z_stream stream ;
      BOOLEAN init = FALSE ;
      INT32 level = Z_DEFAULT_COMPRESSION ;
      UINT32 maxExpectedLen = 0 ;

      if ( strategy )
      {
         switch( strategy->_level )
         {
         case UTIL_COMP_BEST_SPEED:
            level = Z_BEST_SPEED ;
            break ;
         case UTIL_COMP_BEST_COMPRESSION:
            level = Z_BEST_COMPRESSION ;
            break ;
         default:
            level = Z_DEFAULT_COMPRESSION ;  /* In zlib, this is level 6. */
            break;
         }

         maxExpectedLen = (UINT32)( (UINT64)sourceLen *
                                     strategy->_minRatio / 100 ) ;
      }
      else
      {
         maxExpectedLen = (UINT32)( (UINT64)sourceLen *
                                    UTIL_COMPRESSOR_DFT_MIN_RATIO / 100 ) ;
         level = Z_DEFAULT_COMPRESSION ;
      }

      ossMemset( &stream, 0, sizeof( z_stream ) ) ;
      rc = deflateInit( &stream, level ) ;
      PD_CHECK( Z_OK == rc, SDB_UTIL_COMPRESS_INIT_FAIL, error, PDERROR,
                "Failed to initialize deflate, rc: %d", rc ) ;
      init = TRUE ;

      stream.data_type = Z_BINARY ;
      stream.next_in = (Bytef *)source ;
      stream.avail_in = sourceLen ;
      stream.next_out = (Bytef *)( dest + sizeof( UINT32 ) );
      stream.avail_out = destLen - sizeof( UINT32 ) ;

      rc = deflate( &stream, Z_FINISH ) ;
      PD_CHECK( Z_STREAM_END == rc, SDB_UTIL_COMPRESS_FAIL, error, PDERROR,
                "Failed to deflate data, rc: %d", rc ) ;

      if ( stream.total_out + sizeof( UINT32 ) > maxExpectedLen )
      {
         rc = SDB_UTIL_COMPRESS_ABORT ;
         PD_LOG( PDDEBUG, "Compression abort as it dosen't meet the ratio "
                 "requirement, rc: %d", rc ) ;
         goto error ;
      }

      destLen = stream.total_out + sizeof( UINT32 ) ;
      *(UINT32*)dest = sourceLen ;
      rc = SDB_OK ;

   done:
      if ( init )
      {
         (void)deflateEnd( &stream ) ;
      }
      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORZLIB_COMPRESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORZLIB_GETUNCOMPRESSEDLEN, "_utilCompressorZlib::getUncompressedLen" )
   INT32 _utilCompressorZlib::getUncompressedLen( const CHAR *source,
                                                  UINT32 sourceLen,
                                                  UINT32 &length )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORZLIB_GETUNCOMPRESSEDLEN ) ;
      SDB_ASSERT( source, "Source data should not be NULL" ) ;
      length = *(UINT32 *)source ;

      PD_TRACE_EXIT( SDB__UTILCOMPRESSORZLIB_GETUNCOMPRESSEDLEN ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__UTILCOMPRESSORZLIB_DECOMPRESS, "_utilCompressorZlib::decompress" )
   INT32 _utilCompressorZlib::decompress( const CHAR *source, UINT32 sourceLen,
                                          CHAR *dest, UINT32 &destLen,
                                          const utilDictHandle dictionary )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORZLIB_DECOMPRESS ) ;
      z_stream stream ;
      BOOLEAN init = FALSE ;

      ossMemset( &stream, 0, sizeof( z_stream ) ) ;
      rc = inflateInit( &stream ) ;
      PD_CHECK( Z_OK == rc, SDB_UTIL_COMPRESS_INIT_FAIL, error, PDERROR,
                "Failed to initialize inflate, rc: %d", rc ) ;
      init = TRUE ;

      stream.next_in = (Bytef *)(source + sizeof(UINT32)) ;
      stream.avail_in = sourceLen - sizeof(UINT32) ;
      stream.next_out = (Bytef *)dest ;
      stream.avail_out = destLen ;

      while ( ( rc = inflate( &stream, Z_FINISH ) ) == Z_OK )
         ;

      PD_CHECK( Z_STREAM_END == rc, SDB_UTIL_DECOMPRESS_FAIL,
                error, PDERROR,
                "Failed to decompress data with zlib, rc: %d", rc ) ;

      destLen = stream.total_out ;
      rc = SDB_OK ;

   done:
      if ( init )
      {
         (void)inflateEnd( &stream ) ;
      }
      PD_TRACE_EXIT( SDB__UTILCOMPRESSORZLIB_DECOMPRESS ) ;
      return rc ;
   error:
      goto done ;
   }
}

