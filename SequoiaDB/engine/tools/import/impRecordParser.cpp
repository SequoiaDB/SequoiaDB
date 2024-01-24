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

   Source File Name = impRecordParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRecordParser.hpp"
#include "impCSVRecordParser.hpp"
#include "pd.hpp"
#include <iostream>

void _impRecordParseLog( const CHAR *pFunc,
                         const CHAR *pFile,
                         UINT32 line,
                         const CHAR *pFmt,
                         ... )
{
   va_list ap ;
   CHAR buffer[ PD_LOG_STRINGMAX + 1 ] = { 0 } ;
   va_start( ap, pFmt ) ;
   vsnprintf( buffer, PD_LOG_STRINGMAX, pFmt, ap ) ;
   va_end( ap ) ;
   pdLog( PDERROR, pFunc, pFile, line, buffer ) ;
}

namespace import
{
   RecordParser::RecordParser(const string& fieldDelimiter,
                              const string& stringDelimiter,
                              BOOLEAN autoAddField,
                              BOOLEAN autoAddValue,
                              BOOLEAN autoAddStrDel,
                              BOOLEAN mustHasIDField)
   : _fieldDelimiter(fieldDelimiter),
     _stringDelimiter(stringDelimiter),
     _autoAddField(autoAddField),
     _autoAddValue(autoAddValue),
     _autoAddStrDel(autoAddStrDel),
     _mustHasIDField(mustHasIDField)
   {
   }

   INT32 RecordParser::createInstance(INPUT_FORMAT format,
                                      const Options& options,
                                      RecordParser*& parser)
   {
      INT32 rc = SDB_OK;

      if (FORMAT_CSV == format)
      {
         CSVRecordParser* csvParser =
            SDB_OSS_NEW CSVRecordParser(options.fieldDelimiter(),
                                        options.stringDelimiter(),
                                        options.dateFormat(),
                                        options.timestampFormat(),
                                        options.trimString(),
                                        options.autoAddField(),
                                        options.autoCompletion(),
                                        options.hasHeaderLine(),
                                        options.cast(),
                                        options.ignoreNull(),
                                        options.force(),
                                        options.strictFieldNum(),
                                        options.autoAddStrDel(),
                                        options.mustHasIDField());
         if (NULL == csvParser)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "Failed to create CSVRecordParser object, rc=%d",
                   rc);
            goto error;
         }

         parser = csvParser;
      }
      else
      {
         SDB_ASSERT(FORMAT_JSON == format, "format must be JSON");
         
         JSONRecordParser* jsonParser =
            SDB_OSS_NEW JSONRecordParser( options.isUnicode(),
                                          options.decimalto(),
                                          options.mustHasIDField() );

         if (NULL == jsonParser)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "Failed to create JSONRecordParser object, rc=%d",
                   rc);
            goto error;
         }

         parser = jsonParser;

         rc = jsonParser->init() ;
         if( rc )
         {
            PD_LOG( PDERROR, "Failed to call JSONRecordParser init, rc=%d",
                    rc ) ;
            goto error ;
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   void RecordParser::releaseInstance(RecordParser* parser)
   {
      SDB_ASSERT(NULL != parser, "parser can't be NULL");

      SDB_OSS_DEL(parser);
   }

   JSONRecordParser::JSONRecordParser( BOOLEAN isUnicode,
                                       DECIMAL_TO_TYPE decimalto,
                                       BOOLEAN mustHasIDField )
         : RecordParser("", "", FALSE, FALSE, mustHasIDField)
   {
      _isUnicode = isUnicode ;
      _decimalto = (INT32)decimalto ;
      _pMachine = NULL ;
      JsonSetPrintfLog( _impRecordParseLog ) ;
   }

   JSONRecordParser::~JSONRecordParser()
   {
      cJsonRelease( _pMachine ) ;
   }

   INT32 JSONRecordParser::init()
   {
      INT32 rc = SDB_OK ;
      _pMachine = cJsonCreate() ;
      if( _pMachine == NULL )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Failed to call cJsonCreate" ) ;
         goto error ;
      }
   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 JSONRecordParser::parseRecord(const CHAR* data, INT32 length, bson& obj)
   {
      INT32 rc = SDB_OK;
      INT32 flags = JSON_FLAG_RIGOROUS_MODE|JSON_FLAG_NOT_INIT_BSON ;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");



      if ( _isUnicode )
      {
         flags |= JSON_FLAG_ESCAPE_UNICODE ;
      }

      if ( JSON_DECIMAL_TO_DOUBLE == _decimalto )
      {
         flags |= JSON_FLAG_DECIMAL_TO_DOUBLE ;
      }
      else if ( JSON_DECIMAL_TO_STRING == _decimalto )
      {
         flags |= JSON_FLAG_DECIMAL_TO_STRING ;
      }

      SDB_ASSERT( _mustHasIDField, "can not be false" ) ;
      if ( _mustHasIDField )
      {
         flags |= JSON_FLAG_APPEND_OID ;
      }

      if ( !json2bson3( data, _pMachine, flags, &obj ) )
      {
         rc = SDB_INVALIDARG;
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
}
