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

   Source File Name = impCSVRecordParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impCSVRecordParser.hpp"
#include "../client/base64c.h"
#include "ossUtil.h"
#include "impUtil.hpp"
#include "pd.hpp"
#include "utilTypeCast.h"
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>

extern "C" int bson_append_string_not_utf8( bson *b, const char *name, const char *value, int len );

namespace import
{
   /* csv type */
   #define CSV_STR_AUTO             "auto"
   #define CSV_STR_INT              "int"
   #define CSV_STR_INTEGER          "integer"
   #define CSV_STR_LONG             "long"
   #define CSV_STR_BOOL             "bool"
   #define CSV_STR_BOOLEAN          "boolean"
   #define CSV_STR_DOUBLE           "double"
   #define CSV_STR_DECIMAL          "decimal"
   #define CSV_STR_STRING           "string"
   #define CSV_STR_TIMESTAMP        "timestamp"
   #define CSV_STR_AUTO_TIMESTAMP   "autotimestamp"
   #define CSV_STR_DATE             "date"
   #define CSV_STR_AUTO_DATE        "autodate"
   #define CSV_STR_NULL             "null"
   #define CSV_STR_OID              "oid"
   #define CSV_STR_REGEX            "regex"
   #define CSV_STR_BINARY           "binary"
   #define CSV_STR_NUMBER           "number"
   #define CSV_STR_SKIP             "skip"
   #define CSV_STR_STRICT           "strict"

   #define CSV_STR_TRUE       "true"
   #define CSV_STR_FALSE      "false"
   #define CSV_STR_TRUE_SIZE  ((INT32)(sizeof(CSV_STR_TRUE) - 1))
   #define CSV_STR_FALSE_SIZE ((INT32)(sizeof(CSV_STR_FALSE) - 1))

   #define CSV_STR_YES        "yes"
   #define CSV_STR_NO         "no"
   #define CSV_STR_YES_SIZE   ((INT32)(sizeof(CSV_STR_YES) - 1))
   #define CSV_STR_NO_SIZE    ((INT32)(sizeof(CSV_STR_NO) - 1))

   #define CSV_STR_T          "t"
   #define CSV_STR_F          "f"
   #define CSV_STR_T_SIZE     ((INT32)(sizeof(CSV_STR_T) - 1))
   #define CSV_STR_F_SIZE     ((INT32)(sizeof(CSV_STR_F) - 1))

   #define CSV_STR_Y          "y"
   #define CSV_STR_N          "n"
   #define CSV_STR_Y_SIZE     ((INT32)(sizeof(CSV_STR_Y) - 1))
   #define CSV_STR_N_SIZE     ((INT32)(sizeof(CSV_STR_N) - 1))

   #define CSV_STR_INT_SIZE            (sizeof(CSV_STR_INT) - 1)
   #define CSV_STR_INTEGER_SIZE        (sizeof(CSV_STR_INTEGER) - 1)
   #define CSV_STR_LONG_SIZE           (sizeof(CSV_STR_LONG) - 1)
   #define CSV_STR_BOOL_SIZE           (sizeof(CSV_STR_BOOL) - 1)
   #define CSV_STR_BOOLEAN_SIZE        (sizeof(CSV_STR_BOOLEAN) - 1)
   #define CSV_STR_DOUBLE_SIZE         (sizeof(CSV_STR_DOUBLE) - 1)
   #define CSV_STR_DECIMAL_SIZE        (sizeof(CSV_STR_DECIMAL) - 1)
   #define CSV_STR_STRING_SIZE         (sizeof(CSV_STR_STRING) - 1)
   #define CSV_STR_TIMESTAMP_SIZE      (sizeof(CSV_STR_TIMESTAMP) - 1)
   #define CSV_STR_AUTO_TIMESTAMP_SIZE (sizeof(CSV_STR_AUTO_TIMESTAMP) - 1)
   #define CSV_STR_DATE_SIZE           (sizeof(CSV_STR_DATE) - 1)
   #define CSV_STR_AUTO_DATE_SIZE      (sizeof(CSV_STR_AUTO_DATE) - 1)
   #define CSV_STR_NULL_SIZE           (sizeof(CSV_STR_NULL) - 1)
   #define CSV_STR_OID_SIZE            (sizeof(CSV_STR_OID) - 1)
   #define CSV_STR_REGEX_SIZE          (sizeof(CSV_STR_REGEX) - 1)
   #define CSV_STR_BINARY_SIZE         (sizeof(CSV_STR_BINARY) - 1)
   #define CSV_STR_NUMBER_SIZE         (sizeof(CSV_STR_NUMBER) - 1)
   #define CSV_STR_SKIP_SIZE           (sizeof(CSV_STR_SKIP) - 1)
   #define CSV_STR_STRICT_SIZE         (sizeof(CSV_STR_STRICT) - 1)
   #define CSV_STR_TYPE_MIN_SIZE       3

   #define CSV_STR_TYPE_EQ(type, str, len) \
      ((sizeof(type) - 1) == len && ossStrncasecmp(str, type, len) == 0)

   /* key word */
   #define CSV_STR_DEFAULT       "default"
   #define CSV_STR_DEFAULT_SIZE  (sizeof(CSV_STR_DEFAULT) - 1)
   #define CSV_STR_FIELD         "field"

   #define CSV_STR_BACKSLASH     '/'
   #define CSV_STR_EMPTY  ""

   #define CSV_STR_LEFTBRACKET   '('
   #define CSV_STR_RIGHTBRACKET  ')'

   #define CSV_INT32_MAX  (2147483647)
   #define CSV_INT32_MIN  ((INT32)0-2147483648)
   #define CSV_INT64_MAX OSS_SINT64_MAX
   #define CSV_INT64_MIN OSS_SINT64_MIN

   #define RELATIVE_YEAR      1900
   #define RELATIVE_MOD       12
   #define RELATIVE_DAY       31
   #define RELATIVE_HOUR      24
   #define RELATIVE_MIN_SEC   60
   #define RELATIVE_MICRO_SEC 1000000

   #define TIME_FORMAT        (_timestampFormat.c_str())
   #define TIME_FORMAT_LEN    (_timestampFormat.length())
   #define TIME_LAST_YEAR     2038
   #define TIME_START_YEAR    1900
   #define TIME_MAX_NUM       ((INT64)2147443199)
   #define TIME_MIN_NUM       ((INT64)-2147414400)

   #define DATE_FORMAT        (_dateFormat.c_str())
   #define DATE_FORMAT_LEN    (_dateFormat.length())
   #define DATE_START_YEAR    0
   #define DATE_LAST_YEAR     9999
   #define DATE_MAX_NUM       ((INT64)253402271999)
   #define DATE_MIN_NUM       ((INT64)-377705145943)

   #define DOUBLE_PRECISION       (15)
   #define DOUBLE_FRACT_PRECISION (6)
   #define DOUBLE_BOUND           (1.79)
   #define DOUBLE_MAX_EXP         (308)
   #define DOUBLE_MIN_EXP         (-308)

   #define LEFT_BRACKET           ('(')
   #define RIGHT_BRACKET          (')')
   #define DOUBLE_QUOTES          ('"')
   #define COMMA                  (',')

   #define CSV_MAX_STRING_SIZE (1024 * 1024 * 16)

   #define RECORD_ID_NAME "_id"

   static string  _dateFormat;
   static string  _timestampFormat;
   static STR_TRIM_TYPE _stringTrimType = STR_TRIM_NO;
   static BOOLEAN _cast = FALSE;
   static BOOLEAN _ignoreNull = FALSE;
   static BOOLEAN _forceNotUTF8 = FALSE;

   static inline INT32 _str2i(CHAR* data, INT32 dataLength,
                              INT32& value, INT32& valueLength);

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

   static inline BOOLEAN _endWith(const CHAR* data, INT32 dataLen,
                                    const CHAR* str, INT32 strLen)
   {
      SDB_ASSERT(dataLen > 0, "dataLen must be greater than 0");
      SDB_ASSERT(strLen > 0, "strLen must be greater than 0");

      if (data[dataLen - 1] == str[strLen - 1])
      {
         // accelerate for single character
         if (1 == strLen)
         {
            return TRUE;
         }
         else if (dataLen >= strLen && 0 == ossStrncmp(&data[dataLen - strLen], str, strLen))
         {
            return TRUE;
         }
      }

      return FALSE;
   }


   static inline void _skipSpace(CHAR** data, INT32& length)
   {
      CHAR* str = *data;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != str, "str can't be NULL");

      while (length > 0)
      {
         if (!isspace(*str))
         {
            break;
         }

         str++;
         length--;
      }

      *data = str;
   }

   // don't skip if the delimiter is space
   static inline void _skipSpace(CHAR** data, INT32& length,
                                 const CHAR* delimiter, INT32 delLength)
   {
      CHAR* str = *data;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != str, "str can't be NULL");
      SDB_ASSERT(NULL != delimiter, "delimiter can't be NULL");
      SDB_ASSERT(delLength > 0, "delLength must be greater than 0");

      while (length > 0)
      {
         if (!isspace(*str) ||
             (_startWith(str, length, delimiter, delLength)))
         {
            break;
         }

         str++;
         length--;
      }

      *data = str;
   }

   // don't skip if the field and string delimiter is space
   static inline void _skipSpace(CHAR** data, INT32& length,
                                 const CHAR* fieldDelimiter,
                                 INT32 fieldDelimiterLen,
                                 const CHAR* stringDelimiter,
                                 INT32 stringDelimiterLen )
   {
      CHAR* str = *data;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != str, "str can't be NULL");
      SDB_ASSERT(NULL != fieldDelimiter, "delimiter can't be NULL");
      SDB_ASSERT(NULL != stringDelimiter, "delimiter can't be NULL");
      SDB_ASSERT(fieldDelimiterLen > 0, "delLength must be greater than 0");

      while (length > 0)
      {
         if( !isspace( *str ) ||
             ( _startWith(str, length, fieldDelimiter, fieldDelimiterLen ) ) ||
             ( stringDelimiterLen > 0 &&
               _startWith(str, length, stringDelimiter, stringDelimiterLen ) ) )
         {
            break;
         }

         str++;
         length--;
      }

      *data = str;
   }

   static inline BOOLEAN _isValidFieldEnd(const CHAR* data, INT32 length,
                                          const CHAR* fieldDel, INT32 fieldDelLen,
                                          INT32& endLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;

      SDB_ASSERT(NULL != data, "data can't be NULL");

      _skipSpace(&str, len, fieldDel, fieldDelLen);
      if (len == 0)
      {
         endLength = length;
         return TRUE;
      }

      if (_startWith(str, len, fieldDel, fieldDelLen))
      {
         fieldEnd = TRUE;
         endLength = length - len;
         return TRUE;
      }

      return FALSE;
   }

   static inline BOOLEAN _isValidFieldName( CHAR* field, INT32 length )
   {
      CHAR* fieldName = field ;
      INT32 len = length ;
      SDB_ASSERT(NULL != fieldName, "field can't be NULL");

      if ( length <= 0 )
      {
         PD_LOG( PDERROR, "Field name can't be empty" ) ;
         return FALSE ;
      }

      // the first character can't be '$'
      if ( '$' == fieldName[0] )
      {
         PD_LOG( PDERROR, "Field name can't start with '$', field name=%.*s",
                 length, field ) ;
         return FALSE ;
      }

      // the characters can't be invisible or '.'
      while ( len > 0 )
      {
         UINT8 ch = *fieldName ;

         if ( ch <=127 && ( !isprint( ch ) || '.' == ch ) )
         {
            PD_LOG( PDERROR, "Field name can't be invisible char or '.',"
                             " field name=%.*s",
                    length, field ) ;
            return FALSE ;
         }

         fieldName++;
         len--;
      }

      return TRUE;
   }

   static inline CHAR* _CSVTypeToString(CSV_TYPE type)
   {
      switch(type)
      {
      case CSV_TYPE_AUTO:
         return CSV_STR_AUTO;
      case CSV_TYPE_INT:
         return CSV_STR_INT;
      case CSV_TYPE_LONG:
         return CSV_STR_LONG;
      case CSV_TYPE_NUMBER:
         return CSV_STR_NUMBER;
      case CSV_TYPE_DOUBLE:
         return CSV_STR_DOUBLE;
      case CSV_TYPE_DECIMAL:
         return CSV_STR_DECIMAL ;
      case CSV_TYPE_BOOL:
         return CSV_STR_BOOL;
      case CSV_TYPE_STRING:
         return CSV_STR_STRING;
      case CSV_TYPE_TIMESTAMP:
         return CSV_STR_TIMESTAMP;
      case CSV_TYPE_AUTO_TIMESTAMP:
         return CSV_STR_AUTO_TIMESTAMP;
      case CSV_TYPE_DATE:
         return CSV_STR_DATE;
      case CSV_TYPE_AUTO_DATE:
         return CSV_STR_AUTO_DATE;
      case CSV_TYPE_NULL:
         return CSV_STR_NULL;
      case CSV_TYPE_OID:
         return CSV_STR_OID;
      case CSV_TYPE_REGEX:
         return CSV_STR_REGEX;
      case CSV_TYPE_BINARY:
         return CSV_STR_BINARY;
      case CSV_TYPE_SKIP:
         return CSV_STR_SKIP;
      default:
         return "unknown type";
      }
   }

   static inline string _CSVTypeToString(CSV_TYPE type, CSVFieldOpt& opt)
   {
      stringstream ss;

      ss << _CSVTypeToString(type);

      if (CSV_TYPE_DECIMAL == type && opt.hasOpt)
      {
         ss << LEFT_BRACKET
            << opt.opt.decimalOpt.precision
            << COMMA
            << opt.opt.decimalOpt.scale
            << RIGHT_BRACKET;
      }

      return ss.str();
   }

   static INT32 _parseStringLimit( const CHAR* data, INT32 length,
                                   CSVFieldOpt& opt )
   {
      INT32 rc = SDB_OK ;
      INT32 len    = length ;
      INT32 min    = 0 ;
      INT32 max    = -1 ;
      INT32 numLen = 0 ;
      BOOLEAN strict = FALSE ;
      CHAR* str    = (CHAR*)data ;
      SDB_ASSERT( NULL != data, "data can't be NULL" ) ;

      if ( LEFT_BRACKET != *str )
      {
         goto done ;
      }

      ++str ;
      --len ;

      //string(

      _skipSpace( &str, len ) ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format, missing parameters" ) ;
         goto error ;
      }

      if ( RIGHT_BRACKET == *str )
      {
         //string()
         goto done ;
      }

      if ( !isdigit( *str ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string parameters" ) ;
         goto error ;
      }

      rc = _str2i( str, len, min, numLen ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid string parameters" ) ;
         goto error ;
      }

      str += numLen ;
      len -= numLen ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format" ) ;
         goto error ;
      }

      _skipSpace( &str, len ) ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format" ) ;
         goto error ;
      }

      if ( RIGHT_BRACKET == *str )
      {
         //string(x)
         max = min ;
         min = 0 ;
         opt.opt.stringOpt.minLength = min ;
         opt.opt.stringOpt.maxLength = max ;
         opt.opt.stringOpt.strict    = strict ;
         opt.hasOpt = TRUE ;
         goto done ;
      }
      else if ( COMMA != *str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string parameters" ) ;
         goto error ;
      }

      ++str ;
      --len ;

      _skipSpace( &str, len ) ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format" ) ;
         goto error ;
      }

      //string(x,

      if ( 0 == ossStrncasecmp( str, CSV_STR_STRICT, CSV_STR_STRICT_SIZE ) )
      {
         //string(x,strict
         strict = TRUE ;
         max = min ;
         min = 0 ;

         str += CSV_STR_STRICT_SIZE ;
         len -= CSV_STR_STRICT_SIZE ;
      }
      else
      {
         numLen = 0 ;
         rc = _str2i( str, len, max, numLen ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Invalid string max length" ) ;
            goto error ;
         }

         if ( max < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid string max length, "
                             "max length can't be less than 0" ) ;
            goto error ;
         }

         str += numLen ;
         len -= numLen ;

         _skipSpace( &str, len ) ;

         if ( 0 == len )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid string format" ) ;
            goto error ;
         }

         if ( RIGHT_BRACKET == *str )
         {
            //string(x,y)
         }
         else if ( COMMA == *str )
         {
            //string(x,y,
            ++str ;
            --len ;

            _skipSpace( &str, len ) ;

            if ( 0 == len )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            if ( 0 != ossStrncasecmp( str, CSV_STR_STRICT,
                                      CSV_STR_STRICT_SIZE ) )
            {
               rc = SDB_INVALIDARG ;
               goto error ;
            }

            //string(x,y,strict
            strict = TRUE ;
            str += CSV_STR_STRICT_SIZE ;
            len -= CSV_STR_STRICT_SIZE ;
         }
         else
         {
            rc = SDB_INVALIDARG ;
            goto error ;
         }
      }

      _skipSpace( &str, len ) ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( RIGHT_BRACKET != *str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format" ) ;
         goto error ;
      }

      ++str ;
      --len ;

      _skipSpace( &str, len ) ;

      if ( 0 != len )
      {
         rc = SDB_INVALIDARG; 
         PD_LOG( PDERROR, "Invalid string format" ) ;
         goto error ;
      }

      if ( min > max && max > 0 )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "String min length can't be greater than"
                          " max length" ) ;
         goto error ;
      }

      opt.opt.stringOpt.minLength = min ;
      opt.opt.stringOpt.maxLength = max ;
      opt.opt.stringOpt.strict    = strict ;
      opt.hasOpt = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   static INT32 _parseDecimalPrecision(const CHAR* data,
                                       INT32 length,
                                       CSVFieldOpt& opt)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 precision = 0;
      INT32 scale = 0;
      INT32 numLen = 0;
      INT32 rc = SDB_OK;

      if ( LEFT_BRACKET != *str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal format, missing parameters" ) ;
         goto error ;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid decimal format, missing parameters" ) ;
         goto error;
      }

      if ( !isdigit( *str ) )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid decimal parameters" ) ;
         goto error;
      }

      rc = _str2i(str, len, precision, numLen);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid decimal precision" ) ;
         goto error ;
      }

      if ( precision < 1 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal precision, "
                          "precision can't be less than 1" ) ;
         goto error ;
      }

      if ( precision > SDB_DECIMAL_MAX_PRECISION )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal precision, "
                          "precision can't be greater than %d",
                 SDB_DECIMAL_MAX_PRECISION ) ;
         goto error ;
      }

      str += numLen;
      len -= numLen;

      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal parameters" ) ;
         goto error ;
      }

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal parameters" ) ;
         goto error ;
      }

      if (COMMA != *str)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid decimal parameters, missing scale" ) ;
         goto error;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal parameters, missing scale" ) ;
         goto error ;
      }

      numLen = 0;
      rc = _str2i(str, len, scale, numLen);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid decimal scale" ) ;
         goto error ;
      }

      if (scale < 0)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal scale, scale can't be less than 0" );
         goto error ;
      }

      str += numLen;
      len -= numLen;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal format" ) ;
         goto error ;
      }

      if (RIGHT_BRACKET != *str)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal format" ) ;
         goto error ;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if ( 0 != len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid decimal format" ) ;
         goto error ;
      }

      if ( scale > precision )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Decimal scale can't be greater than precision" ) ;
         goto error ;
      }

      opt.opt.decimalOpt.precision = precision;
      opt.opt.decimalOpt.scale = scale;
      opt.hasOpt = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _parseTimestampFmt( const CHAR *data, INT32 length,
                                    CSVFieldOpt &opt )
   {
      INT32 rc = SDB_OK ;
      INT32 len = length;
      INT32 fmtLen = 0 ;
      CHAR* fmtStart = NULL ;
      CHAR* str = (CHAR*)data;

      if ( LEFT_BRACKET != *str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid timestamp format, missing parameters" ) ;
         goto error ;
      }

      str++ ;
      len-- ;

      _skipSpace( &str, len ) ;

      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid timestamp format, missing parameters" ) ;
         goto error ;
      }

      if ( DOUBLE_QUOTES != *str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid timestamp parameter" ) ;
         goto error ;
      }

      str++ ;
      len-- ;

      fmtStart = str ;

      while( TRUE )
      {
         if ( len <= 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid timestamp parameter" ) ;
            goto error ;
         }

         if ( DOUBLE_QUOTES == *str )
         {
            break ;
         }

         str++ ;
         len-- ;
         fmtLen++ ;
      }

      str++ ;
      len-- ;
      
      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid timestamp format" ) ;
         goto error;
      }

      if (RIGHT_BRACKET != *str)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid timestamp format" ) ;
         goto error ;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid timestamp format" ) ;
         goto error;
      }

      {
         string fmtStr( fmtStart, fmtLen ) ;

         rc = checkDateTimeFormat( fmtStr ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Invalid timestamp format" ) ;
            goto error ;
         }

         if ( fmtLen > CSV_TIMESTAMP_FMT_MAX_LEN )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid timestamp parameter, "
                             "parameter length can't greater than %d",
                    CSV_TIMESTAMP_FMT_MAX_LEN ) ;
            goto error ;
         }

         opt.hasOpt = TRUE ;
         opt.opt.timestampOpt.fmtLength = fmtLen ;
         ossStrncpy( opt.opt.timestampOpt.format, fmtStart, fmtLen ) ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static inline INT32 _convertToCSVType(const CHAR* data,
                                         INT32 length,
                                         CSV_TYPE& type,
                                         CSVFieldOpt& opt)
   {
      CHAR* str = (CHAR*)data;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if ( length < CSV_STR_TYPE_MIN_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid csv type" ) ;
         goto error ;
      }

      type = CSV_TYPE_AUTO;

      switch(str[0])
      {
      case 'a':
      case 'A':
         // autodate
         // autotimestamp
         if (CSV_STR_TYPE_EQ(CSV_STR_AUTO_DATE, str, length))
         {
            type = CSV_TYPE_AUTO_DATE;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_AUTO_TIMESTAMP, str, length))
         {
            type = CSV_TYPE_AUTO_TIMESTAMP;
         }
         break;
      case 'b':
      case 'B':
         // bool
         // boolean
         // binary
         if (CSV_STR_TYPE_EQ(CSV_STR_BOOL, str, length) ||
             CSV_STR_TYPE_EQ(CSV_STR_BOOLEAN, str, length))
         {
            type = CSV_TYPE_BOOL;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_BINARY, str, length))
         {
            type = CSV_TYPE_BINARY;
         }
         break;
      case 'd':
      case 'D':
         // date
         // double
         // decimal
         if (CSV_STR_TYPE_EQ(CSV_STR_DATE, str, length))
         {
            type = CSV_TYPE_DATE;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_DOUBLE, str, length))
         {
            type = CSV_TYPE_DOUBLE;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_DECIMAL, str, length))
         {
            type = CSV_TYPE_DECIMAL;
         }
         else if (length > (INT32)CSV_STR_DECIMAL_SIZE &&
                  ossStrncasecmp(str, CSV_STR_DECIMAL, CSV_STR_DECIMAL_SIZE) == 0)
         {
            // decimal(10, 2)
            rc = _parseDecimalPrecision(data + CSV_STR_DECIMAL_SIZE,
                                        length - CSV_STR_DECIMAL_SIZE,
                                        opt);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Invalid decimal type" ) ;
               goto error ;
            }
            type = CSV_TYPE_DECIMAL ;
         }
         break;
      case 'i':
      case 'I':
         // int
         // integer
         if (CSV_STR_TYPE_EQ(CSV_STR_INT, str, length) ||
             CSV_STR_TYPE_EQ(CSV_STR_INTEGER, str, length))
         {
            type = CSV_TYPE_INT;
         }
         break;
      case 'l':
      case 'L':
         // long
         if (CSV_STR_TYPE_EQ(CSV_STR_LONG, str, length))
         {
            type = CSV_TYPE_LONG;
         }
         break;
      case 'n':
      case 'N':
         // null
         // number
         if (CSV_STR_TYPE_EQ(CSV_STR_NULL, str, length))
         {
            type = CSV_TYPE_NULL;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_NUMBER, str, length))
         {
            type = CSV_TYPE_NUMBER;
         }
         break;
      case 'o':
      case 'O':
         // oid
         if (CSV_STR_TYPE_EQ(CSV_STR_OID, str, length))
         {
            type = CSV_TYPE_OID;
         }
         break;
      case 'r':
      case 'R':
         // regex
         if (CSV_STR_TYPE_EQ(CSV_STR_REGEX, str, length))
         {
            type = CSV_TYPE_REGEX;
         }
         break;
      case 's':
      case 'S':
         // string
         // skip
         if ( CSV_STR_TYPE_EQ( CSV_STR_STRING, str, length ) )
         {
            type = CSV_TYPE_STRING ;
         }
         else if ( length > (INT32)CSV_STR_STRING_SIZE &&
                   0 == ossStrncasecmp( str, CSV_STR_STRING,
                                        CSV_STR_STRING_SIZE ) )
         {
            rc = _parseStringLimit( data + CSV_STR_STRING_SIZE,
                                    length - CSV_STR_STRING_SIZE, opt ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Invalid string type" ) ;
               goto error ;
            }

            type = CSV_TYPE_STRING ;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_SKIP, str, length))
         {
            type = CSV_TYPE_SKIP;
         }
         break;
      case 't':
      case 'T':
         // timestamp
         if (CSV_STR_TYPE_EQ(CSV_STR_TIMESTAMP, str, length))
         {
            type = CSV_TYPE_TIMESTAMP;
         }
         else if ( length > (INT32)CSV_STR_TIMESTAMP_SIZE &&
                   ossStrncasecmp( str, CSV_STR_TIMESTAMP,
                                   CSV_STR_TIMESTAMP_SIZE ) == 0 )
         {
            // timestamp("YYYY-MM-DD-HH.mm.ss")
            rc = _parseTimestampFmt( data + CSV_STR_TIMESTAMP_SIZE,
                                     length - CSV_STR_TIMESTAMP_SIZE,
                                     opt ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Invalid timestamp type" ) ;
               goto error ;
            }
            type = CSV_TYPE_TIMESTAMP ;
         }
         break;
      default:
         rc = SDB_INVALIDARG ;
         goto error ;
      }

      if ( CSV_TYPE_AUTO == type )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid csv type" ) ;
         goto error ;
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // _stringToRawXXX used to auto detect field's raw type
   // _stringToXXX used to convert data types

   static inline INT32 _stringToRawNum(const CHAR* data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength, BOOLEAN allowDot = FALSE);

   static inline INT32 _stringToInfinity( const CHAR *data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength ) ;

   static inline INT32 _stringToNan( const CHAR *data, INT32 length,
                                     CSV_TYPE& type, CSVFieldValue& value,
                                     INT32& valueLength ) ;

   static inline INT32 _stringToRawDecimalMax( const CHAR* data, INT32 length,
                                               CSV_TYPE &type ) ;

   static inline INT32 _stringToRawDecimalMin( const CHAR* data, INT32 length,
                                               CSV_TYPE &type ) ;

   static inline INT32 _stringToRawDecimalInf( const CHAR* data, INT32 length,
                                               CSV_TYPE &type ) ;

   static inline INT32 _stringToRawDecimalNan( const CHAR* data, INT32 length,
                                               CSV_TYPE &type ) ;

   // the number is long type,
   // but if the number is overflow, we set it as double
   static inline INT32 _stringToRawNum(const CHAR* data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength, BOOLEAN allowDot)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      UINT64 intNum = 0 ;
      FLOAT64 floatNum = 0.0 ;
      BOOLEAN neg = FALSE;
      UINT64 quo = 0 ; // quoteint
      INT64 rem = 0 ; // remainder
      CHAR* start = NULL ;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if ('-' == *str)
      {
         neg = TRUE;
         str++;
         len--;
      }
      else if ('+' == *str)
      {
         str++;
         len--;
      }

      if (len == 0)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      quo = neg ? ((UINT64)CSV_INT64_MAX + 1) : CSV_INT64_MAX;
      rem = quo % 10;
      quo /= 10;
      intNum = 0;
      floatNum = 0;

      if (allowDot && '.' == *str)
      {
         type = CSV_TYPE_DOUBLE;
         // do not skip '.'
         goto finish;
      }

      type = CSV_TYPE_LONG;
      start = str;

      while (len > 0)
      {
         INT32 ch = *str;

         if (!isdigit(ch))
         {
            break;
         }

         ch -= '0';

         if (CSV_TYPE_LONG == type)
         {
            // overflow
            if (intNum > quo || (intNum == quo && ch > rem))
            {
               type = CSV_TYPE_DOUBLE;
               floatNum = (FLOAT64)intNum;
               floatNum = floatNum * 10 + ch;
            }
            else
            {
               intNum = intNum * 10 + ch;
            }
         }
         else // CSV_TYPE_DOUBLE
         {
            floatNum = floatNum * 10 + ch;
         }

         str++;
         len--;
      }

      if (start == str)
      {
         // no digits
         rc = SDB_INVALIDARG;
         goto error;
      }

   finish:
      if (CSV_TYPE_LONG == type)
      {
         value.longVal = neg ? -(INT64)intNum : (INT64)intNum;
      }
      else // CSV_TYPE_DOUBLE
      {
         value.doubleVal = neg ? -floatNum : floatNum;
      }
      valueLength = length - len;

   done:
      return rc;
   error:
      goto done;
   }

   #define CSV_STR_DECIMAL_MAX "max"
   #define CSV_STR_DECIMAL_MAX2 "MAX"
   #define CSV_STR_DECIMAL_MAX_LEN ((INT32)sizeof(CSV_STR_DECIMAL_MAX)-1)
   static inline INT32 _stringToRawDecimalMax( const CHAR* data, INT32 length,
                                               CSV_TYPE &type )
   {
      INT32 rc = SDB_OK ;

      if( 0 == ossStrncmp( data, CSV_STR_DECIMAL_MAX,
                           CSV_STR_DECIMAL_MAX_LEN ) ||
          0 == ossStrncmp( data, CSV_STR_DECIMAL_MAX2,
                           CSV_STR_DECIMAL_MAX_LEN ) )
      {
         type = CSV_TYPE_DECIMAL ;
      }

      return rc ;
   }

   #define CSV_STR_DECIMAL_MIN "min"
   #define CSV_STR_DECIMAL_MIN2 "MIN"
   #define CSV_STR_DECIMAL_MIN_LEN ((INT32)sizeof(CSV_STR_DECIMAL_MIN)-1)
   static inline INT32 _stringToRawDecimalMin( const CHAR* data, INT32 length,
                                               CSV_TYPE &type )
   {
      INT32 rc = SDB_OK ;

      if( 0 == ossStrncmp( data, CSV_STR_DECIMAL_MIN,
                           CSV_STR_DECIMAL_MIN_LEN ) ||
          0 == ossStrncmp( data, CSV_STR_DECIMAL_MIN2,
                           CSV_STR_DECIMAL_MIN_LEN ) )
      {
         type = CSV_TYPE_DECIMAL ;
      }

      return rc ;
   }

   #define CSV_STR_DECIMAL_INF "inf"
   #define CSV_STR_DECIMAL_INF2 "INF"
   #define CSV_STR_DECIMAL_INF_LEN ((INT32)sizeof(CSV_STR_DECIMAL_INF)-1)
   static inline INT32 _stringToRawDecimalInf( const CHAR* data, INT32 length,
                                               CSV_TYPE &type )
   {
      INT32 rc = SDB_OK ;

      if( length == 4 && data[0] == '-' )
      {
         ++data ;
         --length ;
      }

      if( 0 == ossStrncmp( data, CSV_STR_DECIMAL_INF,
                           CSV_STR_DECIMAL_INF_LEN ) ||
          0 == ossStrncmp( data, CSV_STR_DECIMAL_INF2,
                           CSV_STR_DECIMAL_INF_LEN ) )
      {
         type = CSV_TYPE_DECIMAL ;
      }

      return rc ;
   }

   #define CSV_STR_DECIMAL_NAN "nan"
   #define CSV_STR_DECIMAL_NAN2 "NaN"
   #define CSV_STR_DECIMAL_NAN_LEN ((INT32)sizeof(CSV_STR_DECIMAL_NAN)-1)
   static inline INT32 _stringToRawDecimalNan( const CHAR* data, INT32 length,
                                               CSV_TYPE &type )
   {
      INT32 rc = SDB_OK ;

      if( 0 == ossStrncmp( data, CSV_STR_DECIMAL_NAN,
                           CSV_STR_DECIMAL_NAN_LEN ) ||
          0 == ossStrncmp( data, CSV_STR_DECIMAL_NAN2,
                           CSV_STR_DECIMAL_NAN_LEN ) )
      {
         type = CSV_TYPE_DECIMAL ;
      }

      return rc ;
   }

   static INT32 _stringToRawDecimal(const CHAR* data, INT32 length,
                                    bson_decimal& value, INT32& valueLength)
   {
      INT32 rc = SDB_OK;
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      CHAR* start;
      CHAR end;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if( length == 3 )
      {
         CSV_TYPE tmpType = CSV_TYPE_AUTO;

         rc = _stringToRawDecimalMax( data, length, tmpType ) ;
         if( rc == SDB_OK && tmpType == CSV_TYPE_DECIMAL )
         {
            start = str;
            str += 3 ;
            len -= 3 ;
            goto decimal ;
         }

         rc = _stringToRawDecimalMin( data, length, tmpType ) ;
         if( rc == SDB_OK && tmpType == CSV_TYPE_DECIMAL )
         {
            start = str;
            str += 3 ;
            len -= 3 ;
            goto decimal ;
         }

         rc = _stringToRawDecimalInf( data, length, tmpType ) ;
         if( rc == SDB_OK && tmpType == CSV_TYPE_DECIMAL )
         {
            start = str;
            str += 3 ;
            len -= 3 ;
            goto decimal ;
         }

         rc = _stringToRawDecimalNan( data, length, tmpType ) ;
         if( rc == SDB_OK && tmpType == CSV_TYPE_DECIMAL )
         {
            start = str;
            str += 3 ;
            len -= 3 ;
            goto decimal ;
         }
      }
      else if( length == 4 )
      {
         CSV_TYPE tmpType = CSV_TYPE_AUTO;

         rc = _stringToRawDecimalInf( data, length, tmpType ) ;
         if( rc == SDB_OK && tmpType == CSV_TYPE_DECIMAL )
         {
            start = str;
            str += 4 ;
            len -= 4 ;
            goto decimal ;
         }
      }

      if ('#' == *str)
      {
         str++;
         len--;
      }

      start = str;

      if ('+' == *str || '-' == *str)
      {
         str++;
         len--;
      }

      while (isdigit(*str) && len > 0)
      {
         str++;
         len--;
      }

      if (0 == len)
      {
         goto decimal;
      }

      if ('E' == *str || 'e' == *str)
      {
         str++;
         len--;
         goto exp;
      }

      if ('.' == *str)
      {
         str++;
         len--;
      }

      while (isdigit(*str) && len > 0)
      {
         str++;
         len--;
      }

      if ('E' != *str && 'e' != *str)
      {
         goto decimal;
      }

      str++;
      len--;

   exp:
      if ('+' == *str || '-' == *str)
      {
         str++;
         len--;
      }

      while (isdigit(*str) && len > 0)
      {
         str++;
         len--;
      }

   decimal:
      end = *str;
      *str = '\0';

      rc = sdb_decimal_from_str( start, &value);
      if (0 != rc)
      {
         *str = end;
         rc = SDB_INVALIDARG;
         goto error;
      }

      *str = end;
      valueLength = str - data;

   done:
      return rc;
   error:
      goto done;
   }

   #define CSV_STR_INF "inf"
   #define CSV_STR_INF_LEN ((INT32)sizeof(CSV_STR_INF)-1)
   #define CSV_STR_INFINITY "Infinity"
   #define CSV_STR_INFINITY_LEN ((INT32)sizeof(CSV_STR_INFINITY)-1)
   #define CSV_DOUBLE_INFINTY ((1.79769e+308)*2)
   static inline INT32 _stringToInfinity( const CHAR *data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength )
   {
      INT32 rc = SDB_OK ;
      INT32 len = length ;
      BOOLEAN neg = FALSE ;
      CHAR *pStr = (CHAR *)data ;

      if( '+' == *pStr )
      {
         ++pStr ;
         --len ;
      }
      else if( '-' == *pStr )
      {
         neg = TRUE ;
         ++pStr ;
         --len ;
      }
      if( 'I' == *pStr && len >= CSV_STR_INFINITY_LEN )
      {
         if( 0 == ossStrncmp( CSV_STR_INFINITY, pStr, CSV_STR_INFINITY_LEN ) )
         {
            len -= CSV_STR_INFINITY_LEN ;
            type = CSV_TYPE_DOUBLE ;
            value.doubleVal = neg ? -CSV_DOUBLE_INFINTY : CSV_DOUBLE_INFINTY ;
            valueLength = length - len ;
         }
      }
      else if( 'i' == *pStr && len >= CSV_STR_INF_LEN )
      {
         if( 0 == ossStrncmp( CSV_STR_INF, pStr, CSV_STR_INF_LEN ) )
         {
            len -= CSV_STR_INF_LEN ;
            type = CSV_TYPE_DOUBLE ;
            value.doubleVal = neg ? -CSV_DOUBLE_INFINTY : CSV_DOUBLE_INFINTY ;
            valueLength = length - len ;
         }
      }

      return rc ;
   }

   #define CSV_STR_NAN1 "NAN"
   #define CSV_STR_NAN2 CSV_STR_DECIMAL_NAN2
   #define CSV_STR_NAN3 CSV_STR_DECIMAL_NAN
   #define CSV_STR_NAN_LEN ((INT32)sizeof(CSV_STR_NAN1)-1)
   #define CSV_VALUE_NAN (0x7ff8000000000000)
   static inline INT32 _stringToNan( const CHAR *data, INT32 length,
                                     CSV_TYPE& type, CSVFieldValue& value,
                                     INT32& valueLength )
   {
      INT32 rc = SDB_OK ;
      INT32 len = length ;
      CHAR *pStr = (CHAR *)data ;
      INT64 tmp = CSV_VALUE_NAN ;

      if( len >= CSV_STR_NAN_LEN )
      {
         if( 0 == ossStrncmp( CSV_STR_NAN1, pStr, CSV_STR_NAN_LEN ) ||
             0 == ossStrncmp( CSV_STR_NAN2, pStr, CSV_STR_NAN_LEN ) ||
             0 == ossStrncmp( CSV_STR_NAN3, pStr, CSV_STR_NAN_LEN ) )
         {
            len -= CSV_STR_NAN_LEN ;
            type = CSV_TYPE_DOUBLE ;
            ossMemcpy( &value.doubleVal, &tmp, sizeof(FLOAT64) ) ;
            valueLength = length - len ;
         }
      }

      return rc ;
   }

   static inline INT32 _stringToRawNumber( const CHAR* data, INT32 length,
                                           CSV_TYPE &type,
                                           CSVFieldValue &value,
                                           INT32 &valueLength )
   {
      INT32 rc = SDB_OK ;
      INT32 tmpType = 0 ;
      utilNumberVal tmpValue ;

      //parse double [+/-]inf  [+/-]Infinity
      rc = _stringToInfinity( data, length, type, value, valueLength ) ;
      if ( rc )
      {
         goto error;
      }

      if ( CSV_TYPE_DOUBLE == type && 0 != valueLength )
      {
         goto done ;
      }

      //parse double nan
      rc = _stringToNan( data, length, type, value, valueLength ) ;
      if ( rc )
      {
         goto error ;
      }

      if ( CSV_TYPE_DOUBLE == type && 0 != valueLength )
      {
         goto done ;
      }

      rc = utilStrToNumber( data, length, &tmpType, &tmpValue, &valueLength ) ;
      if ( rc )
      {
         goto error ;
      }

      if( tmpType == 0 )
      {
         type = CSV_TYPE_INT ;
         value.intVal = tmpValue.intVal ;
      }
      else if( tmpType == 1 )
      {
         type = CSV_TYPE_LONG ;
         value.longVal = tmpValue.longVal ;
      }
      else if( tmpType == 2 )
      {
         type = CSV_TYPE_DOUBLE ;
         value.doubleVal = tmpValue.doubleVal ;
      }
      else if( tmpType == 3 )
      {
         type = CSV_TYPE_DECIMAL ;
         sdb_decimal_init( &( value.decimalVal ) ) ;

         rc = _stringToRawDecimal( data, length,
                                   value.decimalVal, valueLength ) ;
         if ( rc )
         {
            goto error ;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // [+|-]<0~9...>
   static inline INT32 _stringToRawInt(const CHAR* data, INT32 length,
                                       INT32& value, INT32& valueLength)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      CSVFieldData tmpField;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawNumber(str, len, tmpField.type, tmpField.value, valueLength);
      if (SDB_OK != rc)
      {
         goto error;
      }

      switch(tmpField.type)
      {
      case CSV_TYPE_INT:
         value = tmpField.value.intVal;
         break;
      case CSV_TYPE_LONG:
         if (_cast)
         {
            value = (INT32)tmpField.value.longVal;
            break;
         }
         // passthrough
      case CSV_TYPE_DOUBLE:
         if (_cast)
         {
            value = (INT32)tmpField.value.doubleVal;
            break;
         }
         // passthrough
      case CSV_TYPE_DECIMAL:
         if (_cast)
         {
            value = (INT32)sdb_decimal_to_int(&(tmpField.value.decimalVal));
            break;
         }
         // passthrough
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // [+|-]<0~9...>
   static inline INT32 _stringToRawLong(const CHAR* data, INT32 length,
                                        INT64& value, INT32& valueLength)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      CSVFieldData tmpField;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawNumber(str, len, tmpField.type, tmpField.value, valueLength);
      if (SDB_OK != rc)
      {
         goto error;
      }

      switch(tmpField.type)
      {
      case CSV_TYPE_INT:
         value = (INT64)tmpField.value.intVal;
         break;
      case CSV_TYPE_LONG:
         value = tmpField.value.longVal;
         break;
      case CSV_TYPE_DOUBLE:
         if (_cast)
         {
            value = (INT64)tmpField.value.doubleVal;
            break;
         }
         // passthrough
      case CSV_TYPE_DECIMAL:
         if (_cast)
         {
            value = (INT64)sdb_decimal_to_long(&(tmpField.value.decimalVal));
            break;
         }
         // passthrough
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // [+|-]<0~9...>[.<0~9...>[<E|e>[+|-]<0~9...>]]
   // -123.45e-678
   static inline INT32 _stringToRawDouble(const CHAR* data, INT32 length,
                                          FLOAT64& value, INT32& valueLength)
   {
      INT32 rc = SDB_OK;
      CSVFieldData subField;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawNumber(data, length, subField.type, subField.value, valueLength);
      if (SDB_OK != rc)
      {
         goto error;
      }

      switch(subField.type)
      {
      case CSV_TYPE_DOUBLE:
         value = subField.value.doubleVal;
         break;
      case CSV_TYPE_INT:
         value = (FLOAT64)subField.value.intVal;
         break;
      case CSV_TYPE_LONG:
         value = (FLOAT64)subField.value.longVal;
         break;
      case CSV_TYPE_DECIMAL:
         value = (FLOAT64)sdb_decimal_to_double(&(subField.value.decimalVal));
         break;
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   // <true|false|t|f|yes|no|y|n>
   static inline INT32 _stringToRawBool(const CHAR* data, INT32 length,
                                        BOOLEAN& value, INT32& valueLength)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if (length >= CSV_STR_TRUE_SIZE &&
          ossStrncasecmp(data, CSV_STR_TRUE, CSV_STR_TRUE_SIZE) == 0)
      {
         value = TRUE;
         valueLength = CSV_STR_TRUE_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_FALSE_SIZE &&
               ossStrncasecmp(data, CSV_STR_FALSE, CSV_STR_FALSE_SIZE) == 0)
      {
         value = FALSE;
         valueLength = CSV_STR_FALSE_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_YES_SIZE &&
               ossStrncasecmp(data, CSV_STR_YES, CSV_STR_YES_SIZE) == 0)
      {
         value = TRUE;
         valueLength = CSV_STR_YES_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_NO_SIZE &&
               ossStrncasecmp(data, CSV_STR_NO, CSV_STR_NO_SIZE) == 0)
      {
         value = FALSE;
         valueLength = CSV_STR_NO_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_T_SIZE &&
               ossStrncasecmp(data, CSV_STR_T, CSV_STR_T_SIZE) == 0)
      {
         value = TRUE;
         valueLength = CSV_STR_T_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_F_SIZE &&
               ossStrncasecmp(data, CSV_STR_F, CSV_STR_F_SIZE) == 0)
      {
         value = FALSE;
         valueLength = CSV_STR_F_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_Y_SIZE &&
               ossStrncasecmp(data, CSV_STR_Y, CSV_STR_Y_SIZE) == 0)
      {
         value = TRUE;
         valueLength = CSV_STR_Y_SIZE;
         goto done;
      }
      else if (length >= CSV_STR_N_SIZE &&
               ossStrncasecmp(data, CSV_STR_N, CSV_STR_N_SIZE) == 0)
      {
         value = FALSE;
         valueLength = CSV_STR_N_SIZE;
         goto done;
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _stringToRawNull(const CHAR* data, INT32 length,
                                 const CHAR* fieldDel, INT32 fieldDelLen,
                                 INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      _skipSpace(&str, len);
      if (len == 0)
      {
         valueLength = length;
         goto done;
      }

      if (0 == ossStrncasecmp(str, CSV_STR_NULL, CSV_STR_NULL_SIZE))
      {
         str += CSV_STR_NULL_SIZE;
         len -= CSV_STR_NULL_SIZE;
      }

      _skipSpace(&str, len);
      if (len == 0)
      {
         valueLength = length;
         goto done;
      }

      if (_startWith(str, len, fieldDel, fieldDelLen))
      {
         fieldEnd = TRUE;
         valueLength = length - len;
         goto done;
      }
      else
      {
         rc = SDB_INVALIDARG;
         goto error;
      }


   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToString(const CHAR* data, INT32 length,
                                       const CHAR* strDel, INT32 strDelLen,
                                       const CHAR* fieldDel, INT32 fieldDelLen,
                                       BOOLEAN autoAddStrDel, CSVString& value,
                                       INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      BOOLEAN inString = FALSE;
      INT32 strDelNum = 0;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      // when *data is: '___"data"___', spaces outside the string
      // delimiter needs to be removed, so we skip the spaces first.
      // but we need to handle those cases: '____' and '___,'
      _skipSpace(&str, len, fieldDel, fieldDelLen, strDel, strDelLen);
      if (len == 0 || _startWith(str, len, fieldDel, fieldDelLen))
      {
          if (autoAddStrDel)
          {
             // spaces skipped in *data are valid data
             value.length = length - len;
             value.str = (CHAR*)data;
          }
          else
          {
             value.length = 0;
          }
          valueLength = length - len;
          fieldEnd = TRUE;
          goto done;
      }

      if (strDelLen > 0 && _startWith(str, len, strDel, strDelLen))
      {
         // skip the string delimiter
         str += strDelLen;
         len -= strDelLen;
         inString = TRUE;
         strDelNum++;
      }

      // point to string head
      value.str = str;

      while (len > 0)
      {
         if ( strDelLen > 0 && _startWith( str, len, strDel, strDelLen ) )
         {
            // spaces outdide the string delimiter are useless data
            // because *data contains string delimiter

            //*(str - strDelLen) = '\0';
            // previous character is escape
            // TODO: process "\\\\"
            /*if ('\\' == *(str - 1))
            {
               len -= strDelLen;
               str += strDelLen;
               continue;
            }*/

            len -= strDelLen;
            str += strDelLen;
            strDelNum++;

            if (len == 0)
            {
               //*(str - strDelLen) = '\0';
               valueLength = length;
               value.length = str - strDelLen - value.str;
               goto done;
            }

            // two consecutive string delimiter
            if ( strDelLen > 0 && _startWith( str, len, strDel, strDelLen ) )
            {
               value.hasEscape = TRUE;
               len -= strDelLen;
               str += strDelLen;
               strDelNum--;
               continue;
            }

            inString = FALSE;
            // terminate the string
            value.length = str - strDelLen - value.str;
            break;
         }

         if (!inString)
         {
            if (_startWith(str, len, fieldDel, fieldDelLen))
            {
               //*str = '\0';
               if ( autoAddStrDel && 0 == strDelNum )
               {
                  // spaces skipped in *data are valid data
                  value.length = str - data;
                  value.str = (CHAR*)data;
               }
               else
               {
                  value.length = str - value.str;
               }
               valueLength = length - len;
               fieldEnd = TRUE;
               goto done;
            }

            /*if (isspace(*str))
            {*/
               //*str = '\0';
               /*value.length = str - value.str;
               str++;
               len--;
               if (len == 0)
               {
                  valueLength = length;
                  goto done;
               }
               break;
            }*/
         }

         str++;
         len--;
      }

      if (0 == len)
      {
         // in this case, the while(len > 0){} loop ends normally
         if (autoAddStrDel && 0 == strDelNum )
         {
            // spaces skipped in *data are valid data
            value.length = str - data;
            value.str = (CHAR*)data;
         }
         else
         {
            value.length = str - value.str;
         }
         valueLength = length;
         goto done;
      }
      else
      {
         // in this case, the while(len > 0){} loop break exits,
         // the *data contains string delimiter, so spaces outside
         // the string delimiter needs to be removed
         _skipSpace(&str, len, fieldDel, fieldDelLen);
         if (len == 0)
         {
            valueLength = length;
            goto done;
         }
         else if (_startWith(str, len, fieldDel, fieldDelLen))
         {
            fieldEnd = TRUE;
            valueLength = length - len;
            goto done;
         }

         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid string format, "
                          "check string escaping and string delimiter" ) ;
         goto error ;
      }

   done:
      if ( SDB_OK == rc && valueLength > CSV_MAX_STRING_SIZE )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "The string is out of length [0-%d]: %d",
                 CSV_MAX_STRING_SIZE, valueLength ) ;
      }
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToInt(const CHAR* data, INT32 length,
                                    const CHAR* strDel, INT32 strDelLen,
                                    const CHAR* fieldDel, INT32 fieldDelLen,
                                    INT32& value, INT32& valueLength,
                                    BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawInt(data, length, value, valueLength);
      if (SDB_OK != rc)
      {
         rc = _stringToRawBool(data, length, (BOOLEAN&)value, valueLength);
      }

      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if ( !_isValidFieldEnd( str, len, fieldDel, fieldDelLen,
                                 tmpLen, fieldEnd ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid int32 value, there are extra "
                             "characters after the value" ) ;
            goto error ;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid int32 value" ) ;
         goto error;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid int32 value, value can't be empty" ) ;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if ( 0 == len )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid int32 value, value can't be empty" ) ;
         goto error ;
      }

      // find out int or bool in string
      rc = _stringToRawInt(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         rc = _stringToRawBool(str, len, (BOOLEAN&)value, tmpLen);
      }

      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid int32 value, "
                          "value conversion to int32 failed" ) ;
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid int32 value, "
                          "there are other characters after the value" ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToLong(const CHAR* data, INT32 length,
                                    const CHAR* strDel, INT32 strDelLen,
                                    const CHAR* fieldDel, INT32 fieldDelLen,
                                    INT64& value, INT32& valueLength,
                                    BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawLong(data, length, value, valueLength);
      if (SDB_OK != rc)
      {
         BOOLEAN boolValue = FALSE;
         rc = _stringToRawBool(data, length, boolValue, valueLength);
         value = boolValue;
      }

      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "Invalid int64 value, there are extra "
                             "characters after the value" ) ;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid int64 value" ) ;
         goto error ;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid int64 value, value can't be empty" ) ;
         goto error ;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid int64 value, value can't be empty" ) ;
         goto error ;
      }

      // find out long or bool in string
      rc = _stringToRawLong(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         BOOLEAN boolValue = FALSE;
         rc = _stringToRawBool(str, len, boolValue, tmpLen);
         value = boolValue;
      }

      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid int64 value, "
                          "value conversion to int64 failed" ) ;
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid int64 value, "
                          "there are other characters after the value" ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToBool(const CHAR* data, INT32 length,
                                    const CHAR* strDel, INT32 strDelLen,
                                    const CHAR* fieldDel, INT32 fieldDelLen,
                                    BOOLEAN& value, INT32& valueLength,
                                    BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawBool(data, length, value, valueLength);
      if (SDB_OK != rc)
      {
         INT64 longValue = 0;
         rc = _stringToRawLong(data, length, longValue, valueLength);
         value = (0 != longValue);
      }

      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "Invalid boolean value, there are extra "
                             "characters after the value" ) ;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid boolean value" ) ;
         goto error;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid boolean value, value can't be empty" ) ;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid boolean value, value can't be empty" ) ;
         goto error;
      }

      // find out long or bool in string
      rc = _stringToRawBool(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         INT64 longValue = 0;
         rc = _stringToRawLong(str, len, longValue, tmpLen);
         value = (0 != longValue);
      }

      if (SDB_OK != rc)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid boolean value, "
                          "value conversion to boolean failed" ) ;
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid boolean value, "
                          "there are other characters after the value" ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToNumber(const CHAR* data, INT32 length,
                                    const CHAR* strDel, INT32 strDelLen,
                                    const CHAR* fieldDel, INT32 fieldDelLen,
                                    CSV_TYPE& type, CSVFieldValue& value,
                                    INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawNumber(data, length, type, value, valueLength);
      if (SDB_OK != rc)
      {
         BOOLEAN boolValue = FALSE;
         rc = _stringToRawBool(data, length, boolValue, valueLength);
         type = CSV_TYPE_INT;
         value.intVal = boolValue;
      }

      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid number value, there are extra "
                             "characters after the value" ) ;
            goto error ;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid number value" ) ;
         goto error;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid number value, value can't be empty" ) ;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid number value, value can't be empty" ) ;
         goto error ;
      }

      // find out double in string
      rc = _stringToRawNumber(str, len, type, value, tmpLen);
      if (SDB_OK != rc)
      {
         BOOLEAN boolValue = FALSE;
         rc = _stringToRawBool(data, length, boolValue, valueLength);
         type = CSV_TYPE_INT;
         value.intVal = boolValue;
      }

      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid number value, "
                          "value conversion to number failed" ) ;
         goto error ;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid number value, "
                          "there are other characters after the value" ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToDouble(const CHAR* data, INT32 length,
                                    const CHAR* strDel, INT32 strDelLen,
                                    const CHAR* fieldDel, INT32 fieldDelLen,
                                    FLOAT64& value, INT32& valueLength,
                                    BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToRawDouble(data, length, value, valueLength);
      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid double value, there are extra "
                             "characters after the value" ) ;
            goto error ;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid double value" ) ;
         goto error ;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid double value, value can't be empty" ) ;
         goto error ;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid double value, value can't be empty" ) ;
         goto error ;
      }

      // find out double in string
      rc = _stringToRawDouble(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid double value, "
                          "value conversion to double failed" ) ;
         goto error ;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid double value, "
                          "there are other characters after the value" ) ;
         goto error ;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _stringToDecimal(const CHAR* data, INT32 length,
                                 const CHAR* strDel, INT32 strDelLen,
                                 const CHAR* fieldDel, INT32 fieldDelLen,
                                 CSV_TYPE& type, const CSVFieldOpt& opt,
                                 bson_decimal& value, INT32& valueLength,
                                 BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 tmpLen = 0;
      CSVString csvStr;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if (opt.hasOpt)
      {
         rc = sdb_decimal_init1( &value, opt.opt.decimalOpt.precision,
                                 opt.opt.decimalOpt.scale ) ;
         if (0 != rc)
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid decimal parameter" ) ;
            goto error ;
         }
      }
      else
      {
         sdb_decimal_init(&value);
      }
      rc = _stringToRawDecimal(data, length, value, valueLength);
      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "Invalid decimal value, there are extra "
                             "characters after the value" ) ;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen, FALSE,
                           csvStr, valueLength, fieldEnd);
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid decimal value" ) ;
         goto error;
      }
      else if ( data == csvStr.str )
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid decimal value, value can't be empty" ) ;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         type = CSV_TYPE_NULL ;
         goto done ;
      }

      // find out decimal in string
      if (opt.hasOpt)
      {
         rc = sdb_decimal_init1( &value, opt.opt.decimalOpt.precision,
                                 opt.opt.decimalOpt.scale ) ;
         if (0 != rc)
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "Invalid decimal parameter" ) ;
            goto error;
         }
      }
      else
      {
         sdb_decimal_init(&value);
      }
      rc = _stringToRawDecimal(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid decimal value, "
                          "value conversion to decimal failed" ) ;
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid decimal value, "
                          "there are other characters after the value" ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _stringToNull(const CHAR* data, INT32 length,
                              const CHAR* fieldDel, INT32 fieldDelLen,
                              INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      while (len > 0)
      {
         if (_startWith(str, len, fieldDel, fieldDelLen))
         {
            fieldEnd = TRUE;
            break;
         }

         str++;
         len--;
      }

      valueLength = length - len;

      return rc;
   }

   static INT32 _escapedString(CSVString& value,
                                const CHAR* strDel,
                                INT32 strDelLen)
   {
      INT32 rc = SDB_OK;
      CHAR* escStr = NULL;
      INT32 escLen = 0;
      CHAR* str = value.str;
      INT32 len = value.length;

      SDB_ASSERT(value.hasEscape, "string must has escape char");
      SDB_ASSERT(!value.escaped, "string already escaped");

      escStr = (CHAR*)SDB_OSS_MALLOC(len);
      if (NULL == escStr)
      {
         rc = SDB_OOM;
         PD_LOG( PDERROR, "Failed to allocate memory, size=%d", len ) ;
         goto error;
      }

      while (len > 0)
      {
         if (_startWith(str, len, strDel, strDelLen))
         {
            len -= strDelLen;
            str += strDelLen;

            // escape string delimiter
            if (_startWith(str, len, strDel, strDelLen))
            {
               memcpy(escStr + escLen, str, strDelLen);
               escLen += strDelLen;
               len -= strDelLen;
               str += strDelLen;
               continue;
            }
         }

         escStr[escLen] = *str;
         escLen++;
         len--;
         str++;
      }

      escStr[escLen] = '\0';
      value.escaped = TRUE;
      value.str = escStr;
      value.length = escLen;

   done:
      return rc;
   error:
      goto done;
   }

   static const UINT8 CSV_UTF8_FULL_WIDTH_SPACE[] = {0xE3, 0x80, 0x80, 0x00};
   static const INT32 CSV_UTF8_FULL_WIDTH_SPACE_LEN =
      (INT32)(sizeof(CSV_UTF8_FULL_WIDTH_SPACE) / sizeof(CSV_UTF8_FULL_WIDTH_SPACE[0]) - 1) ;

   static inline INT32 _trimLeft(CHAR*& str, INT32& length, BOOLEAN escaped)
   {
      INT32 rc = SDB_OK;
      CHAR* head = str;
      INT32 len = length;

      SDB_ASSERT(NULL != str, "str can't be NULL");

      while (len > 0)
      {
         if (' ' == *head)
         {
            head++;
            len--;
         }
         else if (_startWith(head, len,
                             (const CHAR *)CSV_UTF8_FULL_WIDTH_SPACE,
                             CSV_UTF8_FULL_WIDTH_SPACE_LEN))
         {
            head = head + CSV_UTF8_FULL_WIDTH_SPACE_LEN;
            len = len - CSV_UTF8_FULL_WIDTH_SPACE_LEN;
         }
         else
         {
            break;
         }
      }

      if (str != head)
      {
         if (escaped)
         {
            // we can not move escaped str pointer,
            // otherwise it will cause panic when free the memory
            CHAR* trimedStr = (CHAR*)SDB_OSS_MALLOC(len + 1);
            if (NULL == trimedStr)
            {
               rc = SDB_OOM;
               PD_LOG( PDERROR, "Failed to allocate memory, size=%d",
                       len + 1 ) ;
               goto error;
            }

            memcpy(trimedStr, head, len);
            trimedStr[len] = '\0';

            SAFE_OSS_FREE(str);

            str = trimedStr;
            length = len;
         } else {
            str = head;
            length = len;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _trimRight(CHAR*& str, INT32& length)
   {
      CHAR* tail = str + length - 1;
      INT32 len = length;

      SDB_ASSERT(NULL != str, "str can't be NULL");

      while (len > 0)
      {
         if (' ' == *tail)
         {
            tail--;
            len--;
         }
         else if (_endWith(str, len,
                           (const CHAR *)CSV_UTF8_FULL_WIDTH_SPACE,
                           CSV_UTF8_FULL_WIDTH_SPACE_LEN))
         {
            tail = tail - CSV_UTF8_FULL_WIDTH_SPACE_LEN;
            len = len - CSV_UTF8_FULL_WIDTH_SPACE_LEN;
         }
         else
         {
            break;
         }
      }

      length = len;

      return SDB_OK;
   }

   static inline INT32 _trimString(CSVString& value, STR_TRIM_TYPE trimType)
   {
      INT32 rc = SDB_OK;
      
      SDB_ASSERT(NULL != value.str, "CSVString.str can't be NULL");

      if (0 == value.length)
      {
         goto done;
      }

      switch (trimType)
      {
      case STR_TRIM_NO:
         break;
      case STR_TRIM_RIGHT:
         rc = _trimRight(value.str, value.length);
         break;
      case STR_TRIM_LEFT:
         rc = _trimLeft(value.str, value.length, value.escaped);
         break;
      case STR_TRIM_BOTH:
         rc = _trimRight(value.str, value.length);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _trimLeft(value.str, value.length, value.escaped);
         break;
      default:
         SDB_ASSERT(FALSE, "Invalid trim type");
      }

   done:
      return rc;
   error:
      goto done;
   }

   // support INT/LONG/DOUBLE/BOOL/NULL/STRING
   static inline INT32 _detectFieldType(const CHAR* data, INT32 length,
                                        const CHAR* strDel, INT32 strDelLen,
                                        const CHAR* fieldDel, INT32 fieldDelLen,
                                        BOOLEAN autoAddStrDel, CSV_TYPE& fieldType,
                                        CSVFieldValue& fieldValue,
                                        INT32& fieldLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      INT32 valueLength = 0;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      // INT/LONG/DOUBLE/BOOL/NULL need skip the header spaces
      _skipSpace(&str, len, fieldDel, fieldDelLen);
      if (len == 0 || _startWith(str, len, fieldDel, fieldDelLen))
      {
         if (autoAddStrDel)
         {
            fieldType = CSV_TYPE_STRING;
            fieldValue.strVal.length = length - len;
            fieldValue.strVal.str = (CHAR*)data;
         }
         else
         {
            fieldType = CSV_TYPE_SKIP;
         }
         fieldEnd = TRUE;
         fieldLength = length - len;
         goto done;
      }

      rc = _stringToRawNumber(str, len, fieldType, fieldValue, valueLength);
      if (SDB_OK != rc)
      {
         fieldType = CSV_TYPE_BOOL;
         rc = _stringToRawBool(str, len, fieldValue.boolVal, valueLength);
      }

      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         _skipSpace(&str, len, fieldDel, fieldDelLen);
         if (len == 0)
         {
            fieldLength = length;
            goto done;
         }
         else if (_startWith(str, len, fieldDel, fieldDelLen))
         {
            fieldEnd = TRUE;
            fieldLength = length - len;
            goto done;
         }

         // reset
         fieldValue.reset( fieldType ) ;
         str = (CHAR*)data;
         len = length;
      }

      // null
      rc = _stringToRawNull(str, len, fieldDel, fieldDelLen, fieldLength, fieldEnd);
      if (SDB_OK == rc)
      {
         fieldType = CSV_TYPE_NULL;
         goto done;
      }

      // string, not need to skip the header spaces
      fieldType = CSV_TYPE_STRING;
      rc = _stringToString(data, length, strDel, strDelLen,
                           fieldDel, fieldDelLen, autoAddStrDel,
                           fieldValue.strVal, fieldLength, fieldEnd);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to parse string, rc=%d", rc ) ;
         goto error;
      }

      if (fieldValue.strVal.hasEscape)
      {
         rc = _escapedString(fieldValue.strVal, strDel, strDelLen);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to escape string, rc=%d", rc ) ;
            goto error;
         }
      }

      if (STR_TRIM_NO != _stringTrimType)
      {
         rc = _trimString(fieldValue.strVal, _stringTrimType);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to trim string, rc=%d", rc ) ;
            goto error;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _str2i(CHAR* data, INT32 dataLength,
                              INT32& value, INT32& valueLength)
   {
      CSV_TYPE type = CSV_TYPE_AUTO;
      CSVFieldValue tmpValue;
      INT32 rc;

      rc = _stringToRawNum(data, dataLength, type, tmpValue, valueLength);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (CSV_TYPE_LONG != type)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (tmpValue.longVal < CSV_INT32_MIN || tmpValue.longVal > CSV_INT32_MAX)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      value = (INT32)tmpValue.longVal;

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToTimeZone( CHAR **ppStr, INT32 &strLen,
                                          INT32 &gmtoff  )
   {
      INT32 rc = SDB_OK ;
      INT32 valueLength = 0;
      INT32 hourStrLen  = 0 ;
      INT32 gmtHour     = 0 ;
      INT32 gmtMinute   = 0 ;
      INT32 countMinute = 0 ;
      CHAR *str = *ppStr ;

      if( !isdigit( str[1] ) || !isdigit( str[2] ) || !isdigit( str[3] ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid time zone format, value=%.*s", 4, str ) ;
         goto error ;
      }

      if( isdigit( str[4] ) )
      {
         // xxxx
         hourStrLen = 2 ;
      }
      else
      {
         // xxx
         hourStrLen = 1 ;
      }

      str++;
      strLen--;

      rc = _str2i( str, hourStrLen, gmtHour, valueLength ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid time zone hour, value=%.*s",
                 hourStrLen, str ) ;
         goto error;
      }
      str += valueLength;
      strLen -= valueLength;

      rc = _str2i( str, 2, gmtMinute, valueLength ) ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Invalid time zone minute, value=%.*s",
                 2, str ) ;
         goto error;
      }
      str += valueLength;
      strLen -= valueLength;

      countMinute = gmtHour * 60 + gmtMinute ;

      if ( IMP_UTIL_TIMEZONE_MAX < countMinute )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid time zone, time zone can't "
                          "greater than %d minutes",
                 IMP_UTIL_TIMEZONE_MAX ) ;
         goto error ;
      }

      gmtoff = countMinute * 60 ;

   done:
      *ppStr = str ;
      return rc ;
   error:
      goto done ;
   }

   /*
    * year: YYYY
    * month: MM
    * day: DD
    * hour: HH
    * minute: mm
    * second: ss
    * millisecond: SSS
    * microsecond: ffffff
    * any charcater: *
    */
   static inline INT32 _stringToDateTime(const CHAR* data, INT32 dataLength,
                                         const CHAR* format, INT32 formatLength,
                                         struct tm* time, INT32& microsec,
                                         BOOLEAN &isLocalTime, INT32 &gmtoff)
   {
      INT32 year = 0;
      INT32 month = 1;
      INT32 day = 1;
      INT32 hour = 0;
      INT32 minute = 0;
      INT32 second = 0;
      INT32 rc = SDB_OK;
      CHAR* str = (CHAR*)data;
      INT32 strLen = dataLength;
      CHAR* fmt = (CHAR*)format;
      INT32 fmtLen = formatLength;
      INT32 valueLength = 0;
      BOOLEAN mxs = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != format, "format can't be NULL");
      SDB_ASSERT(NULL != time, "time can't be NULL");
      SDB_ASSERT(dataLength > 0, "dataLength must be greater than 0");
      SDB_ASSERT(formatLength > 0, "formatLength must be greater than 0");

      _skipSpace(&str, strLen);
      _skipSpace(&fmt, fmtLen);

      while (strLen > 0 && fmtLen > 0)
      {
         switch (*fmt)
         {
         // year: YYYY
         case 'Y':
            SDB_ASSERT('Y' == fmt[0] &&
                       'Y' == fmt[1] &&
                       'Y' == fmt[2] &&
                       'Y' == fmt[3], "Invalid format of year");
            rc = _str2i(str, 4, year, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse year value, value=%.*s, rc=%d",
                       4, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 4;
            fmtLen -= 4;
            break;
         // month: MM
         case 'M':
            SDB_ASSERT('M' == fmt[0] &&
                       'M' == fmt[1], "Invalid format of month");
            rc = _str2i(str, 2, month, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse month value, "
                                "value=%.*s, rc=%d",
                       2, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         // day: DD
         case 'D':
            SDB_ASSERT('D' == fmt[0] &&
                       'D' == fmt[1], "Invalid format of day");
            rc = _str2i(str, 2, day, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse day value, value=%.*s, rc=%d",
                       2, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         // hour: HH
         case 'H':
            SDB_ASSERT('H' == fmt[0] &&
                       'H' == fmt[1], "Invalid format of hour");
            rc = _str2i(str, 2, hour, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse hour value, value=%.*s, rc=%d",
                       2, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         // minute: mm
         case 'm':
            SDB_ASSERT('m' == fmt[0] &&
                       'm' == fmt[1], "Invalid format of minute");
            rc = _str2i(str, 2, minute, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse minute value, "
                                "value=%.*s, rc=%d",
                       2, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         // second: ss
         case 's':
            SDB_ASSERT('s' == fmt[0] &&
                       's' == fmt[1], "Invalid format of second");
            rc = _str2i(str, 2, second, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse second value, "
                                "value=%.*s, rc=%d",
                       2, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         // millisecond: SSS
         case 'S':
            {
               INT32 ms;
               SDB_ASSERT('S' == fmt[0] &&
                          'S' == fmt[1] &&
                          'S' == fmt[2], "Invalid format of millisecond");
               if (mxs)
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG( PDERROR, "Milliseconds and microseconds "
                                   "cannot exist together" ) ;
                  goto error;
               }
               rc = _str2i(str, 3, ms, valueLength);
               if (SDB_OK != rc)
               {
                  PD_LOG( PDERROR, "Failed to parse millisecond value, "
                                   "value=%.*s, rc=%d",
                          3, str, rc ) ;
                  goto error;
               }
               microsec = ms * 1000;
               str += valueLength;
               strLen -= valueLength;
               fmt += 3;
               fmtLen -= 3;
               mxs = TRUE;
            }
            break;
         // microsecond: ffffff
         case 'f':
            SDB_ASSERT('f' == fmt[0] &&
                       'f' == fmt[1] &&
                       'f' == fmt[2] &&
                       'f' == fmt[3] &&
                       'f' == fmt[4] &&
                       'f' == fmt[5], "Invalid format of microsecond");
            if (mxs)
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Milliseconds and microseconds "
                                "can't exist together" ) ;
               goto error ;
            }
            rc = _str2i(str, 6, microsec, valueLength);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse microsecond value, "
                                "value=%.*s, rc=%d",
                       6, str, rc ) ;
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 6;
            fmtLen -= 6;
            mxs = TRUE;
            break;
         // time zone: +/-XXXX
         case 'Z':
         {
            INT32 sign = 1 ;

            fmt++;
            fmtLen--;

            if( '+' == str[0] )
            {
               sign = 1 ;
            }
            else if ( '-' == str[0] )
            {
               sign = -1 ;
            }
            else if ( 'Z' == str[0] )
            {
               isLocalTime = FALSE ;
               break ;
            }
            else
            {
               break ;
            }

            rc = _stringToTimeZone( &str, strLen, gmtoff ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to parse time zone rc=%d", rc ) ;
               goto error ;
            }

            gmtoff *= sign ;
            isLocalTime = FALSE ;

            break ;
         }
         case '+':
         case '-':
         {
            INT32 sign = 1 ;

            if ( '-' == fmt[0] )
            {
               sign = -1 ;
            }

            if ( !isdigit( fmt[1] ) )
            {
               if (*str != *fmt)
               {
                  rc = SDB_INVALIDARG;
                  PD_LOG( PDERROR, "Invalid time zone rc=%d", rc ) ;
                  goto error;
               }
               str++;
               strLen--;
               fmt++;
               fmtLen--;
               break;
            }

            SDB_ASSERT( isdigit(fmt[2]) &&
                        isdigit(fmt[3]), "Invalid format of time zone");

            if ( '+' == str[0] )
            {
               sign = 1 ;
            }
            else if ( '-' == str[0] )
            {
               sign = -1 ;
            }
            else if ( 'Z' == str[0] )
            {
               str++;
               strLen--;
               fmt++;
               fmtLen--;
               isLocalTime = FALSE ;
               break ;
            }
            else
            {
               rc = _stringToTimeZone( &fmt, fmtLen, gmtoff ) ;
               if ( rc )
               {
                  PD_LOG( PDERROR, "Failed to parse time zone rc=%d", rc ) ;
                  goto error ;
               }
               gmtoff *= sign ;
               isLocalTime = FALSE ;
               break ;
            }

            rc = _stringToTimeZone( &str, strLen, gmtoff ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to parse time zone rc=%d", rc ) ;
               goto error ;
            }

            gmtoff *= sign ;
            isLocalTime = FALSE ;

            if ( isdigit( fmt[4] ) )
            {
               fmt += 4 ;
               fmtLen -= 4 ;
            }
            else
            {
               fmt += 3 ;
               fmtLen -= 3 ;
            }

            break;
         }
         // any charcater: *
         case '*':
            str++;
            strLen--;
            fmt++;
            fmtLen--;
            break;
         default:
            if (*str != *fmt)
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid time value: value=%.*s, format=%.*s",
                       dataLength, data, formatLength, format ) ;
               goto error ;
            }
            str++;
            strLen--;
            fmt++;
            fmtLen--;
            break;
         }
      }

      if ( 0 >= strLen && 4 <= fmtLen )
      {
         while( TRUE )
         {
            INT32 sign = 1 ;

            if ( '+' == fmt[0] )
            {
               sign = 1 ;
            }
            else if ( '-' == fmt[0] )
            {
               sign = -1 ;
            }
            else
            {
               break ;
            }

            if ( !isdigit( fmt[1] ) )
            {
               break;
            }

            SDB_ASSERT( isdigit(fmt[2]) &&
                        isdigit(fmt[3]), "Invalid format of time zone");

            rc = _stringToTimeZone( &fmt, fmtLen, gmtoff ) ;
            if ( rc )
            {
               PD_LOG( PDERROR, "Failed to parse time zone rc=%d", rc ) ;
               goto error ;
            }
            gmtoff *= sign ;
            isLocalTime = FALSE ;
            break ;
         }
      }

      ossMemset(time, 0, sizeof(struct tm));
      time->tm_year = year;
      time->tm_mon  = month - 1;
      time->tm_mday = day;
      time->tm_hour = hour;
      time->tm_min  = minute;
      time->tm_sec  = second;

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToTimestamp(CSVString& data, BOOLEAN autoTimestamp,
                                          CSVTimestamp& value, CSVFieldOpt& opt)
   {
      CHAR* str = data.str;
      INT32 rc = SDB_OK;
      BOOLEAN hasNonDigit = FALSE;
      CHAR* term = NULL ;
      CHAR tmpch ;
      CHAR ch;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      if ( data.length <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid timestamp, timestamp can't be empty" ) ;
         goto error ;
      }

      // terminate string
      term = str + data.length;
      tmpch = *term;
      *term = '\0';

      // may be negative number
      if ('-' == *str)
      {
         str++;
      }

      while ((ch = *(str++)) != '\0')
      {
         if (!isdigit(ch))
         {
            hasNonDigit = TRUE;
            break;
         }
      }

      if (hasNonDigit || !autoTimestamp)
      {
         struct tm t ;
         BOOLEAN isLocalTime = TRUE ;
         INT32 microsec = 0;
         INT32 gmtoff = 0 ;
         time_t timep;

         ossMemset(&t, 0, sizeof(t));

         if ( FALSE == opt.hasOpt || 0 == opt.opt.timestampOpt.fmtLength )
         {
            rc = _stringToDateTime(data.str, data.length,
                                   TIME_FORMAT, TIME_FORMAT_LEN,
                                   &t, microsec, isLocalTime, gmtoff);
         }
         else
         {
            rc = _stringToDateTime(data.str, data.length,
                                   opt.opt.timestampOpt.format,
                                   opt.opt.timestampOpt.fmtLength,
                                   &t, microsec, isLocalTime, gmtoff);
         }

         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to scan timestamp, rc=%d", rc);
            goto error;
         }

         /* sanity check */
         if ( t.tm_year > TIME_LAST_YEAR || t.tm_year < TIME_START_YEAR )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "year is out of range[%d~%d]: %d",
                    TIME_START_YEAR, TIME_LAST_YEAR, t.tm_year ) ;
            goto error ;
         }

         if ( t.tm_mon >= RELATIVE_MOD || t.tm_mon < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "month is out of range[%d~%d]: %d",
                    0, RELATIVE_MOD, t.tm_mon ) ;
            goto error ;
         }

         if ( t.tm_mday > RELATIVE_DAY || t.tm_mday <= 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "day is out of range[%d~%d]: %d",
                    1, RELATIVE_DAY, t.tm_mday ) ;
            goto error ;
         }

         if ( t.tm_hour >= RELATIVE_HOUR || t.tm_hour < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "hour is out of range[%d~%d]: %d",
                    0, RELATIVE_HOUR, t.tm_hour ) ;
            goto error ;
         }

         if ( t.tm_min >= RELATIVE_MIN_SEC || t.tm_min < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "minute is out of range[%d~%d]: %d",
                    0, RELATIVE_MIN_SEC, t.tm_min ) ;
            goto error ;
         }

         if ( t.tm_sec >= RELATIVE_MIN_SEC || t.tm_sec < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "second is out of range[%d~%d]: %d",
                    0, RELATIVE_MIN_SEC, t.tm_sec ) ;
            goto error ;
         }

         if ( microsec >= RELATIVE_MICRO_SEC || microsec < 0 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid date of timestamp, "
                             "second is out of range[%d~%d]: %d",
                    0, RELATIVE_MICRO_SEC, microsec ) ;
            goto error ;
         }

         t.tm_year -= RELATIVE_YEAR;

         /* create integer time representation */
         if ( isLocalTime )
         {
            timep = ossMkTime(&t);
         }
         else
         {
#if defined (_WINDOWS)
            timep = _mkgmtime(&t) - gmtoff;
#else
            timep = timegm(&t) - gmtoff;
#endif
         }
         if( !ossIsTimestampValid( timep ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid time of timestamp, "
                             "time is out of range: %d", time ) ;
            goto error ;
         }
         value.sec = (INT32)timep;
         value.us = microsec;
      }
      else
      {
         INT64 varLong = 0;
         INT64 sec = 0;
         INT64 us = 0;
         INT32 valueLength = 0;

         rc = _stringToRawLong(data.str, data.length, varLong, valueLength);
         if ( rc )
         {
            PD_LOG( PDERROR, "Failed to get the number of timestamp, rc=%d",
                    rc ) ;
            goto error;
         }
         else if ( data.length != valueLength )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Failed to get the number of timestamp, rc=%d",
                    rc ) ;
            goto error ;
         }

         sec = varLong / 1000;
         us = (varLong % 1000) * 1000; // microseconds
         if (us < 0)
         {
            // move 1s from sec to us
            sec--;
            us += 1000000;
         }

         if (varLong < TIME_MIN_NUM * 1000 )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The timestamp %lld is less than %lld000",
                    varLong, TIME_MIN_NUM ) ;
            goto error ;
         }

         if ( ( sec > TIME_MAX_NUM ) || ( ( sec == TIME_MAX_NUM ) && us > 0 ) )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "The timestamp %lld is greater than %lld000",
                    varLong, TIME_MAX_NUM ) ;
            goto error ;
         }

         value.sec = (INT32)sec ;
         value.us = (INT32)us ;
      }

   done:
      // recovery string
      if ( NULL != term )
      {
         *term = tmpch;
      }
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToDate(CSVString& data, BOOLEAN autoDate, INT64& value)
   {
      CHAR* str = data.str;
      INT32 rc = SDB_OK;
      BOOLEAN hasNonDigit = FALSE;
      CHAR* term = NULL ;
      CHAR tmpch ;
      CHAR ch;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      if ( data.length <= 0 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid date, date can't be empty" ) ;
         goto error ;
      }

      // terminate string
      term = str + data.length;
      tmpch = *term;
      *term = '\0';

      // may be negative number
      if ('-' == *str)
      {
         str++;
      }

      while ((ch = *(str++)) != '\0')
      {
         if (!isdigit(ch))
         {
            hasNonDigit = TRUE;
            break;
         }
      }

      if (hasNonDigit || !autoDate)
      {
         struct tm t;
         BOOLEAN isLocalTime = TRUE ;
         INT32 microsec = 0;
         INT32 gmtoff = 0 ;
         time_t timep;

         ossMemset(&t, 0, sizeof(t));
         rc = _stringToDateTime(data.str, data.length,
                                DATE_FORMAT, DATE_FORMAT_LEN,
                                &t, microsec, isLocalTime, gmtoff);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to scan date, rc=%d", rc ) ;
            goto error ;
         }

         /* sanity check */
         if (t.tm_year > DATE_LAST_YEAR || t.tm_year < DATE_START_YEAR ||
             t.tm_mon >= RELATIVE_MOD || t.tm_mon < 0 ||
             t.tm_mday > RELATIVE_DAY || t.tm_mday <= 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid date");
            goto error;
         }

         t.tm_year -= RELATIVE_YEAR;

         /* create integer time representation */
         if ( isLocalTime )
         {
            timep = ossMkTime(&t);
         }
         else
         {
#if defined (_WINDOWS)
            timep = _mkgmtime(&t) - gmtoff;
#else
            timep = timegm(&t) - gmtoff;
#endif
         }
         value = (INT64)timep * 1000 + microsec/1000;
      }
      else
      {
         INT32 valueLength = 0;

         rc = _stringToRawLong(data.str, data.length, value, valueLength);
         if (SDB_OK != rc || data.length != valueLength)
         {
            goto error;
         }
      }

   done:
      // recovery string
      if ( NULL != term )
      {
         *term = tmpch;
      }
      return rc;
   error:
      goto done;
   }

   static inline INT32 _stringToOID(CSVString& data, CSVString& value)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      if (data.length != 24)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Invalid oid length");
      }
      else
      {
         value.str = data.str;
         value.length = data.length;
      }

      return rc;
   }

   // "/pattern/<options>"
   static inline INT32 _stringToRegex(CSVString& data, CSVRegex& value)
   {
      CHAR* str = data.str;
      INT32 len = data.length;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      if (len == 0)
      {
         // regex can be empty
         value.pattern = CSV_STR_EMPTY;
         value.patternLen = 0;
         value.option = CSV_STR_EMPTY;
         value.optionLen = 0;
         goto done;
      }

      if (len < 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Invalid regex length");
         goto error;
      }

      if (CSV_STR_BACKSLASH != *str)
      {
         value.pattern = str;
         value.patternLen = len;
         value.option = CSV_STR_EMPTY;
         goto done;
      }

      // skip '/'
      str++;
      len--;
      value.pattern = str;

      while (len > 0)
      {
         if (CSV_STR_BACKSLASH == *str)
         {
            break;
         }

         str++;
         len--;
      }

      if (len == 0 || str - data.str <= 1) // '/...' or '//...'
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Invalid regex format");
         goto error;
      }

      //*str = '\0'; // terminate the pattern
      value.patternLen = str - value.pattern;

      // skip '/'
      str++;
      len--;

      if (len == 0)
      {
         value.option = CSV_STR_EMPTY;
         goto done;
      }

      if (isspace(*str))
      {
         value.option = CSV_STR_EMPTY;
      }
      else
      {
         value.option = str;
      }

      while (len > 0)
      {
         if (isspace(*str))
         {
            break;
         }

         str++;
         len--;
      }

      value.optionLen = str - value.option;
      if (len != 0)
      {
         str++;
         len--;
         _skipSpace(&str, len);
         if (len != 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid regex format");
            goto error;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   // "(type)value"
   static inline INT32 _stringToBinary(CSVString& data, CSVBinary& value)
   {
      CHAR* str = data.str;
      INT32 len = data.length;
      INT32 rc = SDB_OK;
      INT32 base64Len = 0;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      // terminate string
      CHAR* term = str + data.length;
      CHAR tmpch = *term;
      *term = '\0';

      value.bin = NULL;

      if (len <= 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Invalid binary length");
         goto error;
      }

      if (CSV_STR_LEFTBRACKET == *str)
      {
         INT32 type = 0;
         INT32 typeLength = 0;

         // skip '('
         str++;
         len--;

         _skipSpace(&str, len);
         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid binary format");
            goto error;
         }

         rc = _stringToRawInt(str, len, type, typeLength);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Invalid binary type");
            goto error;
         }

         if (type < 0 || type > 255)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Binary type is out of range[0~255], type:%d",
                   type);
            goto error;
         }

         value.type = type;

         // skip the type number
         str += typeLength;
         len -= typeLength;

         _skipSpace(&str, len);
         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid binary format");
            goto error;
         }

         if (CSV_STR_RIGHTBRACKET != *str)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid binary format");
            goto error;
         }

         // skip ')'
         str++;
         len--;

         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Invalid binary format");
            goto error;
         }
      }
      else
      {
         value.type = 0;
      }

      value.str = str;

      base64Len = getDeBase64Size(str);
      if (base64Len < 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "Invalid binary base64 size");
         goto error;
      }

      if (base64Len > 0)
      {
         value.bin = (CHAR*)SDB_OSS_MALLOC(base64Len);
         if (NULL == value.bin)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "Failed to malloc");
            goto error;
         }
         ossMemset(value.bin, 0, base64Len);
         if (0 > base64Decode(str, value.bin, base64Len))
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Failed to decode binary");
            goto error;
         }

         // ignore '\0'
         value.binLen = base64Len - 1;
      }
      else
      {
         value.bin = "";
         value.binLen = 0;
      }

   done:
      // recovery string
      *term = tmpch;
      return rc ;
   error:
      SAFE_OSS_FREE(value.bin);
      goto done ;
   }

   static void _printField(CSVField& field)
   {
      stringstream ss;
      CSV_TYPE type = field.type;

      ss << "id:" << field.id
         << ", name:" << field.name
         << ", type:" << _CSVTypeToString(type, field.opt);
      if (CSV_TYPE_NUMBER == type)
      {
         ss << ", subtype:" << _CSVTypeToString(field.subType, field.opt);
      }
      ss << ", hasDefault:" << field.hasDefault;
      if (field.hasDefault)
      {
         ss << ", default:";

         if (CSV_TYPE_NUMBER == type)
         {
            type = field.subType;
         }

         switch(type)
         {
         case CSV_TYPE_AUTO:
            ss << "auto";
            break;
         case CSV_TYPE_INT:
            ss << field.defaultValue.intVal;
            break;
         case CSV_TYPE_LONG:
            ss << field.defaultValue.longVal;
            break;
         case CSV_TYPE_DOUBLE:
            ss << field.defaultValue.doubleVal;
            break;
         case CSV_TYPE_DECIMAL:
            {
               INT32 len = 0;
               CHAR* buf = NULL;

               sdb_decimal_to_str_get_len(&(field.defaultValue.decimalVal), &len);

               buf = (CHAR*)SDB_OSS_MALLOC(len);
               if (NULL == buf)
               {
                  break;
               }
               ossMemset(buf, 0, len);

               sdb_decimal_to_str(&(field.defaultValue.decimalVal), buf, len);
               ss << buf;
               SDB_OSS_FREE(buf);
            }
            break;
         case CSV_TYPE_NUMBER:
            break;
         case CSV_TYPE_BOOL:
            ss << field.defaultValue.boolVal;
            break;
         case CSV_TYPE_STRING:
            ss << "[" << string(field.defaultValue.strVal.str,
                                field.defaultValue.strVal.length)
               << "](length: " << field.defaultValue.strVal.length << ")";
            break;
         case CSV_TYPE_TIMESTAMP:
            ss << field.defaultValue.timestampVal.sec
               << "."
               << field.defaultValue.timestampVal.us;
            break;
         case CSV_TYPE_DATE:
            ss << field.defaultValue.dateVal;
            break;
         case CSV_TYPE_OID:
            ss << "[" << string(field.defaultValue.oidVal.str,
                                field.defaultValue.oidVal.length)
               << "](length: " << field.defaultValue.oidVal.length << ")";
            break;
         case CSV_TYPE_REGEX:
            ss << "pattern: [";
            if (0 != field.defaultValue.regexVal.patternLen)
            {
                ss << string(field.defaultValue.regexVal.pattern,
                             field.defaultValue.regexVal.patternLen);
            }
            ss << "], option: [";
            if (0 != field.defaultValue.regexVal.optionLen)
            {
               ss << string(field.defaultValue.regexVal.option,
                            field.defaultValue.regexVal.optionLen);
            }
            ss << "]";
            break;
         case CSV_TYPE_BINARY:
            ss << "type: [" << field.defaultValue.binaryVal.type
               << "], str: ["
               << string(field.defaultValue.binaryVal.str,
                         field.defaultValue.binaryVal.binLen)
               << "], binLen: ["
               << field.defaultValue.binaryVal.binLen
               << "]";
            break;
         default:
            ss << "unsupported type";
            break;
         }
      }

      ss << std::endl;
      std::cout << ss.str();
   }

   static inline INT32 _parseFieldValue(const CHAR* data, INT32 length,
                                        const CHAR* fieldDel, INT32 fieldDelLen,
                                        const CHAR* strDel, INT32 strDelLen,
                                        BOOLEAN autoAddStrDel, CSV_TYPE& type,
                                        CSV_TYPE& subType, CSVField* field,
                                        CSVFieldValue& fieldValue,
                                        INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;
      BOOLEAN autoDateTime = FALSE;
      CSVFieldOpt &opt = field->opt ;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if (_startWith(str, len, fieldDel, fieldDelLen))
      {
         type = CSV_TYPE_SKIP ;
         valueLength = 0;
         fieldEnd = TRUE;
         goto done;
      }

      switch(type)
      {
      case CSV_TYPE_INT:
         rc = _stringToInt(data, length, strDel, strDelLen,
                           fieldDel, fieldDelLen, fieldValue.intVal,
                           valueLength, fieldEnd);
         break;
      case CSV_TYPE_LONG:
         rc = _stringToLong(data, length, strDel, strDelLen,
                            fieldDel, fieldDelLen, fieldValue.longVal,
                            valueLength, fieldEnd);
         break;
      case CSV_TYPE_NUMBER:
         rc = _stringToNumber(data, length, strDel, strDelLen,
                              fieldDel, fieldDelLen, subType, fieldValue,
                              valueLength, fieldEnd);
         break;
      case CSV_TYPE_DOUBLE:
         rc = _stringToDouble(data, length, strDel, strDelLen,
                              fieldDel, fieldDelLen, fieldValue.doubleVal,
                              valueLength, fieldEnd);
         break;
      case CSV_TYPE_DECIMAL:
         rc = _stringToDecimal(data, length, strDel, strDelLen,
                               fieldDel, fieldDelLen, type,
                               opt, fieldValue.decimalVal,
                               valueLength, fieldEnd);
         break;
      case CSV_TYPE_BOOL:
         rc = _stringToBool(data, length, strDel, strDelLen,
                            fieldDel, fieldDelLen, fieldValue.boolVal,
                            valueLength, fieldEnd);
         break;
      case CSV_TYPE_NULL:
         rc = _stringToNull(data, length, fieldDel, fieldDelLen,
                            valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to parse null type, rc=%d", rc ) ;
            goto error;
         }
         goto done;
      case CSV_TYPE_STRING:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, autoAddStrDel, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to parse string, rc=%d", rc ) ;
            goto error;
         }
         if (fieldValue.strVal.hasEscape)
         {
            rc = _escapedString(fieldValue.strVal, strDel, strDelLen);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to escape string, rc=%d", rc ) ;
               goto error;
            }
         }
         if (STR_TRIM_NO != _stringTrimType)
         {
            rc = _trimString(fieldValue.strVal, _stringTrimType);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to trim string, rc=%d", rc ) ;
               goto error;
            }
         }
         if ( opt.hasOpt )
         {
            if ( fieldValue.strVal.length < opt.opt.stringOpt.minLength )
            {
               if ( field->hasDefault )
               {
                  type = CSV_TYPE_NULL ;
               }
               else
               {
                  if ( opt.opt.stringOpt.strict )
                  {
                     rc = SDB_INVALIDARG ;
                     goto error ;
                  }
                  else
                  {
                     type = CSV_TYPE_NULL ;
                  }
               }
            }
            else if ( opt.opt.stringOpt.maxLength > 0 &&
                      fieldValue.strVal.length > opt.opt.stringOpt.maxLength )
            {
               if ( TRUE == opt.opt.stringOpt.strict )
               {
                  rc = SDB_INVALIDARG ;
                  goto error ;
               }
               else
               {
                  fieldValue.strVal.length = opt.opt.stringOpt.maxLength ;
               }
            }
         }
         goto done;
      case CSV_TYPE_AUTO_TIMESTAMP:
         autoDateTime = TRUE;
         // pass through
      case CSV_TYPE_TIMESTAMP:
         {
            BOOLEAN tmpFieldEnd = FALSE ;
            // null
            rc = _stringToRawNull( data, length, fieldDel, fieldDelLen,
                                   valueLength, tmpFieldEnd ) ;
            if( SDB_OK == rc )
            {
               type = CSV_TYPE_NULL;
               fieldEnd = tmpFieldEnd ;
               goto done;
            }
         }
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToTimestamp(fieldValue.strVal, autoDateTime, fieldValue.timestampVal, opt);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_AUTO_DATE:
         autoDateTime = TRUE;
         // pass through
      case CSV_TYPE_DATE:
         {
            BOOLEAN tmpFieldEnd = FALSE ;
            // null
            rc = _stringToRawNull( data, length, fieldDel, fieldDelLen,
                                   valueLength, tmpFieldEnd ) ;
            if( SDB_OK == rc )
            {
               type = CSV_TYPE_NULL;
               fieldEnd = tmpFieldEnd ;
               goto done;
            }
         }
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToDate(fieldValue.strVal, autoDateTime, fieldValue.dateVal);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_OID:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToOID(fieldValue.strVal, fieldValue.oidVal);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_REGEX:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToRegex(fieldValue.strVal, fieldValue.regexVal);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_BINARY:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToBinary(fieldValue.strVal, fieldValue.binaryVal);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_AUTO:
         rc = _detectFieldType(data, length, strDel, strDelLen,
                               fieldDel, fieldDelLen, autoAddStrDel,
                               type, fieldValue, valueLength, fieldEnd);
         SDB_ASSERT(CSV_TYPE_AUTO != type,
                    "type must not be CSV_TYPE_AUTO after detecting field type");
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_SKIP:
         // treat SKIP filed as string
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, FALSE, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      default:
         rc = SDB_INVALIDARG;
      }

      if (SDB_OK != rc)
      {
         if (CSV_TYPE_NULL != type && CSV_TYPE_AUTO != type)
         {
            rc = _stringToRawNull(data, length, fieldDel, fieldDelLen,
                                  valueLength, fieldEnd);
            if (SDB_OK == rc)
            {
               type = CSV_TYPE_NULL;
               goto done;
            }
         }
         goto error;
      }

   done:
      return rc;
   error:
      {
         string field(data, length);
         PD_LOG(PDERROR, "Failed to parse field value, type=%s, value=[%s]",
                _CSVTypeToString(type, opt).c_str(), field.c_str());
      }
      goto done;
   }

   static inline INT32 _parseFieldName( const CHAR* data, INT32 length,
                                        const CHAR* fieldDel, INT32 fieldDelLen,
                                        string& fieldName,
                                        INT32& fieldNameLength,
                                        BOOLEAN& fieldEnd )
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;
      CHAR* start = NULL;
      CHAR quotes = 0;
      BOOLEAN hasQuotes = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != fieldDel, "fieldDel can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(fieldDelLen > 0, "fieldDelLen must be greater than 0");

      if ('\'' == *str || '\"' == *str)
      {
         hasQuotes = TRUE;
         quotes = *str;
         str++;
         len--;
      }

      start = str;

      while (len > 0)
      {
         if (hasQuotes)
         {
            if (quotes == *str)
            {
               break;
            }
         }
         else
         {
            if (isspace(*str))
            {
               break;
            }
            else if (_startWith(str, len, fieldDel, fieldDelLen))
            {
               fieldEnd = TRUE;
               break;
            }
         }

         str++;
         len--;
      }

      if (len == length)
      {
         rc = SDB_INVALIDARG;
         PD_LOG( PDERROR, "Invalid field name" ) ;
         goto error;
      }

      fieldNameLength = str - start;
      fieldName = string(start, fieldNameLength);
      if ( !_isValidFieldName( start, fieldNameLength ) )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field name: [%s]", fieldName.c_str() ) ;
         goto error ;
      }

      if (hasQuotes)
      {
         str++;
         len--;
      }
      _skipSpace(&str, len, fieldDel, fieldDelLen);
      fieldNameLength = length - len;
      if (len != 0 && _startWith(str, len, fieldDel, fieldDelLen))
      {
         fieldEnd = TRUE;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _parseFieldTypeString( const CHAR* data,
                                              INT32 length,
                                              const CHAR* fieldDel,
                                              INT32 fieldDelLen,
                                              CSV_TYPE& fieldType,
                                              INT32& fieldTypeLength,
                                              CSVFieldOpt& opt,
                                              BOOLEAN& fieldEnd )
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      BOOLEAN inBracket = FALSE;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != fieldDel, "fieldDel can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(fieldDelLen > 0, "fieldDelLen must be greater than 0");

      while (len > 0)
      {
         if (inBracket)
         {
            if (LEFT_BRACKET == *str)
            {
               rc = SDB_INVALIDARG;
               PD_LOG( PDERROR, "Invalid fields format, "
                                "duplicate left bracket" ) ;
               goto error;
            }

            if (RIGHT_BRACKET == *str)
            {
               inBracket = FALSE;
            }
         }
         else if (LEFT_BRACKET == *str)
         {
            inBracket = TRUE;
         }
         else if (isspace(*str))
         {
            break;
         }
         else if (_startWith(str, len, fieldDel, fieldDelLen))
         {
            fieldEnd = TRUE;
            break;
         }

         str++;
         len--;
      }

      if (len == length)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field type" ) ;
         goto error ;
      }

      if (inBracket)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field type, bracket is not closed" ) ;
         goto error ;
      }

      fieldTypeLength = length - len;

      rc = _convertToCSVType(data, fieldTypeLength, fieldType, opt);
      if (SDB_OK != rc)
      {
         string s( data, fieldTypeLength ) ;

         PD_LOG( PDERROR, "Invalid csv type: %s", s.c_str() ) ;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _parseFieldDefaultValue(const CHAR* data,
                                               INT32 length,
                                               const CHAR* fieldDel,
                                               INT32 fieldDelLen,
                                               const CHAR* strDel,
                                               INT32 strDelLen,
                                               CSVField& field,
                                               INT32& fieldDefaultLength,
                                               BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 valueLen = 0;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;
      CSV_TYPE fieldType ;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(NULL != fieldDel, "fieldDel can't be NULL");
      SDB_ASSERT(NULL != strDel, "strDel can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(fieldDelLen > 0, "fieldDelLen must be greater than 0");
      SDB_ASSERT(strDelLen > 0, "strDelLen must be greater than 0");

      if (!_startWith(str, len, CSV_STR_DEFAULT, CSV_STR_DEFAULT_SIZE))
      {
         fieldDefaultLength = 0;
         if (_startWith(str, len, fieldDel, fieldDelLen))
         {
            fieldEnd = TRUE;
            goto done;
         }
         else
         {
            rc = SDB_INVALIDARG;
            PD_LOG( PDERROR, "Missing \"default\" keyword" ) ;
            goto error;
         }
      }

      if ( CSV_TYPE_NULL == field.type )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field format, null type does not support "
                          "default values" ) ;
         goto error;
      }

      field.hasDefault = TRUE;

      str += CSV_STR_DEFAULT_SIZE;
      len -= CSV_STR_DEFAULT_SIZE;

      if (len == 0)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field format, missing default value" ) ;
         goto error ;
      }

      if (!isspace(*str))
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field format, missing spaces" ) ;
         goto error ;
      }

      _skipSpace(&str, len);
      if (len == 0)
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "Invalid field format, missing default value" ) ;
         goto error ;
      }

      fieldType = field.type ;

      rc = _parseFieldValue(str, len,
                            fieldDel, fieldDelLen,
                            strDel, strDelLen, FALSE,
                            field.type, field.subType,
                            &field, field.defaultValue,
                            valueLen, fieldEnd);
      if (SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Invalid field's default value" ) ;
         goto error ;
      }

      if ( CSV_TYPE_NULL == field.type )
      {
         if( CSV_TYPE_STRING == fieldType &&
             field.defaultValue.strVal.length <
                     field.opt.opt.stringOpt.minLength )
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Invalid string value, "
                             "value length can't less than %d",
            field.opt.opt.stringOpt.minLength ) ;
            goto error ;
         }

         goto done ;
      }

      str += valueLen;
      len -= valueLen;
      fieldDefaultLength = length - len;

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _bsonAppendField(bson& obj,
                                        CSVField& field,
                                        CSVFieldData& data)
   {
      CSV_TYPE type;
      CSVFieldValue* value = NULL;
      INT32 rc = SDB_OK;

      SDB_ASSERT(CSV_TYPE_AUTO != data.type, "data.type can't be CSV_TYPE_AUTO");

      if( CSV_TYPE_NULL == data.type || CSV_TYPE_SKIP == data.type )
      {
         if( CSV_TYPE_AUTO != field.type && CSV_TYPE_SKIP != field.type &&
             field.hasDefault )
         {
            type = (CSV_TYPE_NUMBER == field.type) ? field.subType: field.type;
            value = &(field.defaultValue);
         }
         else
         {
            type = data.type;
         }
      }
      else
      {
         type = (CSV_TYPE_NUMBER == data.type) ? data.subType: data.type;
         value = &(data.value);
      }

      SDB_ASSERT(type > CSV_TYPE_AUTO && type < CSV_TYPE_NUM, "Invalid type");

      switch(type)
      {
      case CSV_TYPE_INT:
         rc = bson_append_int(&obj, field.name.c_str(), value->intVal);
         break;
      case CSV_TYPE_LONG:
         rc = bson_append_long(&obj, field.name.c_str(), value->longVal);
         break;
      case CSV_TYPE_DOUBLE:
         rc = bson_append_double(&obj, field.name.c_str(), value->doubleVal);
         break;
      case CSV_TYPE_DECIMAL:
         rc = bson_append_decimal(&obj, field.name.c_str(), &(value->decimalVal));
         break;
      case CSV_TYPE_BOOL:
         rc = bson_append_bool(&obj, field.name.c_str(), value->boolVal);
         break;
      case CSV_TYPE_STRING:
         if (!_forceNotUTF8)
         {
            rc = bson_append_string_n(&obj, field.name.c_str(),
                                      value->strVal.str, value->strVal.length);
         }
         else
         {
            rc = bson_append_string_not_utf8(&obj, field.name.c_str(),
                                             value->strVal.str, value->strVal.length);
         }
         break;
      case CSV_TYPE_NULL:
         if (!_ignoreNull)
         {
            rc = bson_append_null(&obj, field.name.c_str());
         }
         break;
      case CSV_TYPE_OID:
         {
            bson_oid_t oid;
            bson_oid_from_string(&oid, value->oidVal.str);
            rc = bson_append_oid(&obj, field.name.c_str(), &oid);
         }
         break;
      case CSV_TYPE_TIMESTAMP:
      case CSV_TYPE_AUTO_TIMESTAMP:
         rc = bson_append_timestamp2(&obj, field.name.c_str(),
                                     value->timestampVal.sec,
                                     value->timestampVal.us);
         break;
      case CSV_TYPE_DATE:
      case CSV_TYPE_AUTO_DATE:
         rc = bson_append_date(&obj, field.name.c_str(), value->dateVal);
         break;
      case CSV_TYPE_REGEX:
         {
            CHAR* patTerm = NULL;
            CHAR* optTerm = NULL;
            CHAR patCh = '\0';
            CHAR optCh = '\0';

            // terminate string
            patTerm = value->regexVal.pattern;
            if (*patTerm != '\0')
            {
               patTerm += value->regexVal.patternLen;
               patCh= *patTerm;
               *patTerm = '\0';
            }

            optTerm = value->regexVal.option;
            if (*optTerm != '\0')
            {
               optTerm += value->regexVal.optionLen;
               optCh = *optTerm;
               *optTerm = '\0';
            }

            rc = bson_append_regex(&obj, field.name.c_str(),
                                   value->regexVal.pattern,
                                   value->regexVal.option);

            // recovery string
            if (patCh != '\0')
            {
               *patTerm = patCh;
            }
            if (optCh != '\0')
            {
               *optTerm = optCh;
            }
         }
         break;
      case CSV_TYPE_BINARY:
         rc = bson_append_binary(&obj, field.name.c_str(),
                                 value->binaryVal.type,
                                 value->binaryVal.bin,
                                 value->binaryVal.binLen);
         break;
      case CSV_TYPE_SKIP:
         // no need to append SKIP filed
         rc = SDB_OK;
         break;
      case CSV_TYPE_AUTO:
      default:
         rc = SDB_INVALIDARG;
         SDB_ASSERT(FALSE, "Invalid csv type");
      }

      if (BSON_OK != rc)
      {
         rc = SDB_DRIVER_BSON_ERROR;
      }
      else
      {
         rc = SDB_OK;
      }

      return rc;
   }

   CSVRecordParser::CSVRecordParser(const string& fieldDelimiter,
                                    const string& stringDelimiter,
                                    const string& dateFormat,
                                    const string& timestampFormat,
                                    STR_TRIM_TYPE stringTrimType,
                                    BOOLEAN autoAddField,
                                    BOOLEAN autoAddValue,
                                    BOOLEAN hasHeaderLine,
                                    BOOLEAN cast,
                                    BOOLEAN ignoreNull,
                                    BOOLEAN forceNotUTF8,
                                    BOOLEAN strictFieldNum,
                                    BOOLEAN autoAddStrDel,
                                    BOOLEAN mustHasIDField)
   : RecordParser(fieldDelimiter,
                  stringDelimiter,
                  autoAddField,
                  autoAddValue,
                  autoAddStrDel,
                  mustHasIDField),
     _hasHeaderLine(hasHeaderLine)
   {
      _hasId = FALSE;
      _dateFormat = dateFormat;
      _timestampFormat = timestampFormat;
      _stringTrimType = stringTrimType;
      _cast = cast;
      _ignoreNull = ignoreNull;
      _forceNotUTF8 = forceNotUTF8;
      _strictFieldNum = strictFieldNum;
   }

   CSVRecordParser::~CSVRecordParser()
   {
      vector<CSVField*>::iterator i = _fieldVec.begin() ;

      for( ; i != _fieldVec.end(); ++i )
      {
         CSVField* field = *i ;

         SAFE_OSS_DELETE( field ) ;
      }
   }

   // field_name [field_type] [default <default_value>],
   INT32 CSVRecordParser::parseFields( const CHAR* data, INT32 length,
                                       BOOLEAN isHeaderline )
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;

      const CHAR* fieldDel = NULL;
      INT32 fieldDelLen = 0;
      const CHAR* strDel = NULL;
      INT32 strDelLen = 0;

      CSVField* field = NULL;
      INT32 fieldCount = 0;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(0 == _fieldVec.size(), "fields already parsed");

      if (isHeaderline)
      {
         fieldDel = _fieldDelimiter.c_str();
         fieldDelLen = _fieldDelimiter.length();
         strDel = _stringDelimiter.c_str();
         strDelLen = _stringDelimiter.length();
      }
      else
      {
         fieldDel = ",";
         fieldDelLen = 1;
         strDel = "\"";
         strDelLen = 1;
      }

      _fields = string(data, length);

      while (len > 0)
      {
         _skipSpace(&str, len);
         if (len == 0)
         {
            if (fieldCount > 0)
            {
               goto done;
            }

            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Fields can't be empty" ) ;
            goto error ;
         }

         field = SDB_OSS_NEW CSVField();
         if (NULL == field)
         {
            rc = SDB_OOM ;
            PD_LOG( PDERROR, "Failed to create CSVField, rc=%d", rc ) ;
            goto error ;
         }

         // field name
         {
            INT32 fieldNameLen = 0;
            BOOLEAN fieldEnd = FALSE;
            rc = _parseFieldName(str, len,
                                 fieldDel, fieldDelLen,
                                 field->name, fieldNameLen, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to parse field name, rc=%d", rc);
               goto error;
            }

            fieldCount++;
            field->id = fieldCount;

            str += fieldNameLen;
            len -= fieldNameLen;

            if (fieldEnd)
            {
               rc = _pushField(field);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
                  goto error;
               }
               field = NULL;
               str += fieldDelLen;
               len -= fieldDelLen;
               continue;
            }
         }

         _skipSpace(&str, len, fieldDel, fieldDelLen);
         if (len == 0)
         {
            rc = _pushField(field);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
               goto error;
            }
            field = NULL;
            goto done;
         }

         // type
         {
            INT32 fieldTypeLen = 0;
            BOOLEAN fieldEnd = FALSE;
            CSV_TYPE fieldType = CSV_TYPE_AUTO;
            CSVFieldOpt opt;

            rc = _parseFieldTypeString(str, len, fieldDel, fieldDelLen,
                                       fieldType, fieldTypeLen, opt, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG( PDERROR, "Failed to parse field type, rc=%d", rc ) ;
               goto error;
            }

            field->type = fieldType;

            if (opt.hasOpt)
            {
               field->opt = opt;
            }

            str += fieldTypeLen;
            len -= fieldTypeLen;

            if (fieldEnd)
            {
               rc = _pushField(field);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
                  goto error;
               }
               field = NULL;
               str += fieldDelLen;
               len -= fieldDelLen;
               continue;
            }
         }

         _skipSpace(&str, len, fieldDel, fieldDelLen);
         if (len == 0)
         {
            rc = _pushField(field);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
               goto error;
            }
            field = NULL;
            goto done;
         }

         // default
         {
            INT32 fieldDefaultLen = 0;
            BOOLEAN fieldEnd = FALSE;

            rc = _parseFieldDefaultValue(str, len, fieldDel, fieldDelLen,
                                         strDel, strDelLen,
                                         *field, fieldDefaultLen, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "Failed to parse field default value, rc=%d", rc);
               goto error;
            }

            str += fieldDefaultLen;
            len -= fieldDefaultLen;

            if (fieldEnd)
            {
               rc = _pushField(field);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
                  goto error;
               }
               field = NULL;
               str += fieldDelLen;
               len -= fieldDelLen;
               continue;
            }

            if (len == 0)
            {
               rc = _pushField(field);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to push field, rc=%d", rc);
                  goto error;
               }
               field = NULL;
               goto done;
            }

            if (len > 0)
            {
               rc = SDB_INVALIDARG ;
               PD_LOG( PDERROR, "Invalid field format, field name=%s",
                       field->name.c_str() ) ;
               goto error ;
            }
         }
      }

   done:
      return rc;
   error:
      SAFE_OSS_DELETE(field);
      goto done;
   }

   INT32 CSVRecordParser::_pushField(CSVField* field)
   {
      static string recordIdName = RECORD_ID_NAME;
      INT32 size = _fieldVec.size();
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != field, "field can't be NULL");

      for (INT32 i = 0; i < size; i++)
      {
         if (field->name == _fieldVec[i]->name)
         {
            rc = SDB_INVALIDARG ;
            PD_LOG( PDERROR, "Duplicate field name, name=%s",
                    field->name.c_str() ) ;
            goto error ;
         }
      }

      _fieldVec.push_back(field);

      if (field->name == recordIdName)
      {
         _hasId = TRUE;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 CSVRecordParser::parseRecord(const CHAR* data,
                                      INT32 length,
                                      bson& obj)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;

      const CHAR* fieldDel = NULL;
      INT32 fieldDelLen = 0;
      const CHAR* strDel = NULL;
      INT32 strDelLen = 0;

      INT32 fieldDefNum = _fieldVec.size();
      INT32 fieldCount = 0;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(_mustHasIDField, "can not be false");


      fieldDel = _fieldDelimiter.c_str();
      fieldDelLen = _fieldDelimiter.length();
      strDel = _stringDelimiter.c_str();
      strDelLen = _stringDelimiter.length();

      if (!_hasId && _mustHasIDField)
      {
         bson_oid_t oid;
         bson_oid_gen(&oid);
         rc = bson_append_oid(&obj, RECORD_ID_NAME, &oid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to append record id, rc=%d", rc);
            goto error;
         }
      }

      while (len > 0 && fieldCount < fieldDefNum)
      {
         CSVFieldData fieldData;
         INT32 valueLength = 0;
         BOOLEAN fieldEnd = FALSE;

         CSVField* field = _fieldVec[fieldCount];
         fieldData.type = field->type;
         fieldData.subType = field->subType;

         // in the case of CSV_TYPE_STRING or CSV_TYPE_AUTO, the space may
         // be data, so cannot be skip
         if ((CSV_TYPE_STRING != fieldData.type &&
            CSV_TYPE_AUTO != fieldData.type) || !_autoAddStrDel)
         {
            _skipSpace(&str, len, fieldDel, fieldDelLen, strDel, strDelLen);
            if (len == 0)
            {
                break;
            }
         }

         rc = _parseFieldValue(str, len,
                               fieldDel, fieldDelLen,
                               strDel, strDelLen, _autoAddStrDel,
                               fieldData.type, fieldData.subType,
                               field, fieldData.value,
                               valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to parse field, rc=%d", rc);
            goto error;
         }

         rc = _bsonAppendField(obj, *field, fieldData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to append field, rc=%d", rc);
            goto error;
         }

         str += valueLength;
         len -= valueLength;
         fieldCount++;

         if (len == 0)
         {
            break;
         }

         if (!fieldEnd)
         {
            goto error;
         }

         str += fieldDelLen;
         len -= fieldDelLen;
      }

      if (len == 0 && fieldCount == 0)
      {
         // empty record
         rc = SDB_DMS_EOC;
         goto error;
      }
      else if (len == 0 && fieldCount < fieldDefNum)
      {
         if (_strictFieldNum)
         {
            string r = string(data, length);
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Record field num less than definition, "\
                            "fieldNum=%d, defNum=%d, record=%s",
                            fieldCount, fieldDefNum, r.c_str());
            goto error ;
         }
         
         if (_autoAddValue)
         {
            while (fieldCount < fieldDefNum)
            {
               CSVFieldData fieldData;
               CSVField* field = _fieldVec[fieldCount];
               fieldData.type = CSV_TYPE_NULL;

               rc = _bsonAppendField(obj, *field, fieldData);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to append field, rc=%d", rc);
                  goto error;
               }

               fieldCount++;
            }
         }
      }
      else if (len != 0 && fieldCount == fieldDefNum)
      {
         if (_strictFieldNum)
         {
            string r = string(data, length);
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Parsed %d fields, but record still has fields, "\
                            "record=%s",
                            fieldCount, r.c_str());
            goto error ;
         }

         if (_autoAddField)
         {
            CSVField tmpField;
            INT32 i = 1;

            while (len > 0)
            {
               CSVFieldData fieldData;
               INT32 valueLength = 0;
               BOOLEAN fieldEnd = FALSE;
               // field type is CSV_TYPE_AUTO
               rc = _parseFieldValue(str, len,
                                     fieldDel, fieldDelLen,
                                     strDel, strDelLen, _autoAddStrDel,
                                     fieldData.type, fieldData.subType,
                                     &tmpField, fieldData.value,
                                     valueLength, fieldEnd);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to parse field, rc=%d", rc);
                  goto error;
               }

               {
                  stringstream ss;
                  ss << "field" << (i++);
                  tmpField.name = ss.str();
               }

               rc = _bsonAppendField(obj, tmpField, fieldData);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "Failed to append field, rc=%d", rc);
                  goto error;
               }

               str += valueLength;
               len -= valueLength;
               fieldCount++;

               if (len == 0)
               {
                  break;
               }

               if (!fieldEnd)
               {
                  goto error;
               }

               str += fieldDelLen;
               len -= fieldDelLen;
            }
         }
      }
      else
      {
         SDB_ASSERT(len == 0, "len must be 0");
         SDB_ASSERT(fieldCount == fieldDefNum,
                    "fieldCount must be equals to fieldDefNum");
      }

      if (BSON_OK != bson_finish(&obj))
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Failed to finish bson, rc=%d", rc);
         goto error;
      }

      if (bson_size(&obj) > IMP_MAX_BSON_SIZE)
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "The bson obj is beyond "
                "the max size %d, actual size %d, rc=%d",
                IMP_MAX_BSON_SIZE, bson_size(&obj), rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   void CSVRecordParser::printFieldsDef()
   {
      INT32 size = _fieldVec.size();

      if (0 == size)
      {
         std::cout << "No fields definition!" << std::endl;
         return;
      }

      for (INT32 i = 0; i < size; i++)
      {
         CSVField* field = _fieldVec[i];
         _printField(*field);
      }
   }
}

