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

   Source File Name = impRoutine.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impRoutine.hpp"
#include "impHosts.hpp"
#include <iostream>
#include <sstream>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace pt = boost::posix_time;

namespace import
{
   Routine::Routine(Options& options)
   : _options(options)
   {
   }

   Routine::~Routine()
   {
      {
         if (!_parsedQueue.empty())
         {
            RecordArray* array = NULL;
            while (_parsedQueue.try_pop(array))
            {
               if (NULL != array)
               {
                  freeRecordArray(&array);
               }
            }
         }
      }

      {
         if (!_shardingQueue.empty())
         {
            RecordArray* array = NULL;
            while (_shardingQueue.try_pop(array))
            {
               if (NULL != array)
               {
                  freeRecordArray(&array);
               }
            }
         }
      }

   }

   INT32 Routine::run()
   {
      INT32 rc = SDB_OK;
      pt::ptime startTime;
      pt::ptime endTime;

      PD_LOG(PDINFO, "begin importing");

      startTime = pt::second_clock::universal_time();

      rc = _startSharding();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start sharding, rc=%d", rc);
         goto stop;
      }

      rc = _startImporter(_options.jobs());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start importers, rc=%d", rc);
         goto stop;
      }

      rc = _startParser();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start parser, rc=%d", rc);
         goto stop;
      }

      while (!_parser.isStopped() && !_importer.isStopped())
      {
         if (_sharding.needSharding())
         {
            if (_sharding.isStopped())
            {
               break;
            }
         }
         ossSleep(100);
      }

   stop:
      rc = _waitParserStop();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait parser stop, rc=%d", rc);
      }

      rc = _stopSharding();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to stop importers, rc=%d", rc);
      }

      rc = _stopImporter();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to stop importers, rc=%d", rc);
      }

      endTime = pt::second_clock::universal_time();

      if (SDB_OK == rc && _importer.importedNum() > 0)
      {
         pt::time_duration time = endTime - startTime;
         INT64 sec = time.total_seconds();

         stringstream ss;
         ss << "import " << _importer.importedNum() << " records in "
            << sec << " second(s)";
         if (sec > 0)
         {
            ss << ", average "
               << _importer.importedNum() / sec << " records/s";
         }
         PD_LOG(PDINFO, "%s", ss.str().c_str());
      }

      PD_LOG(PDINFO, "finished importing");
      return rc;
   }

   INT32 Routine::_startImporter(INT32 workerNum)
   {
      INT32 rc = SDB_OK;

      if (_sharding.needSharding())
      {
         rc = _importer.init(&_options, &_shardingQueue, workerNum);
      }
      else
      {
         rc = _importer.init(&_options, &_parsedQueue, workerNum);
      }

      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init importer, rc=%d", rc);
         goto error;
      }

      rc = _importer.start();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start importer, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Routine::_stopImporter()
   {
      INT32 rc = SDB_OK;

      rc = _importer.stop();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to stop importer, rc=%d", rc);
      }

      return rc;
   }

   INT32 Routine::_startParser()
   {
      INT32 rc = SDB_OK;

      rc = _parser.init(&_options, &_parsedQueue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init parser, rc=%d", rc);
         goto error;
      }

      rc = _parser.start();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start parser, rc=%d", rc);
         goto error;
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Routine::_waitParserStop()
   {
      INT32 rc = SDB_OK;

      rc = _parser.stop();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to wait the parser stop");
      }

      return rc;
   }

   INT32 Routine::_startSharding()
   {
      INT32 rc = SDB_OK;

      rc = _sharding.init(&_options,
                          &_parsedQueue,
                          &_shardingQueue);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init sharding, rc=%d", rc);
         goto error;
      }

      if (_sharding.needSharding())
      {
         rc = _sharding.start();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to start sharding, rc=%d", rc);
            goto error;
         }
      }
      else
      {
         CHAR* str = "no need to sharding";
         PD_LOG(PDINFO, "%s", str);
         if (_options.verbose())
         {
            stringstream ss;
            ss << str << std::endl;
            std::cout << ss.str();
         }
      }

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Routine::_stopSharding()
   {
      INT32 rc = SDB_OK;

      rc = _sharding.stop();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to stop importer, rc=%d", rc);
      }

      return rc;
   }

   void Routine::printStatistics()
   {
      stringstream ss;

      ss << "parsed records: " << _parser.parsedNum() << std::endl
         << "parse failure: " << _parser.failedNum() << std::endl;

      ss << "sharding records: " << _sharding.shardingNum() << std::endl
         << "sharding failure: " << _sharding.failedNum() << std::endl;

      ss << "imported records: " << _importer.importedNum() << std::endl
         << "import failure: " << _importer.failedNum() << std::endl;

      if (_parser.failedNum() > 0)
      {
         ss << "see " << _parser.logFileName()
            << " for parse failure records" << std::endl;
      }

      if (_sharding.failedNum() > 0)
      {
         ss << "see " << _sharding.logFileName()
            << " for sharding failure records" << std::endl;
      }

      if (_importer.failedNum() > 0)
      {
         ss << "see " << _importer.logFileName()
            << " for import failure records" << std::endl;
      }

      string stat = ss.str();

      std::cout << stat;
      PD_LOG(PDEVENT, stat.c_str());
   }
}

