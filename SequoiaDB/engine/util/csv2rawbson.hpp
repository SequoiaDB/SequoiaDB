/*******************************************************************************

   Copyright (C) 2012-2014 SequoiaDB Ltd.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

   Source File Name = csv2rawbson.hpp

   Descriptive Name = CSV To Raw BSON

   When/how to use: this program may be used on binary and text-formatted
   versions of UTIL component. This file contains declare of json2rawbson. Note
   this function should NEVER be directly called other than fromjson.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          05/04/2014  JWH Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef UTIL_CSV_2_BSON_HPP__
#define UTIL_CSV_2_BSON_HPP__

#include "core.hpp"
#include "oss.hpp"
#include <vector>

/* csv type */
#define CSV_STR_INT        "int"
#define CSV_STR_INTEGER    "integer"
#define CSV_STR_LONG       "long"
#define CSV_STR_BOOL       "bool"
#define CSV_STR_BOOLEAN    "boolean"
#define CSV_STR_DOUBLE     "double"
#define CSV_STR_STRING     "string"
#define CSV_STR_TIMESTAMP  "timestamp"
#define CSV_STR_DATE       "date"
#define CSV_STR_NULL       "null"
#define CSV_STR_OID        "oid"
#define CSV_STR_REGEX      "regex"
#define CSV_STR_BINARY     "binary"
#define CSV_STR_NUMBER     "number"

/* type value */
#define CSV_STR_TRUE       "true"
#define CSV_STR_FALSE      "false"

/* key word */
#define CSV_STR_DEFAULT    "default"
#define CSV_STR_FIELD      "field"

/* string size */
#define CSV_STR_INT_SIZE         ( sizeof( CSV_STR_INT ) - 1 )
#define CSV_STR_INTEGER_SIZE     ( sizeof( CSV_STR_INTEGER ) - 1 )
#define CSV_STR_LONG_SIZE        ( sizeof( CSV_STR_LONG ) - 1 )
#define CSV_STR_BOOL_SIZE        ( sizeof( CSV_STR_BOOL ) - 1 )
#define CSV_STR_BOOLEAN_SIZE     ( sizeof( CSV_STR_BOOLEAN ) - 1 )
#define CSV_STR_DOUBLE_SIZE      ( sizeof( CSV_STR_DOUBLE ) - 1 )
#define CSV_STR_STRING_SIZE      ( sizeof( CSV_STR_STRING ) - 1 )
#define CSV_STR_TIMESTAMP_SIZE   ( sizeof( CSV_STR_TIMESTAMP ) - 1 )
#define CSV_STR_DATE_SIZE        ( sizeof( CSV_STR_DATE ) - 1 )
#define CSV_STR_NULL_SIZE        ( sizeof( CSV_STR_NULL ) - 1 )
#define CSV_STR_OID_SIZE         ( sizeof( CSV_STR_OID ) - 1 )
#define CSV_STR_REGEX_SIZE       ( sizeof( CSV_STR_REGEX ) - 1 )
#define CSV_STR_BINARY_SIZE      ( sizeof( CSV_STR_BINARY ) - 1 )
#define CSV_STR_NUMBER_SIZE      ( sizeof( CSV_STR_NUMBER ) - 1 )

#define CSV_STR_TRUE_SIZE        ( sizeof( CSV_STR_TRUE ) - 1 )
#define CSV_STR_FALSE_SIZE       ( sizeof( CSV_STR_FALSE ) - 1 )

#define CSV_STR_DEFAULT_SIZE     ( sizeof( CSV_STR_DEFAULT ) - 1 )
#define CSV_STR_FIELD_SIZE       ( sizeof( CSV_STR_FIELD ) - 1 )

#define CSV_STR_FIELD_MAX_SIZE 1024

enum CSV_TYPE
{
   CSV_TYPE_INT = 0,
   CSV_TYPE_LONG,
   CSV_TYPE_BOOL,
   CSV_TYPE_DOUBLE,
   CSV_TYPE_STRING,
   CSV_TYPE_TIMESTAMP,
   CSV_TYPE_DATE,
   CSV_TYPE_NULL,
   CSV_TYPE_OID,
   CSV_TYPE_REGEX,
   CSV_TYPE_BINARY,
   CSV_TYPE_NUMBER,
   CSV_TYPE_AUTO
} ;

class csvParser : public SDBObject
{
private:
   struct _csvTimestamp : public SDBObject
   {
      INT32 i ; /* increment */
      INT32 t ; /* time in seconds */
   } ;
   struct _csvRegex : public SDBObject
   {
      CHAR *pPattern ;
      CHAR *pOptions ;
   } ;
   struct _csvBinary : public SDBObject
   {
      INT32 strSize ;
      BOOLEAN isOwnmem ;
      CHAR type ;
      CHAR *pStr ;      
   } ;
   struct _fieldData : public SDBObject
   {
      CSV_TYPE type ;
      CSV_TYPE subType ;
      INT32 stringSize ;
      INT32 varInt ;
      BOOLEAN varBool ;
      BOOLEAN hasDefVal ;
      INT64 varLong ;
      FLOAT64 varDouble ;
      CHAR *pVarString ;
      CHAR *pField ;
      _csvTimestamp varTimestamp ;
      _csvRegex varRegex ;
      _csvBinary varBinary ;
      _fieldData() : type(CSV_TYPE_INT),
                     subType(CSV_TYPE_INT),
                     stringSize(0),
                     varInt(0),
                     varBool(FALSE),
                     hasDefVal(FALSE),
                     varLong(0),
                     varDouble(0),
                     pVarString(NULL),
                     pField(NULL)
                     
      {
         varTimestamp.i = 0 ;
         varTimestamp.t = 0 ;
         varRegex.pPattern = NULL ;
         varRegex.pOptions = NULL ;
         varBinary.strSize = 0 ;
         varBinary.isOwnmem = FALSE ;
         varBinary.type = 0 ;
         varBinary.pStr = NULL ;
      }
   } ;
   struct _valueData : public SDBObject
   {
      CSV_TYPE type ;
      INT32 stringSize ;
      INT32 varInt ;
      BOOLEAN varBool ;
      INT64 varLong ;
      FLOAT64 varDouble ;
      CHAR *pVarString ;
      _csvTimestamp varTimestamp ;
      _csvRegex varRegex ;
      _csvBinary varBinary ;
      _valueData() : type(CSV_TYPE_INT),
                     stringSize(0),
                     varInt(0),
                     varBool(FALSE),
                     varLong(0),
                     varDouble(0),
                     pVarString(NULL)
      {
         varTimestamp.i = 0 ;
         varTimestamp.t = 0 ;
         varRegex.pPattern = NULL ;
         varRegex.pOptions = NULL ;
         varBinary.strSize = 0 ;
         varBinary.isOwnmem = FALSE ;
         varBinary.type = 0 ;
         varBinary.pStr = NULL ;
      }
   } ;
private:
   BOOLEAN _addField ;
   BOOLEAN _completion ;
   BOOLEAN _isHeaderline ;
   CHAR    _delChar ;
   CHAR    _delField ;
   CHAR    _delRecord ;
   CHAR   *_pCsvHeader ;
   std::vector<_fieldData *> _vField ;
private:
   CHAR *_trimLeft ( CHAR *pCursor, INT32 &size ) ;
   CHAR *_trimRight ( CHAR *pCursor, INT32 &size ) ;
   CHAR *_trim ( CHAR *pCursor, INT32 &size ) ;
   INT32 _parseNumber( CHAR *pBuffer, INT32 size,
                       CSV_TYPE &csvType,
                       INT32 *pVarInt = NULL,
                       INT64 *pVarLong = NULL,
                       FLOAT64 *pVarDouble = NULL ) ;
   INT32 _string2int( INT32 &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2long( INT64 &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2bool( BOOLEAN &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2double( FLOAT64 &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2timestamp( _csvTimestamp &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2timestamp2( _csvTimestamp &value, CHAR *pBuffer, INT32 size );
   INT32 _string2date( INT64 &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2date2( INT64 &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2regex( _csvRegex &value, CHAR *pBuffer, INT32 size ) ;
   INT32 _string2binary( _csvBinary &value, CHAR *pBuffer, INT32 size ) ;
   CHAR *_findSpace( CHAR *pBuffer, INT32 &size ) ;
   CHAR *_skipSpace( CHAR *pBuffer, INT32 &size ) ;
   INT32 _headerEscape( CHAR *pBuffer, INT32 size,
                        CHAR **ppOutBuf, INT32 &newSize ) ;
   INT32 _valueEscape( CHAR *pBuffer, INT32 size,
                       CHAR **ppOutBuf, INT32 &newSize ) ;
private:
   INT32 _parseValue( _valueData &valueData,
                      _fieldData &fieldData,
                      CHAR *pBuffer, INT32 size ) ;
   INT32 _parseValue( _valueData &valueData, CHAR *pBuffer, INT32 size ) ;
   INT32 _parseField( _fieldData &fieldData, CHAR *pBuffer, INT32 size ) ;
   INT32 _appendBsonNull( void *bsonObj, const CHAR *pKey ) ;
   INT32 _appendBson( void *bsonObj, _fieldData *pFieldData ) ;
   INT32 _appendBson( void *bsonObj, const CHAR *pKey,
                      _valueData *pValueData ) ;
public:
   csvParser() ;
   ~csvParser() ;
   INT32 init( BOOLEAN autoAddField,
               BOOLEAN autoCompletion,
               BOOLEAN isHeaderline,
               CHAR delChar,
               CHAR delField,
               CHAR delRecord ) ;
   INT32 parseHeader( CHAR *pHeader, INT32 size ) ;
   INT32 csv2bson( CHAR *pBuffer, INT32 size, CHAR **ppRawbson ) ;
   INT32 csv2bson( CHAR *pBuffer, INT32 size, void *pbson ) ;
} ;

#endif