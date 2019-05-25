/*******************************************************************************

   Copyright (C) 2011-2015 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

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
#include "pd.hpp"
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

   #define CSV_INT_MAX  (2147483647)
   #define CSV_INT_MIN  (-2147483648)
   #define CSV_LONG_MAX OSS_SINT64_MAX
   #define CSV_LONG_MIN OSS_SINT64_MIN

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

   static inline BOOLEAN _isValidFieldName(CHAR* field, INT32 length)
   {
      SDB_ASSERT(NULL != field, "field can't be NULL");

      if (length <= 0)
      {
         return FALSE;
      }

      if ('$' == field[0])
      {
         return FALSE;
      }

      while (length > 0)
      {
         UINT8 ch = *field;
         if (ch <=127 && (!isprint(ch) || '.' == ch))
         {
            return FALSE;
         }

         field++;
         length--;
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

      if (LEFT_BRACKET != *str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!isdigit(*str))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _str2i(str, len, precision, numLen);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "invalid decimal precision");
         goto error;
      }

      if (precision < 1 || precision > DECIMAL_MAX_PRECISION)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid decimal precision");
         goto error;
      }

      str += numLen;
      len -= numLen;

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (COMMA != *str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      numLen = 0;
      rc = _str2i(str, len, scale, numLen);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "invalid decimal scale");
         goto error;
      }

      if (scale < 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid decimal scale");
         goto error;
      }

      str += numLen;
      len -= numLen;

      _skipSpace(&str, len);

      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (RIGHT_BRACKET != *str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      str++;
      len--;

      _skipSpace(&str, len);

      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (scale > precision)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "decimal scale can't be greater than precision");
         goto error;
      }

      opt.opt.decimalOpt.precision = precision;
      opt.opt.decimalOpt.scale = scale;
      opt.hasOpt = TRUE;

   done:
      return rc;
   error:
      goto done;
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

      if (length < CSV_STR_TYPE_MIN_SIZE)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid csv type");
         goto error;
      }

      type = CSV_TYPE_AUTO;

      switch(str[0])
      {
      case 'a':
      case 'A':
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
            rc = _parseDecimalPrecision(data + CSV_STR_DECIMAL_SIZE,
                                        length - CSV_STR_DECIMAL_SIZE,
                                        opt);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "invalid decimal type");
               goto error;
            }
            type = CSV_TYPE_DECIMAL;
         }
         break;
      case 'i':
      case 'I':
         if (CSV_STR_TYPE_EQ(CSV_STR_INT, str, length) ||
             CSV_STR_TYPE_EQ(CSV_STR_INTEGER, str, length))
         {
            type = CSV_TYPE_INT;
         }
         break;
      case 'l':
      case 'L':
         if (CSV_STR_TYPE_EQ(CSV_STR_LONG, str, length))
         {
            type = CSV_TYPE_LONG;
         }
         break;
      case 'n':
      case 'N':
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
         if (CSV_STR_TYPE_EQ(CSV_STR_OID, str, length))
         {
            type = CSV_TYPE_OID;
         }
         break;
      case 'r':
      case 'R':
         if (CSV_STR_TYPE_EQ(CSV_STR_REGEX, str, length))
         {
            type = CSV_TYPE_REGEX;
         }
         break;
      case 's':
      case 'S':
         if (CSV_STR_TYPE_EQ(CSV_STR_STRING, str, length))
         {
            type = CSV_TYPE_STRING;
         }
         else if (CSV_STR_TYPE_EQ(CSV_STR_SKIP, str, length))
         {
            type = CSV_TYPE_SKIP;
         }
         break;
      case 't':
      case 'T':
         if (CSV_STR_TYPE_EQ(CSV_STR_TIMESTAMP, str, length))
         {
            type = CSV_TYPE_TIMESTAMP;
         }
         break;
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (CSV_TYPE_AUTO == type)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid csv type");
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }


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

   static inline INT32 _stringToRawNum(const CHAR* data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength, BOOLEAN allowDot)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      UINT64 intNum;
      FLOAT64 floatNum;
      BOOLEAN neg = FALSE;
      UINT64 quo; // quoteint
      INT64 rem; // remainder
      CHAR* start;

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

      quo = neg ? ((UINT64)CSV_LONG_MAX + 1) : CSV_LONG_MAX;
      rem = quo % 10;
      quo /= 10;
      intNum = 0;
      floatNum = 0;

      if (allowDot && '.' == *str)
      {
         type = CSV_TYPE_DOUBLE;
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
         rc = SDB_INVALIDARG;
         goto error;
      }

   finish:
      if (CSV_TYPE_LONG == type)
      {
         value.longVal = neg ? (INT64)(-intNum) : (INT64)intNum;
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

      rc = decimal_from_str( start, &value);
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

   static inline INT32 _stringToRawNumber(const CHAR* data, INT32 length,
                                          CSV_TYPE& type, CSVFieldValue& value,
                                          INT32& valueLength)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      FLOAT64 fract;
      FLOAT64 exponent;
      INT32 intLen;
      INT32 fractLen;
      INT32 expLen;
      FLOAT64 num;
      BOOLEAN neg = FALSE;
      CSV_TYPE tmpType = CSV_TYPE_AUTO;
      CSVFieldValue tmpValue;
      CHAR* start;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      rc = _stringToInfinity( str, len, tmpType, tmpValue, intLen );
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (CSV_TYPE_DOUBLE == tmpType && 0 != intLen)
      {
         str += intLen ;
         len -= intLen ;
         type = CSV_TYPE_DOUBLE ;
         value.doubleVal = tmpValue.doubleVal ;
         valueLength = length - len ;
         goto done ;
      }

      rc = _stringToNan( str, len, tmpType, tmpValue, intLen ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }

      if (CSV_TYPE_DOUBLE == tmpType && 0 != intLen)
      {
         str += intLen ;
         len -= intLen ;
         type = CSV_TYPE_DOUBLE ;
         value.doubleVal = tmpValue.doubleVal ;
         valueLength = length - len ;
         goto done ;
      }

      if ('#' == *str)
      {
         str++;
         len--;
      }

      start = str;

      rc = _stringToRawNum(str, len, tmpType, tmpValue, intLen, TRUE);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (CSV_TYPE_DOUBLE == tmpType && 0 != intLen)
      {
         goto decimal ;
      }

      if ('-' == *str)
      {
         neg = TRUE;
      }

      str += intLen;
      len -= intLen;

      if ('E' == *str || 'e' == *str)
      {
         str++;
         len--;
         type = CSV_TYPE_DOUBLE;
         if (CSV_TYPE_LONG == tmpType)
         {
            num = (FLOAT64)tmpValue.longVal;
         }
         else
         {
            num = tmpValue.doubleVal;
         }
         goto exp;
      }

      if ('.' == *str)
      {
         str++;
         len--;

         if (!isdigit(*str) && !isdigit(*(str-2)))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }

      if (!isdigit(*str))
      {
         if (CSV_TYPE_LONG == tmpType)
         {
            if (tmpValue.longVal >= CSV_INT_MIN && tmpValue.longVal <= CSV_INT_MAX)
            {
               type = CSV_TYPE_INT;
               value.intVal = (INT32)tmpValue.longVal;
            }
            else
            {
               type = CSV_TYPE_LONG;
               value.longVal = tmpValue.longVal;
            }
         }
         else
         {
            type = CSV_TYPE_DOUBLE;
            value.doubleVal = tmpValue.doubleVal;
         }
         valueLength = length - len;
         goto done;
      }

      type = CSV_TYPE_DOUBLE;
      if (CSV_TYPE_LONG == tmpType)
      {
         num = (FLOAT64)tmpValue.longVal;
      }
      else
      {
         num = tmpValue.doubleVal;
      }

      rc = _stringToRawNum(str, len, tmpType, tmpValue, fractLen);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if ('-' == *start || '+' == *start)
      {
         intLen--;
      }

      {
         INT32 fLen = fractLen ;

         while ('0' == str[fLen - 1])
         {
            fLen--;
         }

         if (fLen > DOUBLE_FRACT_PRECISION || fLen + intLen > DOUBLE_PRECISION)
         {
            goto decimal;
         }
      }

      if (CSV_TYPE_LONG == tmpType)
      {
         fract = (FLOAT64)tmpValue.longVal;
      }
      else
      {
         fract = tmpValue.doubleVal;
      }

      str += fractLen;
      len -= fractLen;
      valueLength += fractLen;
      if (!neg)
      {
         num = num + fract / pow(10.0, fractLen);
      }
      else
      {
         num = num - fract / pow(10.0, fractLen);
      }

      if ('E' != *str && 'e' != *str)
      {
         value.doubleVal = num;
         valueLength = length - len;
         goto done;
      }

      str++;
      len--;

   exp:
      if (!isdigit(*str) && '+' != *str && '-' != *str)
      {
         value.doubleVal = num;
         valueLength = length - len;
         goto done;
      }

      rc = _stringToRawNum(str, len, tmpType, tmpValue, expLen);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (CSV_TYPE_LONG == tmpType)
      {
         exponent = (FLOAT64)tmpValue.longVal;
      }
      else
      {
         exponent = tmpValue.doubleVal;
      }

      if (exponent > DOUBLE_MAX_EXP || exponent < DOUBLE_MIN_EXP)
      {
         goto decimal;
      }

      if (DOUBLE_MAX_EXP == exponent && ( num > DOUBLE_BOUND || num < -DOUBLE_BOUND ) )
      {
         goto decimal;
      }

      if (DOUBLE_MIN_EXP == exponent && ( num > -DOUBLE_BOUND || num < DOUBLE_BOUND ) )
      {
         goto decimal;
      }

      str += expLen;
      len -= expLen;
      valueLength += expLen;
      num *= pow(10.0, exponent);

      value.doubleVal = num;
      valueLength = length - len;
      goto done;

   decimal:
      type = CSV_TYPE_DECIMAL;
      decimal_init(&(value.decimalVal));
      rc = _stringToRawDecimal( data, length, value.decimalVal, valueLength);
      if (SDB_OK != rc)
      {
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

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
      case CSV_TYPE_DOUBLE:
         if (_cast)
         {
            value = (INT32)tmpField.value.doubleVal;
            break;
         }
      case CSV_TYPE_DECIMAL:
         if (_cast)
         {
            value = (INT32)decimal_to_int(&(tmpField.value.decimalVal));
            break;
         }
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

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
      case CSV_TYPE_DECIMAL:
         if (_cast)
         {
            value = (INT64)decimal_to_long(&(tmpField.value.decimalVal));
            break;
         }
      default:
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

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
         value = (FLOAT64)decimal_to_double(&(subField.value.decimalVal));
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
                                       CSVString& value, INT32& valueLength,
                                       BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      BOOLEAN inString = FALSE;
      fieldEnd = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");
      SDB_ASSERT(!isspace(*data), "data can't begin with space");

      if (_startWith(str, len, strDel, strDelLen))
      {
         str += strDelLen;
         len -= strDelLen;
         inString = TRUE;
      }

      value.str = str;

      while (len > 0)
      {
         if (_startWith(str, len, strDel, strDelLen))
         {
            /*if ('\\' == *(str - 1))
            {
               len -= strDelLen;
               str += strDelLen;
               continue;
            }*/

            len -= strDelLen;
            str += strDelLen;

            if (len == 0)
            {
               valueLength = length;
               value.length = str - strDelLen - value.str;
               goto done;
            }

            if (_startWith(str, len, strDel, strDelLen))
            {
               value.hasEscape = TRUE;
               len -= strDelLen;
               str += strDelLen;
               continue;
            }

            inString = FALSE;
            value.length = str - strDelLen - value.str;
            break;
         }

         if (!inString)
         {
            if (_startWith(str, len, fieldDel, fieldDelLen))
            {
               fieldEnd = TRUE;
               valueLength = length - len;
               value.length = str - value.str;
               goto done;
            }

            /*if (isspace(*str))
            {*/
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

      if (inString || len == 0)
      {
         SDB_ASSERT(len == 0, "len must be equal 0");
         valueLength = length;
         value.length = str - value.str;
         goto done;
      }
      else
      {
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

         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      if (SDB_OK == rc && valueLength > CSV_MAX_STRING_SIZE)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "the string is out of length [0-%d]: %d",
                CSV_MAX_STRING_SIZE, valueLength);
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

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _stringToRawInt(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         rc = _stringToRawBool(str, len, (BOOLEAN&)value, tmpLen);
      }

      if (SDB_OK != rc)
      {
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         goto error;
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
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _stringToRawLong(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         BOOLEAN boolValue = FALSE;
         rc = _stringToRawBool(str, len, boolValue, tmpLen);
         value = boolValue;
      }

      if (SDB_OK != rc)
      {
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         goto error;
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
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

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
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
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
            rc = SDB_INVALIDARG;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

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
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         goto error;
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
            rc = SDB_INVALIDARG;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _stringToRawDouble(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   static INT32 _stringToDecimal(const CHAR* data, INT32 length,
                                 const CHAR* strDel, INT32 strDelLen,
                                 const CHAR* fieldDel, INT32 fieldDelLen,
                                 const CSVFieldOpt& opt,
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
         rc = decimal_init1(&value, opt.opt.decimalOpt.precision, opt.opt.decimalOpt.scale);
         if (0 != rc)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
      else
      {
         decimal_init(&value);
      }
      rc = _stringToRawDecimal(data, length, value, valueLength);
      if (SDB_OK == rc)
      {
         str += valueLength;
         len -= valueLength;

         if (!_isValidFieldEnd(str, len, fieldDel, fieldDelLen, tmpLen, fieldEnd))
         {
            rc = SDB_INVALIDARG;
            goto error;
         }

         valueLength += tmpLen;
         goto done;
      }

      rc = _stringToString(data, length,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           csvStr, valueLength, fieldEnd);
      if (SDB_OK != rc || data == csvStr.str)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      tmpLen = 0;
      str = csvStr.str;
      len = csvStr.length;
      _skipSpace(&str, len);
      if (0 == len)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (opt.hasOpt)
      {
         rc = decimal_init1(&value, opt.opt.decimalOpt.precision, opt.opt.decimalOpt.scale);
         if (0 != rc)
         {
            rc = SDB_INVALIDARG;
            goto error;
         }
      }
      else
      {
         decimal_init(&value);
      }
      rc = _stringToRawDecimal(str, len, value, tmpLen);
      if (SDB_OK != rc)
      {
         goto error;
      }

      str += tmpLen;
      len -= tmpLen;

      _skipSpace(&str, len);
      if (0 != len)
      {
         rc = SDB_INVALIDARG;
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
         goto error;
      }

      while (len > 0)
      {
         if (_startWith(str, len, strDel, strDelLen))
         {
            len -= strDelLen;
            str += strDelLen;

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

   static const CHAR CSV_UTF8_FULL_WIDTH_SPACE[] = {0xE3, 0x80, 0x80, 0x00};
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
                             CSV_UTF8_FULL_WIDTH_SPACE,
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
            CHAR* trimedStr = (CHAR*)SDB_OSS_MALLOC(len + 1);
            if (NULL == trimedStr)
            {
               rc = SDB_OOM;
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
                           CSV_UTF8_FULL_WIDTH_SPACE,
                           CSV_UTF8_FULL_WIDTH_SPACE_LEN))
         {            tail = tail - CSV_UTF8_FULL_WIDTH_SPACE_LEN;
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
         SDB_ASSERT(FALSE, "invalid trim type");
      }

   done:
      return rc;
   error:
      goto done;
   }

   static inline INT32 _detectFieldType(const CHAR* data, INT32 length,
                                        const CHAR* strDel, INT32 strDelLen,
                                        const CHAR* fieldDel, INT32 fieldDelLen,
                                        CSV_TYPE& fieldType,
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
      SDB_ASSERT(!isspace(*data), "data can't begin with space");

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

         fieldValue.reset( fieldType ) ;
         str = (CHAR*)data;
         len = length;
      }

      rc = _stringToRawNull(str, len, fieldDel, fieldDelLen, fieldLength, fieldEnd);
      if (SDB_OK == rc)
      {
         fieldType = CSV_TYPE_NULL;
         goto done;
      }

      fieldType = CSV_TYPE_STRING;
      rc = _stringToString(str, len,
                           strDel, strDelLen,
                           fieldDel, fieldDelLen,
                           fieldValue.strVal, fieldLength,
                           fieldEnd);
      if (SDB_OK != rc)
      {
         goto error;
      }

      if (fieldValue.strVal.hasEscape)
      {
         rc = _escapedString(fieldValue.strVal, strDel, strDelLen);
         if (SDB_OK != rc)
         {
            goto error;
         }
      }

      if (STR_TRIM_NO != _stringTrimType)
      {
         rc = _trimString(fieldValue.strVal, _stringTrimType);
         if (SDB_OK != rc)
         {
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

      if (tmpValue.longVal < CSV_INT_MIN || tmpValue.longVal > CSV_INT_MAX)
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
                                         struct tm* time, INT32& microsec)
   {
      INT32 year = 0;
      INT32 month = 0;
      INT32 day = 0;
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
         case 'Y':
            SDB_ASSERT('Y' == fmt[0] &&
                       'Y' == fmt[1] &&
                       'Y' == fmt[2] &&
                       'Y' == fmt[3], "invalid format of year");
            rc = _str2i(str, 4, year, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 4;
            fmtLen -= 4;
            break;
         case 'M':
            SDB_ASSERT('M' == fmt[0] &&
                       'M' == fmt[1], "invalid format of month");
            rc = _str2i(str, 2, month, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         case 'D':
            SDB_ASSERT('D' == fmt[0] &&
                       'D' == fmt[1], "invalid format of day");
            rc = _str2i(str, 2, day, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         case 'H':
            SDB_ASSERT('H' == fmt[0] &&
                       'H' == fmt[1], "invalid format of hour");
            rc = _str2i(str, 2, hour, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         case 'm':
            SDB_ASSERT('m' == fmt[0] &&
                       'm' == fmt[1], "invalid format of minute");
            rc = _str2i(str, 2, minute, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         case 's':
            SDB_ASSERT('s' == fmt[0] &&
                       's' == fmt[1], "invalid format of second");
            rc = _str2i(str, 2, second, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 2;
            fmtLen -= 2;
            break;
         case 'S':
            {
               INT32 ms;
               SDB_ASSERT('S' == fmt[0] &&
                          'S' == fmt[1] &&
                          'S' == fmt[2], "invalid format of millisecond");
               if (mxs)
               {
                  rc = SDB_INVALIDARG;
                  goto error;
               }
               rc = _str2i(str, 3, ms, valueLength);
               if (SDB_OK != rc)
               {
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
         case 'f':
            SDB_ASSERT('f' == fmt[0] &&
                       'f' == fmt[1] &&
                       'f' == fmt[2] &&
                       'f' == fmt[3] &&
                       'f' == fmt[4] &&
                       'f' == fmt[5], "invalid format of microsecond");
            if (mxs)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            rc = _str2i(str, 6, microsec, valueLength);
            if (SDB_OK != rc)
            {
               goto error;
            }
            str += valueLength;
            strLen -= valueLength;
            fmt += 6;
            fmtLen -= 6;
            mxs = TRUE;
            break;
         case '*':
            str++;
            strLen--;
            fmt++;
            fmtLen--;
            break;
         default:
            if (*str != *fmt)
            {
               rc = SDB_INVALIDARG;
               goto error;
            }
            str++;
            strLen--;
            fmt++;
            fmtLen--;
            break;
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
                                          CSVTimestamp& value)
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
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid timestamp length");
         goto error;
      }

      term = str + data.length;
      tmpch = *term;
      *term = '\0';

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
         INT32 microsec = 0;
         time_t timep;

         ossMemset(&t, 0, sizeof(t));
         rc = _stringToDateTime(data.str, data.length,
                                TIME_FORMAT, TIME_FORMAT_LEN,
                                &t, microsec);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan timepstamp, rc=%d", rc);
            goto error;
         }

         /* sanity check */
         if (t.tm_year > TIME_LAST_YEAR || t.tm_year < TIME_START_YEAR ||
             t.tm_mon >= RELATIVE_MOD || t.tm_mon < 0 ||
             t.tm_mday > RELATIVE_DAY || t.tm_mday <= 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid date of timestamp");
            goto error;
         }

         if (t.tm_hour >= RELATIVE_HOUR || t.tm_hour < 0 ||
             t.tm_min >= RELATIVE_MIN_SEC || t.tm_min < 0 ||
             t.tm_sec >= RELATIVE_MIN_SEC || t.tm_sec < 0 ||
             microsec >= RELATIVE_MICRO_SEC || microsec < 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid time of timestamp");
            goto error;
         }

         t.tm_year -= RELATIVE_YEAR;

         /* create integer time representation */
         timep = mktime(&t);
         if( !ossIsTimestampValid( timep ) )
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid time of timestamp");
            goto error;
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
         if (SDB_OK != rc || data.length != valueLength)
         {
            PD_LOG(PDERROR, "failed to get the number of timestamp, rc=%d", rc);
            goto error;
         }

         sec = varLong / 1000;
         us = (varLong % 1000) * 1000; // microseconds
         if (us < 0)
         {
            sec--;
            us += 1000000;
         }

         if (varLong < TIME_MIN_NUM * 1000 )
         {
            PD_LOG(PDERROR, "The timestamp %lld is less than %lld000",
                   varLong, TIME_MIN_NUM);
            rc = SDB_INVALIDARG;
            goto error;
         }

         if ((sec > TIME_MAX_NUM) || ((sec == TIME_MAX_NUM) && us > 0))
         {
            PD_LOG(PDERROR, "The timestamp %lld is greater than %lld000",
                   varLong, TIME_MAX_NUM);
            rc = SDB_INVALIDARG;
            goto error;
         }

         value.sec = (INT32)sec;
         value.us = (INT32)us;
      }

   done:
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
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid date length");
         goto error;
      }

      term = str + data.length;
      tmpch = *term;
      *term = '\0';

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
         INT32 microsec = 0;
         time_t timep;

         ossMemset(&t, 0, sizeof(t));
         rc = _stringToDateTime(data.str, data.length,
                                DATE_FORMAT, DATE_FORMAT_LEN,
                                &t, microsec);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to scan date");
            goto error;
         }

         /* sanity check */
         if (t.tm_year > DATE_LAST_YEAR || t.tm_year < DATE_START_YEAR ||
             t.tm_mon >= RELATIVE_MOD || t.tm_mon < 0 ||
             t.tm_mday > RELATIVE_DAY || t.tm_mday <= 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid date");
            goto error;
         }

         t.tm_year -= RELATIVE_YEAR;

         /* create integer time representation */
         timep = mktime(&t);
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
         PD_LOG(PDERROR, "invalid oid length");
      }
      else
      {
         value.str = data.str;
         value.length = data.length;
      }

      return rc;
   }

   static inline INT32 _stringToRegex(CSVString& data, CSVRegex& value)
   {
      CHAR* str = data.str;
      INT32 len = data.length;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      if (len == 0)
      {
         value.pattern = CSV_STR_EMPTY;
         value.patternLen = 0;
         value.option = CSV_STR_EMPTY;
         value.optionLen = 0;
         goto done;
      }

      if (len < 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid regex length");
         goto error;
      }

      if (CSV_STR_BACKSLASH != *str)
      {
         value.pattern = str;
         value.patternLen = len;
         value.option = CSV_STR_EMPTY;
         goto done;
      }

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
         PD_LOG(PDERROR, "invalid regex format");
         goto error;
      }

      value.patternLen = str - value.pattern;

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
            PD_LOG(PDERROR, "invalid regex format");
            goto error;
         }
      }

   done:
      return rc ;
   error:
      goto done ;
   }

   static inline INT32 _stringToBinary(CSVString& data, CSVBinary& value)
   {
      CHAR* str = data.str;
      INT32 len = data.length;
      INT32 rc = SDB_OK;
      INT32 base64Len = 0;

      SDB_ASSERT(NULL != data.str, "data.str can't be NULL");

      CHAR* term = str + data.length;
      CHAR tmpch = *term;
      *term = '\0';

      value.bin = NULL;

      if (len <= 0)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid binary length");
         goto error;
      }

      if (CSV_STR_LEFTBRACKET == *str)
      {
         INT32 type = 0;
         INT32 typeLength = 0;

         str++;
         len--;

         _skipSpace(&str, len);
         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid binary format");
            goto error;
         }

         rc = _stringToRawInt(str, len, type, typeLength);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "invalid binary type");
            goto error;
         }

         if (type < 0 || type > 255)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "binary type is out of range[0~255], type:%d",
                   type);
            goto error;
         }

         value.type = type;

         str += typeLength;
         len -= typeLength;

         _skipSpace(&str, len);
         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid binary format");
            goto error;
         }

         if (CSV_STR_RIGHTBRACKET != *str)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid binary format");
            goto error;
         }

         str++;
         len--;

         if (len == 0)
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "invalid binary format");
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
         PD_LOG(PDERROR, "invalid binary base64 size");
         goto error;
      }

      if (base64Len > 0)
      {
         value.bin = (CHAR*)SDB_OSS_MALLOC(base64Len);
         if (NULL == value.bin)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to malloc");
            goto error;
         }
         ossMemset(value.bin, 0, base64Len);
         if (0 > base64Decode(str, value.bin, base64Len))
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "failed to decode binary");
            goto error;
         }

         value.binLen = base64Len - 1;
      }
      else
      {
         value.bin = "";
         value.binLen = 0;
      }

   done:
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

               decimal_to_str_get_len(&(field.defaultValue.decimalVal), &len);

               buf = (CHAR*)SDB_OSS_MALLOC(len);
               if (NULL == buf)
               {
                  break;
               }
               ossMemset(buf, 0, len);

               decimal_to_str(&(field.defaultValue.decimalVal), buf, len);
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
                                        CSV_TYPE& type, CSV_TYPE& subType,
                                        CSVFieldOpt& opt, CSVFieldValue& fieldValue,
                                        INT32& valueLength, BOOLEAN& fieldEnd)
   {
      CHAR* str = (CHAR*)data;
      INT32 len = length;
      INT32 rc = SDB_OK;
      fieldEnd = FALSE;
      BOOLEAN autoDateTime = FALSE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      if (_startWith(str, len, fieldDel, fieldDelLen))
      {
         if (CSV_TYPE_SKIP != type)
         {
            type = CSV_TYPE_NULL;
         }
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
                               fieldDel, fieldDelLen,
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
            goto error;
         }
         goto done;
      case CSV_TYPE_STRING:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         if (fieldValue.strVal.hasEscape)
         {
            rc = _escapedString(fieldValue.strVal, strDel, strDelLen);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         if (STR_TRIM_NO != _stringTrimType)
         {
            rc = _trimString(fieldValue.strVal, _stringTrimType);
            if (SDB_OK != rc)
            {
               goto error;
            }
         }
         goto done;
      case CSV_TYPE_AUTO_TIMESTAMP:
         autoDateTime = TRUE;
      case CSV_TYPE_TIMESTAMP:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, fieldValue.strVal,
                              valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            goto error;
         }
         rc = _stringToTimestamp(fieldValue.strVal, autoDateTime, fieldValue.timestampVal);
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_AUTO_DATE:
         autoDateTime = TRUE;
      case CSV_TYPE_DATE:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, fieldValue.strVal,
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
                              fieldDelLen, fieldValue.strVal,
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
                              fieldDelLen, fieldValue.strVal,
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
                              fieldDelLen, fieldValue.strVal,
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
                               fieldDel, fieldDelLen,
                               type, fieldValue, valueLength, fieldEnd);
         SDB_ASSERT(CSV_TYPE_AUTO != type,
                    "type must not be CSV_TYPE_AUTO after detecting field type");
         if (SDB_OK != rc)
         {
            goto error;
         }
         goto done;
      case CSV_TYPE_SKIP:
         rc = _stringToString(data, length, strDel, strDelLen, fieldDel,
                              fieldDelLen, fieldValue.strVal,
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
         PD_LOG(PDERROR, "failed to parse field value, type=%s, value=[%s]",
                _CSVTypeToString(type, opt).c_str(), field.c_str());
      }
      goto done;
   }

   static inline INT32 _parseFieldName(const CHAR* data, INT32 length,
                                       const CHAR* fieldDel, INT32 fieldDelLen,
                                       string& fieldName, INT32& fieldNameLength,
                                       BOOLEAN& fieldEnd)
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
         goto error;
      }

      fieldNameLength = str - start;
      fieldName = string(start, fieldNameLength);
      if (!_isValidFieldName(start, fieldNameLength))
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid field name: [%s]", fieldName.c_str());
         goto error;
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

   static inline INT32 _parseFieldTypeString(const CHAR* data,
                                             INT32 length,
                                             const CHAR* fieldDel,
                                             INT32 fieldDelLen,
                                             CSV_TYPE& fieldType,
                                             INT32& fieldTypeLength,
                                             CSVFieldOpt& opt,
                                             BOOLEAN& fieldEnd)
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
               PD_LOG(PDERROR, "duplicate left bracket");
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
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "invalid field type");
         goto error;
      }

      if (inBracket)
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "bracket is not closed");
         goto error;
      }

      fieldTypeLength = length - len;

      rc = _convertToCSVType(data, fieldTypeLength, fieldType, opt);
      if (SDB_OK != rc)
      {
         string s(data, fieldTypeLength);
         PD_LOG(PDERROR, "invalid csv type: %s", s.c_str());
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
            PD_LOG(PDERROR, "missed \"default\" keyword");
            goto error;
         }
      }

      field.hasDefault = TRUE;

      str += CSV_STR_DEFAULT_SIZE;
      len -= CSV_STR_DEFAULT_SIZE;

      if (len == 0)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      if (!isspace(*str))
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      _skipSpace(&str, len);
      if (len == 0)
      {
         rc = SDB_INVALIDARG;
         goto error;
      }

      rc = _parseFieldValue(str, len,
                            fieldDel, fieldDelLen,
                            strDel, strDelLen,
                            field.type, field.subType,
                            field.opt, field.defaultValue,
                            valueLen, fieldEnd);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "invalid field value");
         goto error;
      }

      if (CSV_TYPE_NULL == field.type)
      {
         goto error;
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

      if (CSV_TYPE_NULL == data.type)
      {
         if (CSV_TYPE_AUTO != field.type && field.hasDefault)
         {
            type = (CSV_TYPE_NUMBER == field.type) ? field.subType: field.type;
            value = &(field.defaultValue);
         }
         else
         {
            type = CSV_TYPE_NULL;
         }
      }
      else
      {
         type = (CSV_TYPE_NUMBER == data.type) ? data.subType: data.type;
         value = &(data.value);
      }

      SDB_ASSERT(type > CSV_TYPE_AUTO && type < CSV_TYPE_NUM, "invalid type");

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
         rc = SDB_OK;
         break;
      case CSV_TYPE_AUTO:
      default:
         rc = SDB_INVALIDARG;
         SDB_ASSERT(FALSE, "invalid csv type");
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
                                    BOOLEAN strictFieldNum)
   : RecordParser(fieldDelimiter,
                  stringDelimiter,
                  autoAddField,
                  autoAddValue),
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
   }

   INT32 CSVRecordParser::parseFields(const CHAR* data, INT32 length, BOOLEAN isHeaderline)
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

            rc = SDB_INVALIDARG;
            goto error;
         }

         field = SDB_OSS_NEW CSVField();
         if (NULL == field)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create CSVField, rc=%d", rc);
            goto error;
         }

         {
            INT32 fieldNameLen = 0;
            BOOLEAN fieldEnd = FALSE;
            rc = _parseFieldName(str, len,
                                 fieldDel, fieldDelLen,
                                 field->name, fieldNameLen, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to parse field name, rc=%d", rc);
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
                  PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
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
               PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
               goto error;
            }
            field = NULL;
            goto done;
         }

         {
            INT32 fieldTypeLen = 0;
            BOOLEAN fieldEnd = FALSE;
            CSV_TYPE fieldType = CSV_TYPE_AUTO;
            CSVFieldOpt opt;

            rc = _parseFieldTypeString(str, len, fieldDel, fieldDelLen,
                                       fieldType, fieldTypeLen, opt, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to parse field type, rc=%d", rc);
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
                  PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
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
               PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
               goto error;
            }
            field = NULL;
            goto done;
         }

         {
            INT32 fieldDefaultLen = 0;
            BOOLEAN fieldEnd = FALSE;

            rc = _parseFieldDefaultValue(str, len, fieldDel, fieldDelLen,
                                         strDel, strDelLen,
                                         *field, fieldDefaultLen, fieldEnd);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to parse field default value, rc=%d", rc);
               goto error;
            }

            str += fieldDefaultLen;
            len -= fieldDefaultLen;

            if (fieldEnd)
            {
               rc = _pushField(field);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
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
                  PD_LOG(PDERROR, "failed to push field, rc=%d", rc);
                  goto error;
               }
               field = NULL;
               goto done;
            }

            if (len > 0)
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "invalid field");
               goto error;
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
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "duplicate field name: %s", field->name.c_str());
            goto error;
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

      fieldDel = _fieldDelimiter.c_str();
      fieldDelLen = _fieldDelimiter.length();
      strDel = _stringDelimiter.c_str();
      strDelLen = _stringDelimiter.length();

      bson_init(&obj);

      if (!_hasId)
      {
         bson_oid_t oid;
         bson_oid_gen(&oid);
         rc = bson_append_oid(&obj, RECORD_ID_NAME, &oid);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append record id, rc=%d", rc);
            goto error;
         }
      }

      while (len > 0 && fieldCount < fieldDefNum)
      {
         CSVFieldData fieldData;
         INT32 valueLength = 0;
         BOOLEAN fieldEnd = FALSE;

         _skipSpace(&str, len, fieldDel, fieldDelLen);
         if (len == 0)
         {
            break;
         }

         CSVField* field = _fieldVec[fieldCount];
         fieldData.type = field->type;
         fieldData.subType = field->subType;

         rc = _parseFieldValue(str, len,
                               fieldDel, fieldDelLen,
                               strDel, strDelLen,
                               fieldData.type, fieldData.subType,
                               field->opt, fieldData.value,
                               valueLength, fieldEnd);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to parse field, rc=%d", rc);
            goto error;
         }

         rc = _bsonAppendField(obj, *field, fieldData);
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to append field, rc=%d", rc);
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
         rc = SDB_DMS_EOC;
         goto error;
      }
      else if (len == 0 && fieldCount < fieldDefNum)
      {
         if (_strictFieldNum)
         {
            string r = string(data, length);
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "record field num less than definition, "\
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
                  PD_LOG(PDERROR, "failed to append field, rc=%d", rc);
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
            PD_LOG(PDERROR, "parsed %d fields, but record still has fields, "\
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

               _skipSpace(&str, len, fieldDel, fieldDelLen);
               if (len == 0)
               {
                  break;
               }

               rc = _parseFieldValue(str, len,
                                     fieldDel, fieldDelLen,
                                     strDel, strDelLen,
                                     fieldData.type, fieldData.subType,
                                     tmpField.opt, fieldData.value,
                                     valueLength, fieldEnd);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to parse field, rc=%d", rc);
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
                  PD_LOG(PDERROR, "failed to append field, rc=%d", rc);
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
         PD_LOG(PDERROR, "failed to finish bson, rc=%d", rc);
         goto error;
      }

      if (bson_size(&obj) > IMP_MAX_BSON_SIZE)
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "the bson obj is beyond "
                "the max size %d, actual size %d, rc=%d",
                IMP_MAX_BSON_SIZE, bson_size(&obj), rc);
         goto error;
      }

   done:
      return rc;
   error:
      bson_destroy(&obj);
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

