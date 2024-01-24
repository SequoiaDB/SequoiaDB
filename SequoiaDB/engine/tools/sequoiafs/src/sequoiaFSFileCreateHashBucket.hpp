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

#ifndef __SEQUOIAFS_FILECREATE_HASHBUCKET_HPP__
#define __SEQUOIAFS_FILECREATE_HASHBUCKET_HPP__

#include <vector>
#include "ossLatch.hpp"

#include "sequoiaFSCommon.hpp"

namespace sequoiafs
{
   class createFileNode : public SDBObject
   {
      public:
         createFileNode(int k, fileMeta* v)
            : key(k), 
              handle(NULL),
              tryTime(0),
              bucketPre(NULL), 
              bucketNext(NULL),
              data(NULL)
         {
            meta.setName(v->name());
            meta.setMode(v->mode());
            meta.setUid(v->uid());
            meta.setGid(v->gid());
            meta.setPid(v->pid());
            meta.setNLink(v->nLink());
            meta.setLobOid(v->lobOid());
            meta.setSize(v->size());
            meta.setAtime(v->atime());
            meta.setCtime(v->ctime());
            meta.setMtime(v->mtime());
            meta.setSymLink(v->symLink());
         }

         INT32 getKey(){return key;}
         
      public:
         INT32 key;
         INT32 status;
         lobHandle* handle;
         INT32 tryTime;
         fileMeta meta;
         createFileNode *bucketPre;
         createFileNode *bucketNext;
         CHAR*       data; 
   };
   
   struct fileHashItem
   {
      ossSpinSLatch _hashMutex;
      createFileNode* hashHead;
   };

   class fileCreatingMgr;
   class fileCreateHashBucket : public SDBObject
   {
      public:
         fileCreateHashBucket(fileCreatingMgr * createMgr, UINT32 size = 10240)
         {
            _buckets.resize(size);
            _size = size;
            _createMgr = createMgr;
         }
         ~fileCreateHashBucket(){}
         INT32 hash(INT64 parentId, const CHAR* fileName);
         void lockBucketR(UINT32 key);
         void unLockBucketR(UINT32 key);
         void lockBucketW(UINT32 key);
         void unLockBucketW(UINT32 key);
         createFileNode* get(UINT32 key, INT64 parentId, const CHAR* name);
         void add(UINT32 key, createFileNode* node );
         createFileNode* del(UINT32 key, INT64 parentId, const CHAR* name);
         void waitBucketClean();

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
         vector<fileHashItem> _buckets;
         UINT32 _size;
         fileCreatingMgr* _createMgr;

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
