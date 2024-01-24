/******************************************************************************

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

   Source File Name = mthDef.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          15/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef MTH_DEF_HPP_
#define MTH_DEF_HPP_

namespace engine
{
   #define MTH_S_PREFIX "$"
   #define MTH_S_INCLUDE        MTH_S_PREFIX"include"
   #define MTH_S_DEFAULT        MTH_S_PREFIX"default"
   #define MTH_S_SLICE          MTH_S_PREFIX"slice"
   #define MTH_S_ELEMMATCH      MTH_S_PREFIX"elemMatch"
   #define MTH_S_ELEMMATCHONE   MTH_S_PREFIX"elemMatchOne"
   #define MTH_S_ALIAS          MTH_S_PREFIX"alias"
   #define MTH_S_ABS            MTH_S_PREFIX"abs"
   #define MTH_S_CEILING        MTH_S_PREFIX"ceiling"
   #define MTH_S_FLOOR          MTH_S_PREFIX"floor"
   #define MTH_S_MOD            MTH_S_PREFIX"mod"
   #define MTH_S_ADD            MTH_S_PREFIX"add"
   #define MTH_S_SUBTRACT       MTH_S_PREFIX"subtract"
   #define MTH_S_MULTIPLY       MTH_S_PREFIX"multiply"
   #define MTH_S_DIVIDE         MTH_S_PREFIX"divide"
   #define MTH_S_SUBSTR         MTH_S_PREFIX"substr"
   #define MTH_S_STRLEN         MTH_S_PREFIX"strlen"
   #define MTH_S_LOWER          MTH_S_PREFIX"lower"
   #define MTH_S_UPPER          MTH_S_PREFIX"upper"
   #define MTH_S_LTRIM          MTH_S_PREFIX"ltrim"
   #define MTH_S_RTRIM          MTH_S_PREFIX"rtrim"
   #define MTH_S_TRIM           MTH_S_PREFIX"trim"
   #define MTH_S_CAST           MTH_S_PREFIX"cast"
   #define MTH_S_TYPE           MTH_S_PREFIX"type"
   #define MTH_S_SIZE           MTH_S_PREFIX"size"
   #define MTH_S_STRLENBYTES    MTH_S_PREFIX"strlenBytes"
   #define MTH_S_STRLENCP       MTH_S_PREFIX"strlenCP"
   #define MTH_S_KEYSTRING      MTH_S_PREFIX"keyString"

   typedef UINT32 MTH_S_ATTRIBUTE ;

   /* attribute bit
   | 1bit                 | 1bit                 | 1bit                        |1bit                           |
   | 1:invalid, 0:invalid | 1:include, 0:exclude | 1:default 0:non-default     |1:projection, 0:non-projection |
   */

   #define MTH_S_ATTR_VALID_BIT          1
   #define MTH_S_ATTR_INCLUDE_BIT        2
   #define MTH_S_ATTR_DEFAULT_BIT        4
   #define MTH_S_ATTR_PROJECTION_BIT     8

   /// attribute value
   #define MTH_S_ATTR_NONE               0
   #define MTH_S_ATTR_EXCLUDE            ( MTH_S_ATTR_VALID_BIT )
   #define MTH_S_ATTR_INCLUDE            ( MTH_S_ATTR_VALID_BIT | MTH_S_ATTR_INCLUDE_BIT )
   #define MTH_S_ATTR_DEFAULT            ( MTH_S_ATTR_INCLUDE | MTH_S_ATTR_DEFAULT_BIT )
   #define MTH_S_ATTR_PROJECTION         ( MTH_S_ATTR_VALID_BIT | MTH_S_ATTR_PROJECTION_BIT )

   #define MTH_ATTR_IS_VALID( attribute ) \
           OSS_BIT_TEST( attribute, MTH_S_ATTR_VALID_BIT )

   #define MTH_ATTR_IS_INCLUDE( attribute ) \
           ( MTH_ATTR_IS_VALID(attribute) && OSS_BIT_TEST( attribute, MTH_S_ATTR_INCLUDE_BIT ))

   #define MTH_ATTR_IS_DEFAULT( attribute ) \
           ( MTH_ATTR_IS_VALID(attribute) && OSS_BIT_TEST( attribute, MTH_S_ATTR_DEFAULT_BIT ))

   #define MTH_ATTR_IS_PROJECTION( attribute ) \
           ( MTH_ATTR_IS_INCLUDE(attribute) && OSS_BIT_TEST(attribute, MTH_S_ATTR_PROJECTION_BIT))

#ifdef _WINDOWS
   #define MTH_TRUNC(x)  ( (x)>0 ? floor(x) : ceil(x) )
   #define MTH_MOD(x,y)\
        ( (x) - ( MTH_TRUNC((x) / (y)) * (y) ) )
#else
   #define MTH_MOD(x,y)\
        ( (x) - ( trunc((x) / (y)) * (y) ) )
#endif //_WINDOWS
}

#endif

