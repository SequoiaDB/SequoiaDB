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

   Source File Name = utilLZWDictionary.cpp

   Descriptive Name = Implementation of LZW dictionary.

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/12/2015  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#include "utilDictionary.hpp"
#include "utilLZWDictionary.hpp"

namespace engine
{
   void getDictionaryDetail( void *dictionary,
                             utilDictionaryDetail &detail )
   {
      utilDictHead *head = (utilDictHead*)dictionary ;
      utilLZWDictionary dict ;

      SDB_ASSERT( UTIL_DICT_LZW == head->_type, "Dictionary type invalid" ) ;
      SDB_ASSERT( UTIL_LZW_DICT_VERSION == head->_version,
                  "Ditionary version is invalid" ) ;

      dict.attach( dictionary ) ;

      detail._maxCode = dict.getMaxValidCode();
      detail._codeSize = dict.getCodeSize();
      detail._varLenCompEnable = 1 ;
   }
}

