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

   Source File Name = impParser.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impParser.hpp"
#include "impInputStream.hpp"
#include "impRecordScanner.hpp"
#include "impRecordReader.hpp"
#include "impRecordParser.hpp"
#include "impCSVRecordParser.hpp"
#include "impMonitor.hpp"
#include "../util/text.h"
#include "pd.hpp"
#include <sstream>
#include <iostream>

namespace import
{

   void _parserRoutine(WorkerArgs* args)
   {
      Parser* self = (Parser*)args;

      CHAR* buffer = NULL;
      InputStream* input = NULL;
      RecordParser* parser = NULL;
      RecordArray* recordArray = NULL;

      INT32 countInBatch = 0;
      BOOLEAN isFirst = TRUE;
      INT32 rc = SDB_OK;

      string inputString;
      INT32 fileId = 0;
      INT64 parsedNum = 0;
      INT64 failedNum = 0;

      SDB_ASSERT(NULL != args, "arg can't be NULL");

      const Options* options = self->_options;
      LogFile* logFile = &(self->_logFile);
      RecordQueue* workQueue = self->_workQueue;
      Monitor* monitor = impGetMonitor();

      SDB_ASSERT(NULL != options, "options can't be NULL");
      SDB_ASSERT(NULL != logFile, "logFile can't be NULL");
      SDB_ASSERT(NULL != workQueue, "workQueue can't be NULL");
      SDB_ASSERT(NULL != monitor, "monitor can't be NULL");

      {
         CHAR* str = "parser started...\n";

         PD_LOG(PDEVENT, "%s", str);
         if (options->verbose())
         {
            std::cout << str;
         }
      }

      RecordScanner scanner(options->recordDelimiter(),
                            options->stringDelimiter(),
                            options->inputFormat(),
                            options->linePriority());

      RecordReader recordReader;

      INT32 bufferSize = options->bufferSize() * 1024 * 1024;
      // 1 byte to ensure it's safe to terminate string
      buffer = (CHAR*)SDB_OSS_MALLOC(bufferSize + 1);
      if (NULL == buffer)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to malloc buffer, size=%d", bufferSize + 1);
         goto error;
      }
      buffer[bufferSize] = '\0'; 

      rc = RecordParser::createInstance(options->inputFormat(),
                                        *options, parser);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create RecordParser object,"
                "rc=%d, INPUT_FORMAT=%d", rc, options->inputFormat());
         goto error;
      }

      rc = getRecordArray(options->batchSize(), &recordArray);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to get free RecordArray");
         goto error;
      }

   begin:
      if (INPUT_FILE == options->inputType())
      {
         if (NULL != input)
         {
            InputStream::releaseInstance(input);
            input = NULL;
         }
         // maybe multiple files
         if (fileId < (INT32)options->files().size())
         {
            inputString = options->files()[fileId];
            fileId++;
            parsedNum = 0;
            failedNum = 0;
            isFirst = TRUE;
            PD_LOG(PDINFO, "read data from file [%s]", inputString.c_str());
            if (options->verbose())
            {
               std::cout << "read data from file [" 
                         << inputString << "]"
                         << std::endl;
            }
         }
         else
         {
            goto done;
         }
      }
      rc = InputStream::createInstance(options->inputType(),
                                       inputString, *options, input);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to create InputStream object,"
                "rc=%d, INPUT_TYPE=%d", rc, options->inputType());
         goto error;
      }

      recordReader.reset(buffer, bufferSize,
                         input, &scanner,
                         options->recordDelimiter().length());

      if (FORMAT_CSV == options->inputFormat() &&
          options->hasHeaderLine() &&
          options->fields().empty())
      {
         parser->reset() ;
      }

      while(!self->_stopped)
      {
         CHAR* record = NULL;
         INT32 recordLength = 0;
         bson* obj = NULL;

         rc = recordReader.read(record, recordLength);
         if (SDB_OK != rc)
         {
            if (SDB_EOF == rc)
            {
               rc = SDB_OK;
               if (INPUT_FILE == options->inputType())
               {
                  std::stringstream ss;

                  ss << "parsed records of file ["
                     << inputString << "]: "
                     << parsedNum
                     << std::endl;

                  ss << "parse failure of file ["
                     << inputString << "]: "
                     << failedNum
                     << std::endl;

                  PD_LOG(PDINFO, "%s", ss.str().c_str());
                  // maybe multiple files
                  goto begin;
               }
               break;
            }

            printf("read record error!\n");
            PD_LOG(PDERROR, "failed to read record");
            goto error;
         }

         if (0 == recordLength)
         {
            if (isFirst &&
                0 == recordLength &&
                FORMAT_CSV == options->inputFormat() &&
                options->hasHeaderLine())
            {
               rc = SDB_INVALIDARG;
               PD_LOG(PDERROR, "the headerline is empty");
               printf("ERROR: the headerline is empty\n");
               goto error;
            }

            continue;
         }

         if (!options->force() && !isValidUTF8WSize(record, recordLength))
         {
            rc = SDB_MIG_DATA_NON_UTF;
            PD_LOG(PDERROR, "It is not utf-8 file, rc=%d", rc);
            if (options->errorStop())
            {
               goto error;
            }
         }

         if (isFirst)
         {
            isFirst = FALSE;
            if (FORMAT_CSV == options->inputFormat() &&
                options->hasHeaderLine())
            {
               if (!options->fields().empty())
               {
                  // fields is defined, so ignore the headerline
                  continue;
               }

               string fields = string(record, recordLength);

               PD_LOG(PDINFO, "fields: %s", fields.c_str());
               if (options->verbose())
               {
                  std::cout << "fields: " << fields
                            << std::endl;
               }

               CSVRecordParser* csvParser = (CSVRecordParser*)parser;

               rc = csvParser->parseFields(record, recordLength, TRUE);
               if (SDB_OK != rc)
               {
                  std::cout << "failed to parse fields" << std::endl;
                  PD_LOG(PDERROR, "failed to parse fields, rc = %d",
                         rc);
                  goto error;
               }

               if (options->verbose())
               {
                  csvParser->printFieldsDef();
               }

               // headerline can't be parsed as record
               continue;
            }
         }

         SDB_ASSERT(countInBatch < recordArray->capacity(),
                    "countInBatch must be less than batchSize");

         obj = (bson*)SDB_OSS_MALLOC(sizeof(bson));
         if (NULL == obj)
         {
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to malloc bson");
            goto error;
         }
         bson_init(obj);

         rc = parser->parseRecord(record, recordLength, *obj);
         if (SDB_OK != rc)
         {
            bson_destroy(obj);
            SAFE_OSS_FREE(obj);
            if (SDB_DMS_EOC != rc)
            {
               self->_failedNum++;
               failedNum++;
               INT32 ret;
               if (SDB_OK != (ret = logFile->write(record, recordLength)))
               {
                  PD_LOG(PDERROR, "failed to log write record, rc=%d", ret);
               }

               PD_LOG(PDERROR, "failed to parse record, rc=%d", rc);

               if (options->errorStop())
               {
                  goto error;
               }
            }

            continue;
         }

         SDB_ASSERT(!recordArray->full(), "can't be full");
         recordArray->push(obj);
         countInBatch++;
         self->_parsedNum++;
         parsedNum++;

         if (recordArray->full())
         {
            monitor->recordsMemInc(recordArray->bsonSize());
            monitor->recordsNumInc(recordArray->size());
            recordArray->finish();
            workQueue->push(recordArray);
            countInBatch = 0;
            recordArray = NULL;

            if (monitor->recordsMem() > options->recordsMem())
            {
               // records' memory is beyond the threshold,
               // so wait a moment
               INT64 recordsMem = monitor->recordsMem();
               INT64 recordsNum = monitor->recordsNum();
               PD_LOG(PDEVENT, "records memory is beyond the threshold,\n"
                      "records memory: %lld MB, thredshold: %lld MB,\n"
                      "records num: %lld, average record size %lld",
                      recordsMem / (1024 * 1024),
                      options->recordsMem() / (1024 * 1024),
                      recordsNum, recordsMem / recordsNum);

               // push an empty array to queue as a signal,
               // to tell sharding to empty it's sharding groups,
               // so that sharding can avoid deadly waiting for group FULL 
               RecordArray* array = NULL;
               rc = getRecordArray(0, &array);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get free RecordArray");
                  goto error;
               }
               workQueue->push(array);
               array = NULL;

               for(;;)
               {
                  ossSleep(100);
                  if (monitor->recordsMem() <= (options->recordsMem() / 2))
                  {
                     break;
                  }
               }
               PD_LOG(PDEVENT, "records memory usage down, go on");
            }

            rc = getRecordArray(options->batchSize(), &recordArray);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get free RecordArray");
               goto error;
            }
         }
      }

   done:
      if (NULL != recordArray)
      {
         if (!recordArray->empty())
         {
            workQueue->push(recordArray);
            recordArray = NULL;
         }
         else
         {
            freeRecordArray(&recordArray);
         }
      }
      self->_stopped = TRUE;
      if (NULL != input)
      {
         InputStream::releaseInstance(input);
         input = NULL;
      }
      if (NULL != parser)
      {
         RecordParser::releaseInstance(parser);
         parser = NULL;
      }
      SAFE_OSS_FREE(buffer);

      {
         CHAR* str= "parser stopped";

         PD_LOG(PDEVENT, "%s, rc=%d", str, rc);
         if (options->verbose())
         {
            std::cout << str << std::endl;
         }
      }
      return;
   error:
      goto done;
   }

   Parser::Parser()
   {
      _options = NULL;
      _workQueue = NULL;
      _inited = FALSE;
      _worker = NULL;
      _stopped = TRUE;
      _parsedNum = 0;
      _failedNum = 0;
   }

   Parser::~Parser()
   {
      SAFE_OSS_DELETE(_worker);
   }

   INT32 Parser::init(Options* options,
                      RecordQueue* workQueue)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != options, "options can't be NULL");
      SDB_ASSERT(NULL != workQueue, "workQueue can't be NULL");

      _options = options;
      _workQueue = workQueue;

      string parserLogFile = makeRecordLogFileName(_options->csname(),
                                                   _options->clname(),
                                                   string("parse"));

      rc = _logFile.init(parserLogFile, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init parser log file");
         goto error;
      }

      _worker = SDB_OSS_NEW Worker(_parserRoutine, this);
      if (NULL == _worker)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to create parser Worker object");
         goto error;
      }

      _inited = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Parser::start()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_inited, "must be inited");

      _stopped = FALSE;

      rc = _worker->start();
      if (SDB_OK != rc)
      {
         _stopped = TRUE;
         PD_LOG(PDERROR, "failed to start parser");
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Parser::stop()
   {
      INT32 rc = SDB_OK;

      if(_inited)
      {
         SDB_ASSERT(NULL != _worker, "_worker can't be NULL");

         _stopped = TRUE;

         rc = _worker->waitStop();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to wait the parser stop");
         }

         SAFE_OSS_DELETE(_worker);
      }
      return rc;
   }
}
