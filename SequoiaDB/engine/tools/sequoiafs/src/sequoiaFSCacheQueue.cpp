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

   Source File Name = sequoiaFSCacheQueue.cpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2020  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSCacheQueue.hpp"
#include "sequoiaFSFileLobMgr.hpp"
#include "sequoiaFSDao.hpp"
#include "sequoiaFSDataCache.hpp"

using namespace sequoiafs;

fsCacheQueue::fsCacheQueue(sequoiaFSFileLobMgr* flMgr)
{
   _flMgr = flMgr;
}

void fsCacheQueue::run()
{
   while(_flMgr->getRunning())
   {
      struct queueTask task;
      if(_cacheQueue.timed_wait_and_pop(task, OSS_ONE_SEC))
      {
         if(FS_TASK_FLUSH == task._taskType)
         {
            _flushCache(task._hashKey, task._node);
         }
         else if(FS_TASK_DOWNLOAD == task._taskType)
         {
            _preReadCache(task._hashKey, task._flId, task._offset);
         }
      }
   }
}

BOOLEAN fsCacheQueue::addTask(queueTask task)
{
   BOOLEAN isAdd = TRUE;
   try
   {
      _cacheQueue.push(task);
   }
   catch(std::exception e2)
   {
      PD_LOG(PDERROR, "bad_alloc, error=%s", e2.what());
      isAdd = FALSE;
      goto error;
   }

done:
   return isAdd;

error:
   goto done;
}

void fsCacheQueue::_preReadCache(INT32 hashKey, INT32 flId, INT32 offset)
{
   cacheLoader* loader = _flMgr->getCacheLoader();

   PD_LOG(PDDEBUG, "preReadCache(), hashKey:%d, flId:%d, offset:%d", 
                    hashKey, flId, offset);

   if(loader->preLoadCheck(flId, offset))
   {
      loader->load(hashKey, flId, offset);
      loader->unPreLoad(flId, offset);
   }
}

void fsCacheQueue::_flushCache(INT32 hashKey, dataCache* node)
{
   INT32 rc = SDB_OK;
   BOOLEAN isLockedBucket = FALSE;
   BOOLEAN isLockedNode = FALSE;
   BOOLEAN isTryAgain = FALSE;
   INT64 newLobSize = 0;
   fileLob *file = NULL;
   cachePiece flushScope[SCOPE_SIZE];
   INT32      scopeLen = 0;
   fsConnectionDao db(_flMgr->getDataSouce());
   sequoiaFSHashBucket* hashBucket = _flMgr->getHashBucket();

   PD_LOG(PDDEBUG, "flushCache(), hashKey:%d, node:%d", hashKey, node);

   hashBucket->lockBucketR(hashKey);
   isLockedBucket = TRUE;

   if(FALSE == hashBucket->check(hashKey, node))
   {
      SDB_ASSERT( FALSE, "flush node has been delete" ) ;
      goto error;
   }

   file = _flMgr->getFileLob(node->_flId);
   if(NULL == file)
   {
      SDB_ASSERT( FALSE, "flId is invalid" ) ;
      goto error;
   }

   node->lockW();
   isLockedNode = TRUE;

   if(node->_dirty)
   {
      node->_dirty = FALSE;
   }
   else
   {
      if(node->_refCount > 0)
      {
         node->_refCount--;
      }
      node->unLockW();
      isLockedNode = FALSE;
      goto done;
   }

   scopeLen = node->_scopeLen;
   ossMemcpy(&flushScope[0], &node->_validScope[0], scopeLen * sizeof(cachePiece));

   for(INT32 i = 0; i < scopeLen; i++)
   {
      cachePiece p = flushScope[i];
      rc = db.writeLob(node->data + p.first - node->_offset, 
               file->getFullCL(),
               file->getOid(), 
               p.first, 
               (UINT32)(p.last - p.first + 1),
               &newLobSize);
      if(rc != SDB_OK)
      {
         if(SDB_LOB_PIECESINFO_OVERFLOW == rc)
         {
            if(node->_tryTime < 3)
            {
               struct queueTask task(hashKey, FS_TASK_FLUSH, node, -1, -1, FS_TASK_PRIORITY_0);
               if(addTask(task))
               {
                  PD_LOG(PDERROR, "Failed to writeLob, offset:%d, size:%d. try again, try time:%d. rc=%d", 
                                   p.first,  (UINT32)(p.last - p.first + 1), node->_tryTime, rc);
                  node->_tryTime++;
                  isTryAgain = TRUE;
                  //rc = SDB_OK;
                  goto done;
               }
            }
            else 
            {
               PD_LOG(PDERROR, "Failed to writeLob, offset:%d, size:%d. rc=%d", p.first,  (UINT32)(p.last - p.first + 1), rc);
               goto error;
            }
         }
         else 
         {
            PD_LOG(PDERROR, "Failed to writeLob, offset:%d, size:%d. rc=%d", p.first,  (UINT32)(p.last - p.first + 1), rc);
            goto error;
         }
      }
   }

   if(FALSE == node->_dirty)
   {
      node->clearPiece();
   }

   if(node->_refCount > 0)
   {
      node->_refCount--;
   }  

   node->unLockW();
   isLockedNode = FALSE;
      
done:
   if(isLockedNode)
   {
      if(!isTryAgain)
      {
         if(node != NULL && node->_refCount > 0)
         {
            node->_refCount--;
         }
      }
      node->unLockW();
   }

   if(isLockedBucket)
   {
      hashBucket->unLockBucketR(hashKey);
   }
   if(!isTryAgain)
   {
      if(file)
      {
         file->setErrorCode(rc);
         file->releaseShardRWLock();
      }
   }
   
   return;
   
error:
   goto done;
}



