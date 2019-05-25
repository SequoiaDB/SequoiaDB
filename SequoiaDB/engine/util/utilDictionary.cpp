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

