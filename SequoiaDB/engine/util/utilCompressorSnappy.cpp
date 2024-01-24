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

   Source File Name = utilCompressorSnappy.hpp

   Descriptive Name = Snappy compressor wrapper.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/01/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCompressorSnappy.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "utilTrace.hpp"

namespace engine
{
   _utilCompressorSnappy::_utilCompressorSnappy()
   : _utilCompressor( UTIL_COMPRESSOR_SNAPPY )
   {
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__UTILCOMPRESSORSNAPPY_COMPRESSBOUND, "_utilCompressorSnappy::compressBound")
   INT32 _utilCompressorSnappy::compressBound( UINT32 srcLen,
                                              UINT32 &maxCompressedLen,
                                              const utilDictHandle dictionary )
   {
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORSNAPPY_COMPRESSBOUND ) ;
      (void)dictionary ;

      SDB_ASSERT( UTIL_INVALID_DICT == dictionary,
                  "snappy does not use any dictionary" ) ;
      maxCompressedLen = ( UINT32 )snappy::MaxCompressedLength( srcLen ) ;

      PD_TRACE_EXIT( SDB__UTILCOMPRESSORSNAPPY_COMPRESSBOUND ) ;

      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__UTILCOMPRESSORSNAPPY_COMPRESS, "_utilCompressorSnappy::compress")
   INT32 _utilCompressorSnappy::compress( const CHAR *source, UINT32 sourceLen,
                                          CHAR *dest, UINT32 &destLen,
                                          const utilDictHandle dictionary,
                                          const utilCompressStrategy *strategy )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORSNAPPY_COMPRESS ) ;
      size_t resultLen = 0 ;
      (void)dictionary ;
      (void)strategy ;

      SDB_ASSERT( UTIL_INVALID_DICT == dictionary,
                  "snappy does not use any dictionary" ) ;
      SDB_ASSERT( destLen >= (UINT32)snappy::MaxCompressedLength( sourceLen ),
                  "Buffer for decompressed data is not big enough" ) ;

      snappy::RawCompress ( source, (size_t)sourceLen, dest, &resultLen ) ;
      destLen = ( UINT32 )resultLen ;

      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORSNAPPY_COMPRESS, rc ) ;
      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__UTILCOMPRESSORSNAPPY_GETUNCOMPRESSEDLEN, "_utilCompressorSnappy::getUncompressedLen")
   INT32 _utilCompressorSnappy::getUncompressedLen( const CHAR *source,
                                                    UINT32 sourceLen,
                                                    UINT32 &length)
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORSNAPPY_GETUNCOMPRESSEDLEN ) ;
      size_t resultLen = 0 ;
      if ( !snappy::GetUncompressedLength ( source, sourceLen, &resultLen ) )
      {
         rc = SDB_CORRUPTED_RECORD ;
         PD_LOG( PDERROR, "Failed to get uncompressed length" ) ;
         goto error ;
      }

      length = resultLen ;
   done:
      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORSNAPPY_GETUNCOMPRESSEDLEN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION( SDB__UTILCOMPRESSORSNAPPY_DECOMPRESS, "_utilCompressorSnappy::decompress")
   INT32 _utilCompressorSnappy::decompress( const CHAR *source,
                                            UINT32 sourceLen,
                                            CHAR *dest,
                                            UINT32 &destLen,
                                            const utilDictHandle dictionary )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__UTILCOMPRESSORSNAPPY_DECOMPRESS ) ;

      UINT32 unCompLen = 0 ;

      SDB_ASSERT( UTIL_INVALID_DICT == dictionary,
                 "snappy does not use any dictionary" ) ;

      if ( SDB_OK != getUncompressedLen( source, sourceLen, unCompLen ) ||
           destLen < unCompLen ||
           !snappy::RawUncompress ( source, (size_t)sourceLen, dest ) )
      {
         rc = SDB_CORRUPTED_RECORD ;
         PD_LOG( PDERROR, "Failed to uncompress record" )  ;
         goto error ;
      }
      destLen = unCompLen ;

   done:
      PD_TRACE_EXITRC( SDB__UTILCOMPRESSORSNAPPY_DECOMPRESS, rc ) ;
      return rc ;
   error:
      goto done ;
   }
}

