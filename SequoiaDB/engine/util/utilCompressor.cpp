/*******************************************************************************


   Copyright (C) 2011-2016 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = utilCompressor.cpp

   Descriptive Name = Compressor for data compression and decompression.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilCompressorSnappy.hpp"
#include "utilCompressorLZW.hpp"
#include "utilCompressorLZ4.hpp"
#include "utilCompressorZlib.hpp"
#include "msgDef.hpp"

namespace engine
{
   utilCompressor* getCompressorByType( UTIL_COMPRESSOR_TYPE type )
   {
      utilCompressor *compressor = NULL ;

      static utilCompressorSnappy snappyCompressor ;
      static utilCompressorLZW lzwCompressor ;
      static utilCompressorLZ4 lz4Compressor ;
      static utilCompressorZlib zlibCompressor ;
      switch ( type )
      {
         case UTIL_COMPRESSOR_LZW:
            compressor = &lzwCompressor ;
            break ;
         case UTIL_COMPRESSOR_SNAPPY:
            compressor = &snappyCompressor ;
            break;
         case UTIL_COMPRESSOR_LZ4:
            compressor = &lz4Compressor ;
            break;
         case UTIL_COMPRESSOR_ZLIB:
            compressor = &zlibCompressor ;
            break;
         default:
            compressor = NULL ;
      }

      return compressor ;
   }

   const CHAR *utilCompressType2String( UINT8 type )
   {
      const CHAR *pStr = "Invalid" ;
      switch( type )
      {
         case UTIL_COMPRESSOR_SNAPPY :
            pStr = VALUE_NAME_SNAPPY ;
            break ;
         case UTIL_COMPRESSOR_LZW :
            pStr = VALUE_NAME_LZW ;
            break ;
         case UTIL_COMPRESSOR_LZ4 :
            pStr = VALUE_NAME_LZ4 ;
            break ;
         case UTIL_COMPRESSOR_ZLIB :
            pStr = VALUE_NAME_ZLIB ;
            break ;
         default :
            break ;
      }
      return pStr ;
   }

   UTIL_COMPRESSOR_TYPE utilString2CompressType( const CHAR *pStr )
   {
      UTIL_COMPRESSOR_TYPE type = UTIL_COMPRESSOR_INVALID ;
      if ( pStr )
      {
         if ( 0 == ossStrcasecmp( pStr, VALUE_NAME_SNAPPY ) )
         {
            type = UTIL_COMPRESSOR_SNAPPY ;
         }
         else if ( 0 == ossStrcasecmp( pStr, VALUE_NAME_LZW ) )
         {
            type = UTIL_COMPRESSOR_LZW ;
         }
         else if ( 0 == ossStrcasecmp( pStr, VALUE_NAME_LZ4 ) )
         {
            type = UTIL_COMPRESSOR_LZ4 ;
         }
         else if ( 0 == ossStrcasecmp( pStr, VALUE_NAME_ZLIB ) )
         {
            type = UTIL_COMPRESSOR_ZLIB ;
         }
      }

      return type ;
   }

} /* engine */

