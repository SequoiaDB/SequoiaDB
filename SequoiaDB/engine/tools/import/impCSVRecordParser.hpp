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

   Source File Name = impCSVRecordParser.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_CSV_RECORD_PARSER_HPP_
#define IMP_CSV_RECORD_PARSER_HPP_

#include "impRecordParser.hpp"
#include "ossUtil.h"
#include <vector>

namespace import
{

   #define CSV_INVALID_FIELD_ID (-1)

   enum CSV_TYPE
   {
      CSV_TYPE_AUTO = 0,
      CSV_TYPE_INT,
      CSV_TYPE_LONG,
      CSV_TYPE_NUMBER,
      CSV_TYPE_DOUBLE,
      CSV_TYPE_DECIMAL,
      CSV_TYPE_BOOL,
      CSV_TYPE_STRING,
      CSV_TYPE_TIMESTAMP,
      CSV_TYPE_AUTO_TIMESTAMP,
      CSV_TYPE_DATE,
      CSV_TYPE_AUTO_DATE,
      CSV_TYPE_NULL,
      CSV_TYPE_OID,
      CSV_TYPE_REGEX,
      CSV_TYPE_BINARY,
      CSV_TYPE_SKIP,
      CSV_TYPE_NUM
   };

   struct CSVString
   {
      CHAR*    str;
      INT32    length;
      BOOLEAN  hasEscape;
      BOOLEAN  escaped;
   };

   struct CSVTimestamp
   {
      INT32 sec; // seconds
      INT32 us;  // microseconds
   };

   struct CSVRegex
   {
      CHAR* pattern;
      CHAR* option;
      INT32 patternLen;
      INT32 optionLen;
   };

   struct CSVBinary
   {
      CHAR* str;
      CHAR* bin;
      INT32 binLen;
      INT32 type;
   };

   union CSVFieldValue
   {
      INT32          intVal;
      INT64          longVal;
      FLOAT64        doubleVal;
      bson_decimal   decimalVal;
      BOOLEAN        boolVal;
      CSVString      strVal;
      CSVTimestamp   timestampVal;
      INT64          dateVal;
      CSVString      oidVal;
      CSVRegex       regexVal;
      CSVBinary      binaryVal;

      CSVFieldValue()
      {
         ossMemset(this, 0, sizeof(CSVFieldValue));
      }

      void reset( CSV_TYPE type, CSV_TYPE subType = CSV_TYPE_AUTO )
      {
         if (CSV_TYPE_BINARY == type)
         {
            SAFE_OSS_FREE(binaryVal.bin);
            binaryVal.binLen = 0;
         }
         else if (CSV_TYPE_STRING == type)
         {
            if (strVal.escaped)
            {
               SAFE_OSS_FREE(strVal.str);
            }
         }
         else if (CSV_TYPE_DECIMAL == type ||
                  (CSV_TYPE_NUMBER == type && CSV_TYPE_DECIMAL == subType))
         {
            sdb_decimal_free(&decimalVal);
         }
         ossMemset(this, 0, sizeof(CSVFieldValue));
      }
   };

   struct CSVStringOpt
   {
      INT32 minLength ;
      INT32 maxLength ;
      BOOLEAN strict ;
   } ;

   struct CSVDecimalOpt
   {
      INT32 precision;
      INT32 scale;
   };

   #define CSV_TIMESTAMP_FMT_MAX_LEN 63
   struct CSVTimestampOpt
   {
      INT32 fmtLength ;
      CHAR format[CSV_TIMESTAMP_FMT_MAX_LEN + 1] ;
   } ;

   struct CSVFieldOpt
   {
      BOOLEAN hasOpt;
      union {
         CSVStringOpt stringOpt ;
         CSVDecimalOpt decimalOpt;
         CSVTimestampOpt timestampOpt ;
      } opt;

      CSVFieldOpt()
      {
         ossMemset(&opt, 0, sizeof(opt));
         hasOpt = FALSE;
      }
   };

   struct CSVField: public SDBObject
   {
      INT32          id;
      CSV_TYPE       type;
      CSV_TYPE       subType;
      string         name;
      CSVFieldOpt    opt;
      BOOLEAN        hasDefault;
      CSVFieldValue  defaultValue;

      CSVField()
      {
         id = CSV_INVALID_FIELD_ID;
         type = CSV_TYPE_AUTO;
         subType = CSV_TYPE_AUTO;
         name = "";
         hasDefault = FALSE;
      }

      ~CSVField()
      {
         if (hasDefault)
         {
            defaultValue.reset( type, subType ) ;
         }
      }
   };

   struct CSVFieldData: public SDBObject
   {
      CSV_TYPE       type;
      CSV_TYPE       subType;
      CSVFieldValue  value;

      CSVFieldData()
      {
         type = CSV_TYPE_AUTO;
         subType = CSV_TYPE_AUTO;
      }

      void reset()
      {
         if (CSV_TYPE_BINARY == type)
         {
            SAFE_OSS_FREE(value.binaryVal.bin);
            value.binaryVal.binLen = 0;
         }
         else if (CSV_TYPE_STRING == type)
         {
            if (value.strVal.escaped)
            {
               SAFE_OSS_FREE(value.strVal.str);
            }

            value.strVal.hasEscape = FALSE;
            value.strVal.escaped = FALSE;
            value.strVal.length = 0;
         }
         else if (CSV_TYPE_DECIMAL == type ||
                  (CSV_TYPE_NUMBER == type && CSV_TYPE_DECIMAL == subType))
         {
            sdb_decimal_free(&(value.decimalVal));
         }

         type = CSV_TYPE_AUTO;
      }

      ~CSVFieldData()
      {
         reset();
      }
   };

   class CSVRecordParser: public RecordParser
   {
   public:
      CSVRecordParser(const string& fieldDelimiter,
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
                      BOOLEAN mustHasIDField);
      ~CSVRecordParser();
      INT32 parseRecord(const CHAR* data, INT32 length, bson& obj);
      INT32 parseFields( const CHAR* data, INT32 length, BOOLEAN isHeaderline );
      void  printFieldsDef();

      void  reset()
      {
         vector<CSVField*>::iterator i = _fieldVec.begin() ;

         for( ; i != _fieldVec.end(); ++i )
         {
            CSVField* field = *i ;

            SAFE_OSS_DELETE( field ) ;
         }

         _fieldVec.clear() ;
         _fields.clear() ;
         _hasId = FALSE ;
      }

   private:
      INT32 _pushField(CSVField* field);

   private:
      vector<CSVField*> _fieldVec;
      string            _fields;
      BOOLEAN           _hasHeaderLine;
      BOOLEAN           _hasId;
      BOOLEAN           _strictFieldNum;
   };
}

#endif /* IMP_CSV_RECORD_PARSER_HPP_ */
