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
   defect Date       Who Description
   ====== =========== === ==============================================
        10/30/2020  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSFileCreateHashBucket.hpp"
#include "sequoiaFSFileCreatingMgr.hpp"

using namespace sequoiafs; 

INT32 fileCreateHashBucket::hash(INT64 parentId, const CHAR* fileName)
{
   UINT32 hashKey = 0;
   CHAR hashStr[FS_MAX_NAMESIZE + 1];
   ossStrncpy(hashStr, fileName, FS_MAX_NAMESIZE);
   hashStr[FS_MAX_NAMESIZE] = '\0';

   hashKey = ossHash((const BYTE *)hashStr, ossStrlen(hashStr),
                     (const BYTE *)(&parentId), sizeof(parentId));
   
   return hashKey/_size;
}

void fileCreateHashBucket::lockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get_shared();
}

void fileCreateHashBucket::unLockBucketR(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release_shared(); 
}

void fileCreateHashBucket::lockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.get();
}

void fileCreateHashBucket::unLockBucketW(UINT32 key)
{
   _buckets[ key % _size ]._hashMutex.release(); 
}

createFileNode* fileCreateHashBucket::get(UINT32 key, INT64 parentId, const CHAR* name)
{
   createFileNode* cur = _buckets[ key % _size ].hashHead;
   INT32 visit = 0;

   while ( cur )
   {
      visit++;
      if(cur->meta.pid() == parentId
            && !ossStrncmp(cur->meta.name(), name, FS_MAX_NAMESIZE))
      {
         break;
      }

      cur = cur->bucketNext;
   }

   _getCount++;
   if(visit > 1)
   {
      _getConflictCount++;
   }

   return cur;
}

void fileCreateHashBucket::add(UINT32 key, createFileNode* node )
{
   createFileNode* cur = NULL;
   UINT32 hashKey = key % _size;

   PD_LOG(PDDEBUG, "hashbucket add(), key:%d, node:%d", key, node);

   _addCount++;
   if(_buckets[ hashKey ].hashHead == NULL)
   {
      _buckets[ hashKey ].hashHead = node;
      goto done;
   }
   _addConflictCount++;

   cur = _buckets[ hashKey ].hashHead;

   #ifdef _DEBUG
      while ( cur->bucketNext)
      {
         if(cur->meta.pid() == node->meta.pid()
               && !ossStrncmp(cur->meta.name(), node->meta.name(), FS_MAX_NAMESIZE))
         {
            SDB_ASSERT(FALSE, "dup node");
         }

         cur = cur->bucketNext;  
      }

      cur = _buckets[ hashKey ].hashHead;
   #endif

   cur->bucketPre = node;
   node->bucketNext = cur;

   _buckets[ hashKey ].hashHead = node;

done:
   return;
}

createFileNode* fileCreateHashBucket::del(UINT32 key, INT64 parentId, const CHAR* name)
{
   UINT32 hashKey = key % _size;
   createFileNode* cur = _buckets[ hashKey ].hashHead;
   INT32 visit = 0;

   PD_LOG(PDDEBUG, "hashbucket del(), key:%d, parentId:%d, name:%s", key, parentId, name);

   while ( cur)
   {
      visit++;
      if(cur->meta.pid() == parentId 
            && !ossStrncmp(cur->meta.name(), name, FS_MAX_NAMESIZE))
      {
         break;
      }

      cur = cur->bucketNext;
   }

   if(NULL == cur)
   {
      PD_LOG(PDDEBUG, "bucket del null, key:%d, parentId:%d, name:%s", 
                       key, parentId, name);
      goto done;
   }
      
   if(cur == _buckets[ hashKey ].hashHead)
   {
      if(cur->bucketNext)
      {
         cur->bucketNext->bucketPre = NULL;
         _buckets[ hashKey ].hashHead = cur->bucketNext;
      }
      else
      {
         _buckets[ hashKey ].hashHead = NULL;
      }
   }
   else
   {
      if(cur->bucketPre != NULL)
      {
         cur->bucketPre->bucketNext = cur->bucketNext;
      }
      if(cur->bucketNext != NULL)
      {
         cur->bucketNext->bucketPre = cur->bucketPre;
      }
   }

   cur->bucketPre = NULL;
   cur->bucketNext = NULL;

   _delCount++;
   if(visit > 1)
   {
      _delConflictCount++;
   }
  
done:
   return cur;
}

void fileCreateHashBucket::waitBucketClean()
{
   BOOLEAN isFind = TRUE;
   
   //add all node to upload queue
   while(isFind)
   {
      isFind = FALSE;
      for(UINT32 hashKey = 0; hashKey < _size; hashKey++)
      {
         lockBucketW(hashKey);

         createFileNode *cur = _buckets[hashKey].hashHead;
         while(cur != NULL)
         {
            isFind = TRUE;
            if(cur->handle != NULL)
            {
               CHAR fileName[FS_MAX_NAMESIZE+1] = {0};
               ossStrncpy(fileName, cur->meta.name(), FS_MAX_NAMESIZE);
               if(cur->handle->status != CREATE_STATUS_INDISK)
               {
                  if(SDB_OK == _createMgr->writeTmpFile(cur))
                  {
                     cur->handle->status = CREATE_STATUS_INDISK;
                  }
               }
               if(SDB_OK == _createMgr->addTask(cur->meta.pid(), fileName))
               {
                  cur->handle = NULL;
               }
            }
            cur = cur->bucketNext;
         }
         
         unLockBucketW(hashKey); 
      }
   }
}


