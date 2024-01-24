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

   Source File Name = utilDictionary.hpp

   Descriptive Name = Uniform dictionary definitions.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          04/23/2016  YSD Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef UTIL_DICTIONARY
#define UTIL_DICTIONARY

#include "core.hpp"
#include "oss.hpp"

namespace engine
{
   /* Dictionary type. Currently only lzw is supported. */
   enum UTIL_DICT_TYPE
   {
      UTIL_DICT_LZW = 1,
      UTIL_DICT_INVALID
   } ;

   #define UTIL_INVALID_DICT_VERSION      0xFF
   #define UTIL_LZW_DICT_VERSION          1
   /*
    * The uniform header structure of dictionaries. Any dictionary should put
    * this structure at the head.
    */
   struct _utilDictHead
   {
      UTIL_DICT_TYPE _type ;
      UINT8 _version ;
      CHAR _reserve[20] ;
   } ;
   typedef _utilDictHead utilDictHead ;

   struct _utilDictionaryDetail
   {
      UTIL_DICT_TYPE _type ;
      UINT32 _maxCode ;
      UINT8 _version ;
      UINT8 _codeSize ;
      UINT8 _varLenCompEnable ;
   } ;
   typedef _utilDictionaryDetail utilDictionaryDetail ;

   class _utilDictCreator : public SDBObject
   {
   public:
      virtual ~_utilDictCreator() {}
      virtual INT32 prepare() = 0 ;
      virtual void reset() = 0 ;
      virtual void build( const CHAR *src, UINT32 srcLen, BOOLEAN &full ) = 0 ;
      virtual INT32 finalize( CHAR *buff, UINT32 &size ) = 0 ;
   } ;
   typedef _utilDictCreator utilDictCreator ;

   void getDictionaryDetail( void *dictionary, utilDictionaryDetail &detail ) ;
}

#endif /* UTIL_DICTIONARY */

