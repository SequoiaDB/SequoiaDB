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

   Source File Name = impSharding.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#include "impSharding.hpp"
#include "impMonitor.hpp"
#include "pd.hpp"
#include <iostream>
#include <sstream>

namespace import
{
   void _emptyShardingGroups(ShardingGroups* groups, RecordQueue* outQueue)
   {
      SDB_ASSERT(NULL != groups, "groups can't be NULL");
      SDB_ASSERT(NULL != outQueue, "outQueue can't be NULL");

      if (!groups->empty())
      {
         for (ShardingGroups::iterator it = groups->begin();
              it != groups->end();
              it++)
         {
            SubShardingGroups* subGroups = &(it->second);
            for (SubShardingGroups::iterator subIt = subGroups->begin();
                 subIt != subGroups->end();
                 subIt++)
            {
               RecordArray* array = subIt->second;
               outQueue->push(array);
            }

            subGroups->clear();
         }

         groups->clear();
      }
   }

   void _shardingRoutine(WorkerArgs* args)
   {
      Sharding* self = (Sharding*)args;
      RecordArray* records = NULL;
      INT32 rc = SDB_OK;
      INT32 shardingCount = 0;

      SDB_ASSERT(NULL != args, "arg can't be NULL");

      Options* options = self->_options;
      RecordQueue* inQueue = self->_inQueue;
      RecordQueue* outQueue = self->_outQueue;
      RecordSharding* sharding = &(self->_sharding);
      ShardingGroups* groups = &(self->_groups);
      LogFile* logFile = &(self->_logFile);
      Monitor* monitor = impGetMonitor();

      {
         stringstream ss;
         ss << "sharding started with "
            << sharding->getGroupNum() << " groups..."
            << std::endl;

         PD_LOG(PDEVENT, "%s", ss.str().c_str());
         if (options->verbose())
         {
            std::cout << ss.str();
         }
      }

      for(;;)
      {
         INT32 size;
         inQueue->wait_and_pop(records);
         if (NULL == records)
         {
            // stop signal
            break;
         }

         if (records->empty())
         {
            // empty signal
            _emptyShardingGroups(groups, outQueue);
            freeRecordArray(&records);
            PD_LOG(PDINFO, "empty sharding groups");
            continue;
         }

         size = records->size();
         for (INT32 i = 0; i < size; i++)
         {
            bson* record = records->get(i);
            SubShardingGroups::iterator it;
            string cl;
            SubShardingGroups* subGroups = NULL;
            UINT32 groupId = 0;

            rc = sharding->getGroupByRecord(record, cl, groupId);
            if (SDB_OK != rc)
            {
               INT32 ret;
               PD_LOG(PDERROR, "failed to get group by record, rc=%d", rc);
               if (SDB_OK != (ret = logFile->write(record)))
               {
                  PD_LOG(PDERROR, "failed to log record, rc=%d", ret);
               }
               monitor->recordsMemDec(bson_size(record));
               monitor->recordsNumDec(1);
               freeRecord(record);
               records->pop(i);

               self->_failedNum++;
               continue;
            }

            self->_shardingNum++;
            subGroups = &((*groups)[cl]);

            it = subGroups->find(groupId);
            if (it != subGroups->end())
            {
               // find the group
               RecordArray* array = it->second;

               SDB_ASSERT(!array->full(), "record array can't be full");

               array->push(record);
               if (array->full())
               {
                  array->finish();
                  outQueue->push(array);
                  subGroups->erase(it);
               }
            }
            else
            {
               // add new group
               RecordArray* array = NULL;

               rc = getRecordArray(options->batchSize(), &array);
               if (SDB_OK != rc)
               {
                  PD_LOG(PDERROR, "failed to get free record array, rc=%d", rc);
                  for (INT32 i = 0; i < records->size(); i++)
                  {
                     INT32 ret;
                     bson* obj = records->get(i);
                     if (NULL == obj)
                     {
                        continue;
                     }
                     if (SDB_OK != (ret = logFile->write(obj)))
                     {
                        PD_LOG(PDERROR, "failed to log write records, rc=%d", ret);
                        break;
                     }
                  }
                  freeRecordArray(&records);
                  goto error;
               }

               array->push(record);
               if (array->full())
               {
                  array->finish();
                  outQueue->push(array);
               }
               else
               {
                  (*subGroups)[groupId] = array;
               }
            }

            shardingCount++;
            // clear this record in array,
            // otherwise the bson will be freed by freeRecordArray
            records->pop(i);
         }

         freeRecordArray(&records);
      }

   done:
      _emptyShardingGroups(groups, outQueue);
      self->_stopped = TRUE;

      {
         stringstream ss;
         ss << "sharding stopped, sharding records "
            << shardingCount << "."
            << std::endl;

         PD_LOG(PDEVENT, "%s", ss.str().c_str());
         if (options->verbose())
         {
            std::cout << ss.str();
         }
      }
      return;
   error:
      goto done;
   }

   Sharding::Sharding()
   {
      _options = NULL;
      _inQueue = NULL;
      _outQueue = NULL;
      _inited = FALSE;
      _worker = NULL;
      _stopped = TRUE;
      _shardingNum = 0;
      _failedNum = 0;
   }

   Sharding::~Sharding()
   {
      SAFE_OSS_DELETE(_worker);
   }

   INT32 Sharding::init(Options* options,
                        RecordQueue* inQueue,
                        RecordQueue* outQueue)
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(!_inited, "can't init again");
      SDB_ASSERT(NULL != options, "options can't be NULL");
      SDB_ASSERT(NULL != inQueue, "inQueue can't be NULL");
      SDB_ASSERT(NULL != outQueue, "outQueue can't be NULL");

      _options = options;
      _inQueue = inQueue;
      _outQueue = outQueue;

      string shardingLogFile = makeRecordLogFileName(_options->csname(),
                                                     _options->clname(),
                                                     string("sharding"));

      rc = _logFile.init(shardingLogFile, FALSE);
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init sharding log file");
         goto error;
      }

      if ( !_options->enableSharding() ||
           _options->batchSize() <= 1 )
      {
         // no need to do anything
         _inited = TRUE ;
         goto done ;
      }

      rc = _sharding.init(_options->hosts(),
                          _options->user(),
                          _options->password(),
                          _options->csname(),
                          _options->clname(),
                          _options->useSSL());
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to init sharding, rc=%d", rc);
         goto error;
      }

      SDB_ASSERT(_sharding.getGroupNum() >= 0,
                 "groupNum must be greater than or equals 0");

      if ( _sharding.getGroupNum() <= 1 )
      {
         // no need to do anything
         _inited = TRUE ;
         goto done ;
      }

      _worker = SDB_OSS_NEW Worker(_shardingRoutine, this);
      if (NULL == _worker)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "failed to create sharding Worker object");
         goto error;
      }

      _inited = TRUE;

   done:
      return rc;
   error:
      goto done;
   }

   BOOLEAN Sharding::needSharding() const
   {
      SDB_ASSERT(_inited, "must be inited");

      return (_options->enableSharding() &&
              _options->batchSize() > 1 &&
              _sharding.getGroupNum() > 1);
   }

   INT32 Sharding::getGroupNum() const
   {
      SDB_ASSERT(_inited, "must be inited");

      return _sharding.getGroupNum();
   }

   INT32 Sharding::start()
   {
      INT32 rc = SDB_OK;

      SDB_ASSERT(_inited, "must be inited");

      rc = _worker->start();
      if (SDB_OK != rc)
      {
         PD_LOG(PDERROR, "failed to start parser");
         goto error;
      }

      _stopped = FALSE;

   done:
      return rc;
   error:
      goto done;
   }

   INT32 Sharding::stop()
   {
      INT32 rc = SDB_OK;

      if(_inited && NULL != _worker)
      {
         RecordArray* empty = NULL;

         // push empty RecordArray as stop signal
         _inQueue->push(empty);

         rc = _worker->waitStop();
         if (SDB_OK != rc)
         {
            PD_LOG(PDERROR, "failed to wait the sharding stop");
         }

         SAFE_OSS_DELETE(_worker);
      }
      return rc;
   }
}
