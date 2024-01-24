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

   Source File Name = sequoiaFSCacheLoader.cpp

   Descriptive Name = sequoiafs meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSCacheLoader.hpp"
#include "sequoiaFSHashBucket.hpp"
#include "sequoiaFSFileLob.hpp"
#include "sequoiaFSCacheQueue.hpp"
#include "sequoiaFSFileLobMgr.hpp"

using namespace engine;
using namespace sequoiafs;

INT32 cacheLoader::load(UINT32 hashKey, INT32 flId, INT64 offset)
{
   INT32 rc = SDB_OK;
   INT32 tryTime = 2;
   UINT32 mapKey = hash(flId, offset);
   UINT32 bucketKey = hashKey;
   ossEvent* tmpPtr = NULL;
   ossPoolMap<int, eventPtr>::iterator iter;
   eventPtr event;
   BOOLEAN loading = FALSE;
   INT32 result = SDB_OK ;
   dataCache* node = NULL;
   INT32 allocLen = FS_CACHE_LEN + sizeof(dataCache);
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();
   fileLob *fl = _mgr->getFileLob(flId);
   fsConnectionDao db(_mgr->getDataSouce());

   PD_LOG(PDDEBUG, "load(), hashKey:%d, mapKey:%d, flId:%d, offset:%d", hashKey, mapKey, flId, offset);

   if(NULL == fl)
   {
      rc = SDB_SYS;
      SDB_ASSERT(FALSE, "fl is null.");
      PD_LOG(PDERROR, "fl is null, flId=%d", flId);
      goto error;
   }

   while(tryTime)
   {
      _loadMapMutex.get();
      tryTime--;
      iter = _loadMap.find(mapKey);
      if(iter != _loadMap.end())
      {
         //there is another task for the same key, wait
         event = iter->second;
         _loadMapMutex.release();

         rc = event->wait(-1, &result);
         if(rc != SDB_OK || EVENT_FAILED == result)
         {
            PD_LOG(PDERROR, "event wait failed, rc=%d, result=%d", rc, result);
            goto error;
         }
         if(EVENT_ALLOC_SUCCESS == result)
         {
            continue;
         }
         break;
      }
      else
      {
         // add a key to the map
         tmpPtr = SDB_OSS_NEW _ossEvent();
         if(NULL == tmpPtr)
         {
            PD_LOG(PDERROR, "alloc event failed, flId=%d, offset=%d", flId, offset);
            rc = SDB_OOM;
            goto error;
         }
         event = eventPtr(tmpPtr);
         try
         {
            _loadMap[mapKey] = event;
         }
         catch(std::exception &e)
         {
            _loadMapMutex.release();
            PD_LOG(PDERROR, "add map failed, error=%s", e.what());
            rc = SDB_OOM;
            goto error;
         }
         _loadMapMutex.release();
         loading = TRUE;

         hashBucket->lockBucketR(bucketKey);
         node = hashBucket->get(bucketKey, flId, offset);  
         if(NULL != node)
         {
            node->lockW();
            rc = node->cacheDownLoad(&db, fl->getFullCL(), fl->getOid());
            node->unLockW();
            hashBucket->unLockBucketR(bucketKey);
            if(SDB_OK != rc)
            {
               PD_LOG(PDERROR, "cacheDownLoad failed, error=%d", rc);
               goto error;
            }            
         }
         else
         {
            hashBucket->unLockBucketR(bucketKey);
            
            node = _getNewCache(allocLen);
            if(NULL == node)
            {
               PD_LOG(PDERROR, "alloc cache failed, flId=%d, offset=%d", flId, offset);
               rc = SDB_OOM;
               goto error;
            }
   
            node->init(flId, offset);

            rc = node->cacheDownLoad(&db, fl->getFullCL(), fl->getOid());
            if(SDB_OK != rc)
            {
               PD_LOG(PDERROR, "cacheDownLoad failed, error=%d", rc);
               releaseCache(node);
               goto error;
            }

            hashBucket->lockBucketW(bucketKey);
            hashBucket->add(bucketKey, node); 
            hashBucket->unLockBucketW(bucketKey);

            fl->addNode(node);
         }

         _preSetMutex.get();
         _preReadSet.erase(mapKey);
         _preSetMutex.release();
         event->signalAll(EVENT_DOWNLOAD_SUCCESS);
         break;
      }
   }

done:
   if(loading)
   {
      _loadMapMutex.get();
      _loadMap.erase(mapKey);
      _loadMapMutex.release();
   }
   return rc;

error:
   if(loading)
   {
      event->signalAll(EVENT_FAILED);
   }
   goto done;
}

INT32 cacheLoader::loadEmpty(INT32 flId, INT64 offset)
{
   INT32 rc = SDB_OK;
   ossEvent* tmpPtr = NULL;
   ossPoolMap<int, eventPtr>::iterator iter;
   eventPtr event;
   BOOLEAN loading = FALSE;
   INT32 result = SDB_OK ;
   dataCache* node = NULL;
   INT32 allocLen = FS_CACHE_LEN + sizeof(dataCache);
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();
   fileLob *fl = _mgr->getFileLob(flId);
   UINT32 mapKey = hash(flId, offset);
   UINT32 bucketKey = 0;

   PD_LOG(PDDEBUG, "loadEmpty(), flId:%d, offset:%d", flId, offset);

   if(NULL == fl)
   {
      rc = SDB_OOM;
      goto error;
   }

   _loadMapMutex.get();
   iter = _loadMap.find(mapKey);
   if(iter != _loadMap.end())
   {
      //there is another task for the same key, wait
      //event = loadMap[mapKey];
      event = iter->second;
      _loadMapMutex.release();
      rc = event->wait(-1, &result);
      if(rc != SDB_OK || EVENT_FAILED == result)
      {
         PD_LOG(PDERROR, "event wait failed, rc=%d, result=%d", rc, result);
         goto error;
      }
   }
   else
   {
      // add a key to the map
      tmpPtr = SDB_OSS_NEW _ossEvent();
      if(NULL == tmpPtr)
      {
         PD_LOG(PDERROR, "alloc event failed, flId=%d, offset=%d", flId, offset);
         rc = SDB_OOM;
         goto error;
      }
      event = eventPtr(tmpPtr);
      try
      {
         _loadMap[mapKey] = event;
      }
      catch(std::exception &e)
      {
         _loadMapMutex.release();
         PD_LOG(PDERROR, "add map failed, error=%s", e.what());
         rc = SDB_OOM;
         goto error;
      }
      _loadMapMutex.release();
      loading = TRUE;

      bucketKey = hashBucket->hash(flId, offset);
      hashBucket->lockBucketW(bucketKey);

      node = hashBucket->get(bucketKey, flId, offset);  
      if(NULL == node)
      {
         node = _getNewCache(allocLen);
         if(NULL == node)
         {
            hashBucket->unLockBucketW(bucketKey);
            PD_LOG(PDERROR, "alloc cache failed, flId=%d, offset=%d", flId, offset);
            rc = SDB_OOM;
            goto error;
         }

         node->init(flId, offset);
         hashBucket->add(bucketKey, node); 
         fl->addNode(node);
         hashBucket->unLockBucketW(bucketKey);
      }
      else
      {
         hashBucket->unLockBucketW(bucketKey);
      }
      event->signalAll(EVENT_ALLOC_SUCCESS);     
   }
   
done:
   if(loading)
   {
      _loadMapMutex.get();
      _loadMap.erase(mapKey);
      _loadMapMutex.release();
   }
   return rc;

error:
   if(loading)
   {
      event->signalAll(EVENT_FAILED);
   }
   goto done;
}

void cacheLoader::preLoad(INT32 flId, INT64 offset)
{
   UINT32 mapKey = hash(flId, offset);
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();

   PD_LOG(PDDEBUG, "preLoad(), flId:%d, offset:%d", flId, offset);
   
   _preSetMutex.get();
   if(_preReadSet.find(mapKey) == _preReadSet.end())
   {
      struct queueTask task(hashBucket->hash(flId, offset), FS_TASK_DOWNLOAD, NULL, flId, offset, FS_TASK_PRIORITY_0);
      if(_mgr->getCacheQueue()->addTask(task))
      {
         try
         {
            _preReadSet.insert(mapKey);
         }
         catch(std::exception &e)
         {
            _preSetMutex.release();
            PD_LOG(PDERROR, "add set failed, error=%s", e.what());
            goto error;
         }
      }
   }
   _preSetMutex.release();

error:
   return;
}

BOOLEAN cacheLoader::preLoadCheck(INT32 flId, INT64 offset)
{
   UINT32 mapKey = hash(flId, offset);
   BOOLEAN isFind = FALSE;
   
   _preSetMutex.get();
   if(_preReadSet.find(mapKey) != _preReadSet.end())
   {
      isFind = TRUE;
   }
   _preSetMutex.release();

   return isFind;   
}

void cacheLoader::unPreLoad(INT32 flId, INT64 offset)
{
   UINT32 mapKey = hash(flId, offset);
   PD_LOG(PDDEBUG, "unPreLoad(), flId:%d, offset:%d", flId, offset);
   
   _preSetMutex.get();
   _preReadSet.erase(mapKey);
   _preSetMutex.release();
}

UINT32 cacheLoader::hash(INT32 flId, INT64 offset)
{
   return ossHash( ( const BYTE * )(&flId), sizeof(flId),  
                   ( const BYTE * )(&offset), sizeof(offset) );
}

dataCache* cacheLoader::_getNewCache(INT32 allocLen)
{
   dataCache* node = NULL;

   _countMutex.get();
   if(_usedCount > _capaCount)
   {
      PD_LOG(PDERROR, "_usedCount(%d) > _capacity(%d)", _usedCount * 4, _capacity);
      _mgr->getHashBucket()->setCacheClean();
      goto done;
   }
   
   node = (dataCache* )SDB_THREAD_ALLOC(allocLen);
   if(NULL != node)
   {
      ++_usedCount;
      if(_usedCount > _capaCount*0.7)
      {
         _mgr->getHashBucket()->setCacheClean();
      }
   }
   
done:
   _countMutex.release();
   return node;
}

void cacheLoader::releaseCache(dataCache* node)
{
   _countMutex.get();
   --_usedCount;
   SAFE_OSS_DELETE(node->_rwMutex);
   SDB_THREAD_FREE(node);
   _countMutex.release();
}

