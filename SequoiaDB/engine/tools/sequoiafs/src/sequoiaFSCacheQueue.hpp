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

   Source File Name = sequoiaFSCacheQueue.hpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSCACHEQUEUE_HPP__
#define __SEQUOIAFSCACHEQUEUE_HPP__

#include "ossPriorityQueue.hpp"
#include "ossQueue.hpp"

#include "sequoiaFSDataCache.hpp"
#include "sequoiaFSFileLob.hpp"

namespace sequoiafs
{
   class sequoiaFSFileLobMgr;
   enum FS_TASK_TYPE
   {
      FS_TASK_FLUSH = 1,
      FS_TASK_DOWNLOAD
   } ;

   enum FS_TASK_PRIORITY
   {
      FS_TASK_PRIORITY_0 = 0,
      FS_TASK_PRIORITY_1 = 1
   } ;
   
   struct queueTask
   {
      public:
         UINT32 _hashKey;
         UINT32 _taskType;  // 1: upload, 2:download
         dataCache* _node;
         INT32  _flId;
         INT64  _offset;

      private:
         INT32 _priority ;

      public:
         queueTask( INT32 hashKey,
                    INT8 taskType, 
                    dataCache* node,
                    INT32 flId,
                    INT64 offset,
                    INT32 priority = 0 )
         {
            _hashKey = hashKey ;
            _taskType = taskType;
            _node = node;
            _priority = priority ;
            _flId = flId;
            _offset = offset;
         }
         queueTask()
         {
            _priority = 0 ;
         }

         bool operator< ( const queueTask &task ) const
         {
            if ( _priority < task._priority )
            {
               return true ;
            }
            return false ;
         }
   };
   
   class fsCacheQueue : public SDBObject
   {
      public:
         fsCacheQueue(sequoiaFSFileLobMgr* flMgr);
         ~fsCacheQueue(){}

         BOOLEAN addTask(queueTask task);
         void run();
         INT32 getQueueSize(){return _cacheQueue.size();}
         
      private:
         void _preReadCache(INT32 hashKey, INT32 flId, INT32 offset);
         void _flushCache(INT32 hashKey, dataCache* node);

      private:
         ossPriorityQueue<queueTask> _cacheQueue;
         sequoiaFSFileLobMgr * _flMgr;
   };
}
#endif

