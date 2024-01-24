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

   Source File Name = sequoiaFSHashBucket.hpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
        10/30/2020    zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSMETAHASHBUCKET_HPP__
#define __SEQUOIAFSMETAHASHBUCKET_HPP__

#include <vector>
#include "ossLatch.hpp"

#include "sequoiaFSCommon.hpp"
#include "sequoiaFSLRUList.hpp"

namespace sequoiafs
{
/*
   struct dirNameId
   {
      public:
         INT64 _parentId;
         CHAR _hashStr[OSS_MAX_NAMESIZE + 1];

      public:
         dirNameId(const CHAR* hashStr, INT64 parentId)
         {
            ossMemset(_hashStr, 0, OSS_MAX_NAMESIZE + 1);
            ossStrncpy(_hashStr, hashStr, OSS_MAX_NAMESIZE);
            _parentId = parentId;
         }
   };
   */
   struct metaHashItem
   {
      ossSpinSLatch _hashMutex;
      dirMetaNode* hashHead;
   };

   class dirMetaCache;
   class sequoiaFSMetaHashBucket : public SDBObject
   {
      public:
         sequoiaFSMetaHashBucket(dirMetaCache * cache, UINT32 size = 10240)
         {
            _buckets.resize(size);
            _size = size;
            _dirCache = cache;
         }
         ~sequoiaFSMetaHashBucket(){}
         INT32 hashDirPid(INT64 parentId, const CHAR* dirName);
         void lockBucketR(UINT32 key);
         void unLockBucketR(UINT32 key);
         void lockBucketW(UINT32 key);
         void unLockBucketW(UINT32 key);
         BOOLEAN check(UINT32 key, dirMetaNode* node);
         dirMetaNode* get(UINT32 key, const CHAR* name, INT64 pid);
         void add(UINT32 key, dirMetaNode* node );
         dirMetaNode* del(UINT32 key, dirMetaNode* node );

         //INT32 getCallTimes(){return _callCount;}
         //INT32 getConflictTimes(){return _conflictCount;}
         INT32 getGetCallCount(){return _getCount;}
         INT32 getGetConflictCount(){return _getConflictCount;}
         INT32 getAddCallCount(){return _addCount;}
         INT32 getAddConflictCount(){return _addConflictCount;}
         INT32 getDelCallCount(){return _delCount;}
         INT32 getDelConflictCount(){return _delConflictCount;}
         void cleanMetaHashCounts()
         {
            _callCount = 0;
            _conflictCount = 0;
            _getCount = 0;
            _getConflictCount = 0;
            _addCount = 0;
            _addConflictCount = 0;
            _delCount = 0;
            _delConflictCount = 0;
         }

      private:
         vector<metaHashItem> _buckets;
         UINT32 _size;
         dirMetaCache * _dirCache;

         INT32 _callCount;
         INT32 _conflictCount;
         INT32 _getCount;
         INT32 _getConflictCount;
         INT32 _addCount;
         INT32 _addConflictCount;
         INT32 _delCount;
         INT32 _delConflictCount;
   };
}

#endif
