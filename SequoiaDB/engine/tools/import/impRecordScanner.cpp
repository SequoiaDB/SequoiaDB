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

   Source File Name = impRecordScanner.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordScanner.hpp"
#include "ossUtil.h"
#include "pd.hpp"

namespace import
{
   static inline BOOLEAN _startWith(const CHAR* data, INT32 dataLen,
                                    const CHAR* str, INT32 strLen)
   {
      SDB_ASSERT(dataLen > 0, "dataLen must be greater than 0");
      SDB_ASSERT(strLen > 0, "strLen must be greater than 0");

      if (data[0] == str[0])
      {
         // accelerate for single character
         if (1 == strLen)
         {
            return TRUE;
         }
         else if (dataLen >= strLen && 0 == ossStrncmp(data, str, strLen))
         {
            return TRUE;
         }
      }

      return FALSE;
   }

   RecordScanner::RecordScanner(const string& recordDelimiter,
                                const string& stringDelimiter,
                                INPUT_FORMAT format,
                                BOOLEAN linePriority)
   : _recordDelimiter(recordDelimiter),
     _stringDelimiter(stringDelimiter),
     _format(format),
     _linePriority(linePriority)
   {
      SDB_ASSERT(FORMAT_CSV == format || FORMAT_JSON == format, "invalid format");
   }

   RecordScanner::~RecordScanner()
   {
   }

   INT32 RecordScanner::scan(const CHAR* data, INT32 length, BOOLEAN final,
                             INT32& recordLength)
   {
      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if (FORMAT_CSV == _format)
      {
         return _scanCSV(data, length, final, recordLength);
      }
      else if (FORMAT_JSON == _format)
      {
         return _scanJSON(data, length, final, recordLength);
      }

      return SDB_INVALIDARG;
   }

   INT32 RecordScanner::_scanCSV(const CHAR* data, INT32 length, BOOLEAN final,
                                 INT32& recordLength)
   {
      const CHAR* recDel = _recordDelimiter.c_str();
      const INT32 recDelLen = _recordDelimiter.length();
      INT32 len = length;
      CHAR* str = (CHAR*)data;
      INT32 rc = SDB_EOF;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(recDelLen > 0, "recDelLen must be greater than 0");

      // no need to consider string
      if (_linePriority || _stringDelimiter.empty())
      {
         while(len > 0)
         {
            if (_startWith(str, len, recDel, recDelLen))
            {
               recordLength = length - len;
               *str = '\0';
               rc = SDB_OK;
               break;
            }

            len--;
            str++;
         }
      }
      // should consider string
      else
      {
         const CHAR* strDel = _stringDelimiter.c_str();
         const INT32 strDelLen = _stringDelimiter.length();
         BOOLEAN inString = FALSE;

         while (len > 0)
         {
            if (!inString)
            {
               if (_startWith(str, len, strDel, strDelLen))
               {
                  inString = TRUE;
                  len -= strDelLen;
                  str += strDelLen;
                  continue;
               }

               if (_startWith(str, len, recDel, recDelLen))
               {
                  recordLength = length - len;
                  *str = '\0';
                  rc = SDB_OK;
                  break;
               }
            }
            else // in string
            {
               if (_startWith(str, len, strDel, strDelLen))
               {
                  // previous character is escape
                  /*if ('\\' == *(str - 1))
                  {
                     len -= strDelLen;
                     str += strDelLen;
                     continue;
                  }*/

                  len -= strDelLen;
                  str += strDelLen;

                  // two consecutive string delimiter
                  if (len > 0 && _startWith(str, len, strDel, strDelLen))
                  {
                     len -= strDelLen;
                     str += strDelLen;
                     continue;
                  }

                  // the string is end
                  inString = FALSE;
                  continue;
               }
            }

            len--;
            str++;
         }
      }

      if (SDB_EOF == rc && final)
      {
         recordLength = length;
         // it's safe to terminate the string
         str = (CHAR*)data + length;
         *str = '\0';
         rc = SDB_OK;
      }

      return rc;
   }

   #define SCANNER_QUOTES_NONE   0
   #define SCANNER_QUOTES        1
   #define SCANNER_QUOTES_DOUBLE 2
   INT32 RecordScanner::_scanJSON(const CHAR* data, INT32 length, BOOLEAN final,
                                  INT32& recordLength)
   {
      INT32 rc = SDB_EOF;
      INT32 level = 0;
      INT32 len   = length;
      INT32 stringType = SCANNER_QUOTES_NONE ;
      INT32 recDelLen  = _recordDelimiter.length();
      BOOLEAN hasJson  = FALSE;
      CHAR* str = (CHAR*)data;
      const CHAR* recDel = _recordDelimiter.c_str();

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      // no need to consider string
      if ( _linePriority )
      {
         while(len > 0)
         {
            if ( _startWith(str, len, recDel, recDelLen ) )
            {
               recordLength = length - len ;
               *str = '\0' ;
               rc = SDB_OK ;
               break ;
            }

            len-- ;
            str++ ;
         }
      }
      // should consider string
      else
      {
         while (len > 0)
         {
            switch (*str)
            {
            case '{':
               if( stringType == SCANNER_QUOTES_NONE )
               {
                  level++;
                  hasJson = TRUE;
               }
               break;
            case '}':
               if( stringType == SCANNER_QUOTES_NONE )
               {
                  level--;
               }
               break;
            case '\'':
               if( stringType == SCANNER_QUOTES_NONE )
               {
                  stringType = SCANNER_QUOTES ;
               }
               else if( stringType == SCANNER_QUOTES )
               {
                  stringType = SCANNER_QUOTES_NONE ;
               }
               break ;
            case '\"':
               if( stringType == SCANNER_QUOTES_NONE )
               {
                  stringType = SCANNER_QUOTES_DOUBLE ;
               }
               else if( stringType == SCANNER_QUOTES_DOUBLE )
               {
                  stringType = SCANNER_QUOTES_NONE ;
               }
               break;
            case '\\':
               // escape char, so skip one more char
               str++;
               len--;
               break;
            default:
               break;
            }

            str++;
            len--;

            // json is closed
            if (hasJson && level == 0)
            {
               rc = SDB_OK;
               break;
            }
         }
      }

      if (SDB_OK == rc)
      {
         // skip chars until next json
         while (len > 0)
         {
            if ('{' == *str)
            {
               break;
            }

            str++;
            len--;
         }

         recordLength = length - len;         
      }
      else if (SDB_EOF == rc && final)
      {
         recordLength = length;
         // it's safe to terminate the string
         str = (CHAR*)data + length;
         *str = '\0';
         rc = SDB_OK;
      }

      return rc;
   }
}
