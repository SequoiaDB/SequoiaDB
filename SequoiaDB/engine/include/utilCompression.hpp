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

   Source File Name = utilCompression.hpp

   Descriptive Name = util compression definitions

   When/how to use: this program may be used on binary and text-formatted
   versions of DPS component. This file contains code logic for log page
   operations

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          8/13/2016  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_COMPRESSION_HPP_
#define UTIL_COMPRESSION_HPP_

namespace engine
{
   enum UTIL_COMPRESSOR_TYPE
   {
      UTIL_COMPRESSOR_SNAPPY  = 0,
      UTIL_COMPRESSOR_LZW     = 1,
      UTIL_COMPRESSOR_LZ4     = 2,
      UTIL_COMPRESSOR_ZLIB    = 3,

      UTIL_COMPRESSOR_INVALID = 255
   } ;

   enum UTIL_COMPRESSION_LEVEL
   {
      UTIL_COMP_BEST_SPEED          = 1,
      UTIL_COMP_BALANCE             = 2,
      UTIL_COMP_BEST_COMPRESSION    = 3,
   } ;

   #define UTIL_COMPRESS_EMPTY_FLAG       ( 0x00 )
   #define UTIL_COMPRESS_ALTERABLE_FLAG   ( 0x01 )

}

#endif /* UTIL_COMPRESSION_HPP_ */
