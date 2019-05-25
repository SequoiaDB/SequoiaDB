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
                              BOOLEAN autoAddValue)
   : _fieldDelimiter(fieldDelimiter),
     _stringDelimiter(stringDelimiter),
     _autoAddField(autoAddField),
     _autoAddValue(autoAddValue)
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
                                        options.strictFieldNum());
         if (NULL == csvParser)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create CSVRecordParser object, rc=%d",
                   rc);
            goto error;
         }

         if (!options.fields().empty())
         {
            PD_LOG(PDINFO, "fields: %s", options.fields().c_str());
            if (options.verbose())
            {
               std::cout << "fields: " << options.fields()
                         << std::endl;
            }
            const CHAR* str = options.fields().c_str();
            INT32 len = options.fields().length();

            rc = csvParser->parseFields(str, len, FALSE);
            if (SDB_OK != rc)
            {
               std::cout << "failed to parse fields" << std::endl;
               PD_LOG(PDERROR, "failed to parse fields, rc=%d", rc);
               goto error;
            }

            if (options.verbose())
            {
               csvParser->printFieldsDef();
            }
         }

         parser = csvParser;
      }
      else
      {
         SDB_ASSERT(FORMAT_JSON == format, "format must be JSON");
         
         JSONRecordParser* jsonParser =
            SDB_OSS_NEW JSONRecordParser();

         if (NULL == jsonParser)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create JSONRecordParser object, rc=%d",
                   rc);
            goto error;
         }

         rc = jsonParser->init() ;
         if( rc )
         {
            PD_LOG( PDERROR, "failed to call JSONRecordParser init, rc=%d",
                    rc ) ;
            goto error ;
         }

         parser = jsonParser;
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

   JSONRecordParser::JSONRecordParser()
   : RecordParser("", "", FALSE, FALSE)
   {
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
      BOOLEAN result = TRUE;

      SDB_ASSERT(NULL != data, "data can't be NULL");
      SDB_ASSERT(length > 0, "length must be greater than 0");

      bson_init(&obj);

      result = json2bson( data, _pMachine, CJSON_RIGOROUS_PARSE, FALSE, &obj ) ;
      if (!result)
      {
         rc = SDB_INVALIDARG;
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
}
