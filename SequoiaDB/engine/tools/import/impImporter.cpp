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

   Source File Name = impImporter.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impImporter.hpp"
#include "impRecordImporter.hpp"
#include "impMonitor.hpp"
#include "pd.hpp"
#include <sstream>
#include <iostream>

namespace import
{
   struct ImporterArgs: public WorkerArgs
   {
      INT32          id;
      string         hostname;
      string         svcname;
      Importer*      self;

      ImporterArgs()
      {
         id = -1;
         self = NULL;
      }
   };

   void _importerRoutine(WorkerArgs* args)
   {
      ImporterArgs* impArgs = (ImporterArgs*)args;
      RecordArray* records = NULL;
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != args, "arg can't be NULL");

      Importer* self = impArgs->self;
      Options* options = self->_options;
      Monitor* monitor = impGetMonitor();

      BOOLEAN dryRun = options->dryRun();
      LogFile* logFile = &(self->_logFile);
      RecordQueue* workQueue = self->_workQueue;
      RecordImporter importer(impArgs->hostname,
                              impArgs->svcname,
                              options->user(),
                              options->password(),
                              options->csname(),
                              options->clname(),
                              options->useSSL(),
                              options->enableTransaction(),
                              options->allowKeyDuplication());

      SDB_ASSERT(NULL != workQueue, "workQueue can't be NULL");
      SDB_ASSERT(NULL != logFile, "logFile can't be NULL");
      SDB_ASSERT(NULL != monitor, "monitor can't be NULL");

      {
         stringstream ss;
         ss << "importer [" << impArgs->id << "] with "
            << impArgs->hostname << ":" << impArgs->svcname
            << " started..." << std::endl;

         PD_LOG(PDEVENT, "%s", ss.str().c_str());
         if (options->verbose())
         {
            std::cout << ss.str();
         }
      }

      rc = importer.connect();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to connect, rc=%d", rc);
         goto error;
      }

      for(;;)
      {
         workQueue->wait_and_pop(records);
         if (NULL == records)
         {
            break;
         }

         if (records->empty())
         {
            freeRecordArray(&records);
            continue;
         }

         if (!dryRun)
         {
            rc = importer.import(records);
            if (SDB_OK != rc)
            {
               self->_failedNum.add(records->size());
               for (INT32 i = 0; i < records->size(); i++)
               {
                  INT32 ret;
                  bson* obj = records->get(i);
                  if (SDB_OK != (ret = logFile->write(obj)))
                  {
                     PD_LOG(PDERROR, "failed to log write records, rc=%d", ret);
                     break;
                  }
               }
               monitor->recordsMemDec(records->bsonSize());
               monitor->recordsNumDec(records->size());
               freeRecordArray(&records);
               PD_LOG(PDERROR, "failed to import records, rc=%d", rc);
               continue;
            }
            self->_importedNum.add(records->size());
            monitor->recordsMemDec(records->bsonSize());
            monitor->recordsNumDec(records->size());
         }

         freeRecordArray(&records);
      }

   done:
      {
         stringstream ss;
         ss << "importer [" << impArgs->id << "] stop" << std::endl;

         PD_LOG(PDEVENT, "%s", ss.str().c_str());
         if (options->verbose())
         {
            std::cout << ss.str();
         }
      }
      self->_livingNum.dec();
      return;
   error:
      goto done;
   }

   Importer::Importer()
   : _livingNum(0),
     _importedNum(0),
     _failedNum(0)
   {
      _options = NULL;
      _workQueue = NULL;
      _inited = FALSE;
      _refCount = 0;
   }

   Importer::~Importer()
   {
      for (vector<Worker*>::iterator i = _workers.begin(); i != _workers.end();)
      {
         Worker* worker = *i;
         SDB_OSS_DEL(worker);
         i = _workers.erase(i);
      }
   }

   INT32 Importer::init(Options* options,
                 RecordQueue* workQueue,
                 INT32 workerNum)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(NULL != options, "options can't be NULL");
      SDB_ASSERT(NULL != workQueue, "workQueue can't be NULL");

      _options = options;
      _workQueue = workQueue;

      string fileName = makeRecordLogFileName(_options->csname(),
                                              _options->clname(),
                                              string("import"));

      rc = _logFile.init(fileName, TRUE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init importer log file, rc=%d", rc);
         goto error;
      }

      if (_options->enableCoord())
      {
         rc = _coords.init(_options->hosts(),
                           _options->user(),
                           _options->password(),
                           _options->useSSL());
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to init coords, rc=%d", rc);
            goto error;
         }
      }

      for (INT32 i = 0; i < workerNum; i++)
      {
         string hostname;
         string svcname;
         ImporterArgs* args = NULL;

         if (_options->enableCoord())
         {
            rc = _coords.getRandomCoord(hostname, svcname);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get coord, rc=%d", rc);
               goto error;
            }
         }
         else
         {
            rc = Coords::getRandomCoord(_options->hosts(), _refCount,
                                        hostname, svcname);
            if (SDB_OK != rc)
            {
               PD_LOG(PDERROR, "failed to get coord, rc=%d", rc);
               goto error;
            }
         }

         args = SDB_OSS_NEW ImporterArgs();
         if (NULL == args)
         {
            rc = SDB_OOM;
            goto error;
         }

         args->id = i;
         args->hostname = hostname;
         args->svcname = svcname;
         args->self = this;

         Worker* worker = SDB_OSS_NEW Worker(_importerRoutine, args, TRUE);
         if (NULL == worker)
         {
            SDB_OSS_DEL(args);
            rc = SDB_OOM;
            PD_LOG(PDERROR, "failed to create importer Worker object");
            goto error;
         }

         _workers.push_back(worker);
      }

      _inited = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Importer::start()
   {
      INT32 rc = SDB_OK;
      INT32 num = _workers.size();

      SDB_ASSERT(num > 0, "must have woker");

      for (INT32 i = 0; i < num; i++)
      {
         Worker* worker = _workers[i];
         rc = worker->start();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to start importer");
            goto error;
         }

         _livingNum.inc();
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Importer::stop()
   {
      INT32 rc = SDB_OK;
      INT32 num = _workers.size();

      if (0 == num)
      {
         goto done;
      }

      for (INT32 i = 0; i < num; i++)
      {
         RecordArray* empty = NULL;

         _workQueue->push(empty);
      }

      for (vector<Worker*>::iterator i = _workers.begin(); i != _workers.end();)
      {
         Worker* worker = *i;

         rc = worker->waitStop();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to wait importer stop");
            i++;
            rc = SDB_OK;
         }
         else
         {
            SDB_OSS_DEL(worker);
            i = _workers.erase(i);
         }
      }

   done:
      return rc;
   }
}
