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

   Source File Name = impSharding.hpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          7/7/2015  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef IMP_SHARDING_HPP_
#define IMP_SHARDING_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "impRecordSharding.hpp"
#include "impRecordQueue.hpp"
#include "impWorker.hpp"
#include "impOptions.hpp"
#include "impLogFile.hpp"
#include <map>

using namespace std;

namespace import
{
   typedef map<UINT32, RecordArray*> SubShardingGroups;
   typedef map<string, SubShardingGroups> ShardingGroups;

   class Sharding: public WorkerArgs
   {
   public:
      Sharding();
      ~Sharding();
      INT32 init(Options* options,
                 RecordQueue* inQueue,
                 RecordQueue* outQueue);
      BOOLEAN needSharding() const;
      INT32 getGroupNum() const;
      INT32 start();
      INT32 stop();
      inline BOOLEAN isStopped() const { return _stopped; }
      inline INT64 shardingNum() const { return _shardingNum; }
      inline INT64 failedNum() const { return _failedNum; }
      inline const string& logFileName() const
      {
         return _logFile.fileName();
      }

   private:
      Options*          _options;
      RecordQueue*      _inQueue;
      RecordQueue*      _outQueue;
      BOOLEAN           _inited;

      RecordSharding    _sharding;
      Worker*           _worker;
      BOOLEAN           _stopped;

      ShardingGroups    _groups;

      LogFile           _logFile;

      INT64             _shardingNum;
      INT64             _failedNum;

      friend void _shardingRoutine(WorkerArgs* args);
   };
}

#endif /* IMP_SHARDING_HPP_ */
