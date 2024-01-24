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

   Source File Name = sequoiaFSFileLob.cpp

   Descriptive Name = sequoiafs fuse file-lob manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2020  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSFileLob.hpp"

#include <time.h>
#include "ossUtil.h"
#include "sequoiaFSFileLobMgr.hpp"
#include "sequoiaFSHashBucket.hpp"
#include "sequoiaFSDao.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSCacheQueue.hpp"

using namespace engine;
using namespace sdbclient;
using namespace bson;
using namespace sequoiafs;

void fileLob::init(INT32 flId, 
                   sdbConnectionPool* ds, 
                   sequoiaFSFileLobMgr* mgr, 
                   INT32 preReadBlock)
{
   _flId = flId;
   _dataSource = ds;
   _mgr = mgr;
   _preReadBlock = preReadBlock;
}

INT32 fileLob::flOpen(const CHAR *clFullName, 
                      const OID &oid, INT32 fflag, INT64 size)  
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;

   PD_LOG(PDDEBUG, "flopen(), clFullName:%s, oid:%s, flags:%d", 
                    clFullName, oid.toString().c_str(), fflag);

   _fileSize = size;
   ossStrncpy(_fullCL, clFullName, sizeof(_fullCL) - 1);
   _oid = oid;
   _fflag = fflag;
   _errCode = SDB_OK;
   _fileHead = NULL;
   _writeFlag = FALSE;

   _readCount = 0;
   _downLoadCount = 0;

   return rc;   
}

INT32 fileLob::flwrite(INT64 offset,
                       INT64 size,
                       const CHAR *buf)
{
   INT32 rc = SDB_OK;
   INT64 newOffset = 0;
   INT64 writeSize = size;
   INT64 writeLen = 0;
   INT64 cacheNo = 0;
   INT64 cacheNo2 = 0;

   PD_LOG(PDDEBUG, "flwrite(), offset:%ld, size:%ld, flId:%d", offset, size, _flId);
   
   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   cacheNo = offset >> FS_CACHE_OFFSETBIT;
   cacheNo2 = (offset + size - 1) >> FS_CACHE_OFFSETBIT;

   if(cacheNo2 > cacheNo)
   {
      newOffset = (cacheNo + 1) << FS_CACHE_OFFSETBIT;
      writeSize = newOffset - offset;
   }
   
   rc = _lwrite(offset, writeSize, buf);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to lwrite,  offset, size, rc=%d", offset, size, rc);
      goto error;
   }
   writeLen = writeSize;
   while(writeLen < size)
   {
      writeSize = min64(size - writeLen, FS_CACHE_LEN);
      rc = _lwrite(newOffset, writeSize, buf + writeLen);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "Failed to lwrite, offset:%d, size:%d, rc=%d", offset, size, rc);
         goto error;
      }
      
      writeLen += writeSize;
      newOffset += FS_CACHE_LEN;
   }

   flUpdateSize(offset+size);
   _writeFlag = TRUE;

done:
   return rc;
   
error:
   _errCode = rc;
   goto done;
}

INT32 fileLob::flRead(INT64 offset, 
                      INT64 size, 
                      CHAR *buf, 
                      INT32 *len)
{
   INT32 rc = SDB_OK;
   INT64 newOffset = 0;
   INT32 readOnceLen = 0;
   INT32 readLen = 0;
   INT32 readSize = size;
   INT32 totalSize = size;
   INT64 cacheNo = 0;
   INT64 cacheNo2 = 0;

   PD_LOG(PDDEBUG, "flRead(), offset:%d, size:%d, flId:%d", offset, size, _flId);

   // read errcode, if != SDB_OK return errcode
   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   if(0 == _fileSize)
   {
      *len = 0;
      goto done;
   }

   if(offset >= _fileSize)
   {
      //PD_LOG(PDERROR, "offset >= filesize, offset=%d, filesize=%d", offset, _fileSize);
      *len = 0;
      goto done;
   }

   if(offset+size > _fileSize)
   {
      totalSize = _fileSize - offset;
      readSize = totalSize ;
   }
   
   cacheNo = offset >> FS_CACHE_OFFSETBIT;
   cacheNo2 = (offset + totalSize - 1) >> FS_CACHE_OFFSETBIT;
   
   if(cacheNo2 > cacheNo)
   {
      newOffset = (cacheNo + 1) << FS_CACHE_OFFSETBIT;
      readSize = newOffset - offset;
   }

   _preRead(size, offset);  
   
   rc = _lread(offset, readSize, buf, &readOnceLen);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "fail to lread, offset=%d, size=%d, rc=%d", offset, size,rc);
      goto error;
   }
   readLen += readOnceLen;
   while(readLen < totalSize && readOnceLen == readSize)
   {   
      readSize = min64(totalSize - readLen, FS_CACHE_LEN);
      rc = _lread(newOffset, readSize, buf + readLen, &readOnceLen);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "fail to lread, offset=%d, size=%d, rc=%d", offset, size, rc);
         goto error;
      }

      readLen += readOnceLen;
      newOffset += FS_CACHE_LEN;
   }
   
   *len = readLen;
   ++_readCount;

done:
   PD_LOG(PDDEBUG, "flRead() end, offset=%d, size=%d, readLen=%d, flId=%d", offset, size, readLen, _flId);
   return rc;
   
error:
   _errCode = rc;
   goto done;
}

INT32 fileLob::fltruncate(INT64 offset)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;
   UINT32 hashKey = 0;
   CHAR *buf = NULL;
   INT64 size;
   INT64 newLobSize = 0;
   fsConnectionDao db(_dataSource);

   PD_LOG(PDDEBUG, "fltruncate(), offset=%d", offset);

   _fileMutex.get();
   if(_fileHead)
   {
      dataCache* cur = _fileHead;
      while(cur)
      {
         if(cur->_dirty && 0 == cur->_refCount)
         {
            hashKey = _mgr->getHashBucket()->hash(_flId, cur->_offset);
            _mgr->getHashBucket()->lockBucketR(hashKey);
            _addQuickQueue(hashKey, cur);
            //addQueue(hashKey, cur);
            _mgr->getHashBucket()->unLockBucketR(hashKey);
         }
         cur = cur->_fileNext;
      }
   }
   _fileMutex.release();

   _waitFileCacheUnused();

   if(_fileSize < offset)
   {
      size = offset - _fileSize;
      buf = (CHAR *)SDB_OSS_MALLOC(size);
      ossMemset(buf, '\0', size);

      rc = db.writeLob(buf, _fullCL, _oid, _fileSize, (UINT32)size, &newLobSize);
      SAFE_OSS_FREE(buf);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to write lob, offset=%d, rc=%d", offset, rc);
         goto error;
      }
      
      _fileSize = offset;
   }
   else if(_fileSize > offset)
   {
      rc = db.truncateLob(_fullCL, _oid, offset);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to truncate lob, offset=%d, rc=%d", offset, rc);
          goto error;
      }
      _fileSize = offset;
   }
   else
   {
      PD_LOG(PDDEBUG, "Lobsize is equal to offset, nothing to do");
      goto done;
   }

done:
   return rc;
   
error:
   goto done;   
}

INT32 fileLob::flFlush(INT64 *fileSize)
{
   INT32 rc = SDB_OK;
   INT32 hashKey = -1;
   fsConnectionDao db(_dataSource);
   SINT64 lobSize = 0;

   PD_LOG(PDDEBUG, "flFlush()");

   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   _fileMutex.get();
   if(_fileHead)
   {
      dataCache *cur = _fileHead;
      while(cur)
      {
         if(cur->_dirty && 0 == cur->_refCount)
         {
            hashKey = _mgr->getHashBucket()->hash(_flId, cur->_offset);
            _mgr->getHashBucket()->lockBucketR(hashKey);
            _addQuickQueue(hashKey, cur);
            //addQueue(hashKey, cur);
            _mgr->getHashBucket()->unLockBucketR(hashKey);
         }
         cur = cur->_fileNext;
      }
   }
   _fileMutex.release();

   if(FS_SYNC == _fflag)
   {
      _waitFileCacheUnused();
      //TODO:getlobsize
   }

   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   if(FS_SYNC == _fflag && _writeFlag)
   {
      rc = db.getLobSize(_fullCL, _oid, &lobSize);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to getLobSize, error=%d", rc);
          goto error;
      }
      _fileSize = lobSize;
      _writeFlag = FALSE;
   }

   *fileSize = _fileSize;
   
done:
   return rc;
   
error:
   goto done;    
}

INT32 fileLob::flClose(INT64 *fileSize)
{
   INT32 rc = SDB_OK;
   UINT32 hashKey = 0;
   fsConnectionDao db(_dataSource);
   SINT64 lobSize = 0;

   PD_LOG(PDDEBUG, "flClose()");

   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   _fileMutex.get();
   if(_fileHead)
   {
      dataCache* cur = _fileHead;
      while(cur)
      {
         //no dirty no reference
         if(cur->_dirty && 0 == cur->_refCount)
         {
            hashKey = _mgr->getHashBucket()->hash(_flId, cur->_offset);
            _mgr->getHashBucket()->lockBucketR(hashKey);
            _addQuickQueue(hashKey, cur);
            //addQueue(hashKey, cur);
            _mgr->getHashBucket()->unLockBucketR(hashKey);
         }
         cur = cur->_fileNext;
      }
   }
   _fileMutex.release();

   if(FS_SYNC == _fflag)
   {
      _waitFileCacheUnused();
      //TODO:getlobsize
   }

   _addrecycleQueue(_flId);

   if(_errCode != SDB_OK)
   {
      rc = _errCode;
      goto error;
   }

   if(FS_SYNC == _fflag && _writeFlag)
   {
      rc = db.getLobSize(_fullCL, _oid, &lobSize);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to getLobSize, error=%d", rc);
          goto error;
      }
      _fileSize = lobSize;
      _writeFlag = FALSE;
   }

   *fileSize = _fileSize;

   _mgr->setReadCount(_readCount, _downLoadCount);
   
done:
   return rc;
   
error:
   goto done;  
}

BOOLEAN fileLob::flClean()
{
   BOOLEAN isClean = TRUE;

   PD_LOG(PDDEBUG, "flClean()");
   
   if(_tryLockFileW())
   {
      if(_fileMutex.try_get())
      {
         if(_fileHead)
         {
            dataCache * cur = _fileHead;
            dataCache * next;
            while(cur)
            {
               next = cur->_fileNext;
               releaseCache(cur);
               cur = next;
            }
         }
         if(_fileHead != NULL)
         {
            isClean = FALSE; 
         }
         _fileMutex.release();
      }

      _unLockFileW();
   }
   else
   {
      if(NULL == _fileHead)
      {
         SDB_ASSERT(FALSE, "_fileHead is null , but file cache user is not 0");
         _fileCacheUserDecrease();
      }
      isClean = FALSE;
   }
   
   return isClean;
}

void fileLob::flUpdateSize(INT64 newSize)
{
   _fileSizeMutex.get();
   if(newSize > _fileSize)
   {
      _fileSize = newSize;
   }
   _fileSizeMutex.release();
}

void fileLob::addNode(dataCache* node)
{
   _fileMutex.get();
   if(_fileHead == NULL)
   {
      _fileHead = node;
   }
   else
   {
      node->_fileNext = _fileHead;
      _fileHead->_filePre = node;
      _fileHead = node;
   }
   _fileMutex.release();
}

void fileLob::delNode(dataCache* node, BOOLEAN needMutex)
{
   if(needMutex)
   {
      _fileMutex.get();
   }
   if(node == _fileHead)
   {
      if(NULL == node->_fileNext)
      {
         _fileHead = NULL;
      }
      else
      {
         _fileHead = node->_fileNext;
         _fileHead->_filePre = NULL;
      }
   }
   else
   {
      node->_filePre->_fileNext = node->_fileNext;
      if(node->_fileNext != NULL)
      {
         node->_fileNext->_filePre = node->_filePre;
      }
   }
   node->_filePre = NULL;
   node->_fileNext = NULL;
   if(needMutex)
   {
      _fileMutex.release();
   }
   SAFE_OSS_DELETE(node->_rwMutex);
}

BOOLEAN fileLob::tryDelNode(dataCache* node)
{
   if(_fileMutex.try_get())
   {
      if(node->tryLockW())
      {
         if(node == _fileHead)
         {
            if(NULL == node->_fileNext)
            {
               _fileHead = NULL;
            }
            else
            {
               _fileHead = node->_fileNext;
               _fileHead->_filePre = NULL;
            }
         }
         else
         {
            node->_filePre->_fileNext = node->_fileNext;
            if(node->_fileNext != NULL)
            {
               node->_fileNext->_filePre = node->_filePre;
            }
         }
         node->_filePre = NULL;
         node->_fileNext = NULL;
         node->unLockW();
         _fileMutex.release();
         return TRUE;
      }
      else
      {
         _fileMutex.release();
         return FALSE;
      }
   }
   else
   {
      return FALSE;
   }
}

void fileLob::releaseCache(dataCache* meta)
{
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();
   cacheLoader* loader = _mgr->getCacheLoader();
   
   UINT32 hashKey = hashBucket->hash(meta->_flId, meta->_offset);
   hashBucket->lockBucketW(hashKey);
   if(FALSE == meta->_dirty && 0 == meta->_refCount)
   {
      hashBucket->del(hashKey, meta);
      delNode(meta, FALSE);
      loader->releaseCache(meta);
   }
   hashBucket->unLockBucketW(hashKey);
}

INT32 fileLob::_lwrite(INT64 offset, INT64 size, const CHAR *buf)
{
   INT32 rc = SDB_OK;
   INT64 newOffset = offset >> FS_CACHE_OFFSETBIT << FS_CACHE_OFFSETBIT;
   UINT32 hashKey = _mgr->getHashBucket()->hash(_flId, newOffset);
   cacheLoader* loader = _mgr->getCacheLoader();
   INT64 newLobSize = 0;  //TODO：lob的总大小
   
   PD_LOG(PDDEBUG, "lwrite(), offset:%ld, size:%ld", offset, size);

   rc = _writeToCache(hashKey, offset, size, buf);
   if(SDB_OK == rc)
   {
      goto done;
   }
   
   rc = loader->loadEmpty(_flId, newOffset);
   if(SDB_OK == rc)
   {
      rc = _writeToCache(hashKey, offset, size, buf);
      if(SDB_OK == rc)
      {
         goto done;
      }
   }

   //前面写缓存失败，现在直接写lob
   rc = writeLobDirect(buf, size, offset, &newLobSize);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to writeLobDirect, offset:%ld, size:%d, rc=%d", offset, size, rc);
      goto error;
   }
   
done:
   return rc;
   
error:
   goto done;   
}

INT32 fileLob::_lread(INT64 offset, INT64 size, CHAR *buf, INT32 *len)
{
   INT32 rc = SDB_OK;
   INT32 readLen = 0;
   fsConnectionDao db(_dataSource);
   cacheLoader* loader = _mgr->getCacheLoader();
   INT64 newOffset = offset >> FS_CACHE_OFFSETBIT << FS_CACHE_OFFSETBIT;
   UINT32 hashKey  = _mgr->getHashBucket()->hash(_flId, newOffset);
   BOOLEAN download = false;

   PD_LOG(PDDEBUG, "lread(), offset:%ld, size:%ld", offset, size);

   rc = _readInCache(hashKey, offset, size, buf, len);
   if(SDB_OK == rc)
   {
      goto done;
   }
  
   download = true;
   rc = loader->load(hashKey, _flId, newOffset);
   if(SDB_OK == rc)
   {
      rc = _readInCache(hashKey, offset, size, buf, len);
      if(SDB_OK == rc)
      {
         goto done;
      }
   }  

   //从缓存读不到内容，直接读lob
   rc = readLobDirect(buf, size, offset, &readLen);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to readLobDirect, offset:%ld, size:%ld, rc=%d", offset, size, rc);
      _errCode = rc;
      goto error;
   }
   *len = readLen;

done:
   if(download)
   {
      ++_downLoadCount;
   }
   return rc;
   
error:
   goto done;   
}

INT32 fileLob::_readInCache(UINT32 hashKey, INT64 offset, INT64 size, CHAR *buf, INT32 *len)
{
   INT32 rc = SDB_OK;
   dataCache* node = NULL;
   INT64 newOffset = offset >> FS_CACHE_OFFSETBIT << FS_CACHE_OFFSETBIT;
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();

   hashBucket->lockBucketR(hashKey);
   node = hashBucket->get(hashKey, _flId, newOffset);
   if(node != NULL)
   {
      node->lockR();
      if(node->matchPiece(size, offset))
      {
         rc = node->cacheRead(buf, size, offset, len);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to read cache, offset:%ld, size:%ld. rc=%d", 
                            offset, size, rc);
            node->unLockR();
            goto error;
         }
      }
      else
      {
         node->unLockR();
         rc = SDB_DMS_RECORD_NOTEXIST;
         goto error;
      }

      node->unLockR();
   }
   else
   {
      rc = SDB_DMS_RECORD_NOTEXIST;
      goto error;
   }

done:
   hashBucket->unLockBucketR(hashKey);
   return rc;
   
error:
   goto done;  
}

INT32 fileLob::_writeToCache(UINT32 hashKey, INT64 offset, INT64 size, const CHAR *buf)
{
   INT32 rc = SDB_OK;
   dataCache* node = NULL;
   INT64 newOffset = offset >> FS_CACHE_OFFSETBIT << FS_CACHE_OFFSETBIT;
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();

   hashBucket->lockBucketR(hashKey);
   node = hashBucket->get(hashKey, _flId, newOffset);     
   if(node != NULL)
   {
      node->lockW();
      rc = node->cacheWrite(buf, size, offset); 
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to write cache, hashKey:%d, offset:%ld, size:%ld. rc=%d", 
                         hashKey, offset, size, rc);
         node->unLockW();
         goto error;
      }

      if (node->canFlush())
      {
         addQueue(hashKey, node);
      }

      node->unLockW();
   }
   else
   {
      rc = SDB_DMS_RECORD_NOTEXIST;
   }
   
done:
   hashBucket->unLockBucketR(hashKey);
   return rc;
   
error:
   goto done;  
}

void fileLob::addQueue(INT32 key, dataCache* node)
{
   PD_LOG(PDDEBUG, "addQueue(), key:%d, offset:%ld", key, node->_offset);
   node->_refCount++;
   //_rwlock.lock_r();
   _fileCacheUserIncrease(); 
   struct queueTask task(key, FS_TASK_FLUSH, node, -1, -1, FS_TASK_PRIORITY_0);
   if(!_mgr->getCacheQueue()->addTask(task))
   {
      node->_refCount--;
      //_rwlock.release_r();
      _fileCacheUserDecrease();
   }
}

void fileLob::_addQuickQueue(INT32 key, dataCache* node)
{
   PD_LOG(PDDEBUG, "addQuickQueue(), key:%d, offset:%ld", key, node->_offset);
   node->lockW();
   node->_refCount++;
   _fileCacheUserIncrease();
   struct queueTask task(key, FS_TASK_FLUSH, node, -1, -1, FS_TASK_PRIORITY_1);
   if(!_mgr->getCacheQueue()->addTask(task))
   {
      node->_refCount--;
      _fileCacheUserDecrease();
   }
   node->unLockW();
}

void fileLob::_addrecycleQueue(INT32 flId)
{
   _mgr->addRecycle(flId);
}

INT32 fileLob::writeLobDirect(const CHAR *buf,
                              INT64 size, 
                              INT64 offset,
                              INT64 *lobSizeNew)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "writeLobDirect(), offset:%ld, size:%ld", offset, size);

   fsConnectionDao db(_dataSource);
   rc = db.writeLob(buf, _fullCL, _oid, offset, size, lobSizeNew);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to writeLobDirect,  rc=%d", rc);
      _errCode = rc;
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fileLob::readLobDirect(CHAR *buf,
                             INT64 size, 
                             INT64 offset,
                             INT32 *len)
{
   INT32 rc = SDB_OK;
   INT32 readlen = 0;

   PD_LOG(PDDEBUG, "readLobDirect(), size:%ld, offset:%ld", size, offset);

   fsConnectionDao db(_dataSource);
   rc = db.readLob(_fullCL, _oid, offset, size, buf, &readlen);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to read file, cl=%s, offset:%ld, size:%ld, rc=%d",
             _fullCL, offset, size, rc);
      goto error;
   }
   
   *len = readlen;

done:
   PD_LOG(PDDEBUG, "readLobDirect() end, size:%ld, offset:%ld", size, offset);
   return rc;
   
error:
   goto done;
}

void fileLob::expiedCaches(INT64 offset, INT64 size)
{
   INT64 newOffset = 0;
   INT64 end = offset + size;
   UINT32 hashKey = 0;
   dataCache* node = NULL;
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();

   newOffset = offset >> FS_CACHE_OFFSETBIT << FS_CACHE_OFFSETBIT;

   while(newOffset < end)
   {
      hashKey = hashBucket->hash(_flId, newOffset);
      hashBucket->lockBucketR(hashKey);
      node = hashBucket->get(hashKey, _flId, newOffset);  
      if(node)
      {
         node->lockW();
         node->setExpired();
         node->unLockW();
      }
      hashBucket->unLockBucketR(hashKey);
      newOffset += FS_CACHE_LEN;
   }
}

void fileLob::_preRead(INT32 size, INT64 offset)
{
   UINT32 hashKey = 0;
   dataCache* node;
   INT64 newOffset = (((offset + size) >> FS_CACHE_OFFSETBIT) + 1) << FS_CACHE_OFFSETBIT;
   sequoiaFSHashBucket* hashBucket = _mgr->getHashBucket();
   INT16 maxreadBlock = _preReadBlock;
   cacheLoader* loader = _mgr->getCacheLoader();

   PD_LOG(PDDEBUG, "preRead(), offset:%ld, size:%ld", offset, size);

   while(newOffset <= _fileSize && maxreadBlock > 0)
   {
      maxreadBlock--;
      hashKey = hashBucket->hash(_flId, newOffset);
      if(hashBucket->tryLockBucketR(hashKey))
      {
         node = hashBucket->get(hashKey, _flId, newOffset);     
         if(NULL == node)
         {
            loader->preLoad(_flId, newOffset);
         }
         hashBucket->unLockBucketR(hashKey);
      }

      newOffset = newOffset + FS_CACHE_LEN;
   }
  
   return;
}

void fileLob::_waitFileCacheUnused()
{
   _rwlock.get();
   _rwlock.release();
}

void fileLob::_unLockFileW()
{
   _rwlock.release();
}

void fileLob::_fileCacheUserIncrease()
{
   _rwlock.get_shared();
}

void fileLob::_fileCacheUserDecrease()
{
   _rwlock.release_shared();
}

BOOLEAN fileLob::_tryLockFileW()
{
   if(_rwlock.try_get())
   {
      return TRUE;
   }
   else
   {
      return FALSE;
   }
}

INT32 fileLobDirect::flwrite(INT64 offset,
                             INT64 size,
                             const CHAR *buf)
{
   INT32 rc = SDB_OK;
   INT64 newLobSize = 0;

   PD_LOG(PDDEBUG, "flwrite(), size:%ld, offset:%ld", size, offset);

   rc = writeLobDirect(buf, size, offset, &newLobSize);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to writeLobDirect, rc=%d", rc);
      goto error;
   }

   expiedCaches(offset, size);

   // fileLob modify size
   flUpdateSize(newLobSize);

done:
   return rc;
   
error:
   goto done;
}


