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

   Source File Name = sequoiaFSHashBucket.cpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSHashBucket.hpp"
#include "sequoiaFSFileLob.hpp"
#include "sequoiaFSFileLobMgr.hpp"

using namespace sequoiafs; 

UINT32 sequoiaFSHashBucket::hash(INT32 flId, INT64 offset)
{
   UINT32 hashKey = 0;

   struct hashElem elem(flId, offset);
   hashKey = ossHash((CHAR *)&elem, sizeof(hashElem), 5) % _size; //ossHash(char ,size ,)
   
   return hashKey;
}

void sequoiaFSHashBucket::lockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get_shared();
}

BOOLEAN sequoiaFSHashBucket::tryLockBucketR(UINT32 key)
{
   if(_buckets[ key % _size ]._hashMutex.try_get_shared())
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

void sequoiaFSHashBucket::unLockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release_shared();
}

void sequoiaFSHashBucket::lockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get();
}

BOOLEAN sequoiaFSHashBucket::tryLockBucketW(UINT32 key)
{
   if(_buckets[ key % _size ]._hashMutex.try_get())
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

void sequoiaFSHashBucket::unLockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release(); 
}

BOOLEAN sequoiaFSHashBucket::check(UINT32 key, dataCache* node)
{
   BOOLEAN find = FALSE;
   
   dataCache* cur = _buckets[ key % _size ].hashHead;
   while ( cur )
   {
      if(cur == node)
      {
         find = TRUE;
         break;
      }
      cur = cur->_bucketNext;
   }

   return find;
}

dataCache* sequoiaFSHashBucket::get(UINT32 key, INT32 flId, INT64 offset)
{
   dataCache* cur = _buckets[ key % _size ].hashHead;
   INT32 visit = 0;

   while ( cur )
   {
      visit++; 
      if(cur->_flId == flId && cur->_offset == offset)
      {
         break;
      }

      cur = cur->_bucketNext;
   }

   _countMutex.get();
   _callCount++;
   if(visit > 1)
   {
      _conflictCount++;
   }
   _countMutex.release();

   return cur;
}

void sequoiaFSHashBucket::add(UINT32 key, dataCache* node )
{
   dataCache* cur = NULL;

   PD_LOG(PDDEBUG, "hashbucket add(), key:%d, node:%d", key, node);

   if(_buckets[ key % _size ].hashHead == NULL)
   {
      _buckets[ key % _size ].hashHead = node;
      goto done;
   }

   cur = _buckets[ key % _size ].hashHead;
   _countMutex.get();
   _callCount++;
   if(cur != NULL)
   {
      _conflictCount++;
   }
   _countMutex.release();
   while ( cur->_bucketNext )
   {
      if(cur->_flId == node->_flId && cur->_offset == node->_offset)
      {
         SDB_ASSERT( FALSE, "dup node" ) ;
      }

      cur = cur->_bucketNext;
   }

   cur->_bucketNext = node;
   node->_bucketPre = cur;

done:
   return;
}

void sequoiaFSHashBucket::del(UINT32 key, dataCache* node )
{
   PD_LOG(PDDEBUG, "hashbucket del(), key:%d, node:%d", key, node);
   dataCache* cur = _buckets[ key % _size ].hashHead;
   INT32 visit = 0;

   while(cur)
   {
      visit++;
      if(cur->_flId == node->_flId && cur->_offset == node->_offset)
      {
         break;
      }
      cur = cur->_bucketNext;
   }

   if(NULL == cur)
   {
      PD_LOG(PDERROR, "bucket del null, key:%d, node:%d", key, node);
      //SDB_ASSERT( FALSE, "node is null" ) ;
      goto done;
   }
   else
   {
      cur->lockW();
      if(cur == _buckets[ key % _size ].hashHead)
      {
         if(cur->_bucketNext)
         {
            cur->_bucketNext->_bucketPre = NULL;
            _buckets[ key % _size ].hashHead = cur->_bucketNext;
         }
         else
         {
            _buckets[ key % _size ].hashHead = NULL;
         }
      }
      else
      {
         if(cur->_bucketPre != NULL)
         {
            cur->_bucketPre->_bucketNext = cur->_bucketNext;
         }
         if(cur->_bucketNext != NULL)
         {
            cur->_bucketNext->_bucketPre = cur->_bucketPre;
         }
      }
      cur->unLockW();
   }

   cur->_bucketPre = NULL;
   cur->_bucketNext = NULL;

   _countMutex.get();
   _callCount++;
   if(visit > 1)
   {
      _conflictCount++;
   }
   _countMutex.release();

done:
   return;
}

void sequoiaFSHashBucket::cleanOverTimeCache()
{
   cacheLoader* loader = _flMgr->getCacheLoader();
   INT64 count = 0;
   
   while(_flMgr->getRunning())
   {
      if(!needClean)
      {
         ossSleepsecs(3);
         continue;
      }
      
      for(UINT32 hashKey = 0; hashKey < _size; hashKey++)
      {
         //try lock hashbucket
         if(!tryLockBucketW(hashKey))
         {
            continue;
         }
         
         // find overtimeCache in the bucket
         UINT64 nowTime = ossGetCurrentMilliseconds();
         dataCache *cur = _buckets[hashKey].hashHead;
         dataCache *next;
         while(cur)
         {
            next = cur->_bucketNext;
            INT32 allocTime = nowTime - cur->_time;
            if((allocTime > 3000 && cur->_isReaded)
                || (allocTime > 30000))
            {
               fileLob *file = _flMgr->getFileLob(cur->_flId);
               if(file)
               {
                  if((FALSE == cur->_dirty) && (0 == cur->_refCount))
                  {
                     if(file->tryDelNode(cur))
                     {
                        del(hashKey, cur);
                        //SAFE_OSS_DELETE(cur->_rwMutex);
                        loader->releaseCache(cur);
                        count++;
                     }
                  }
                  else if((TRUE == cur->_dirty) && (0 == cur->_refCount))
                  {
                     file->addQueue(hashKey, cur);
                  }
               }
            }
            cur = next;
         }

         unLockBucketW(hashKey);
      }
      PD_LOG(PDEVENT, "cleanOverTimeCache %d times", count);
      needClean = false;
      count = 0;
   }
}

