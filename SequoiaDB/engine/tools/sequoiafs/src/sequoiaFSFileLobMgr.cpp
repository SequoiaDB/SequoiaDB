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

   Source File Name = sequoiaFSFileLobMgr.cpp

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

#include "sequoiaFSFileLobMgr.hpp"
#include "sequoiaFSFileLob.hpp"

#include <time.h>
#include "ossUtil.h"
#include "utilMemBlockPool.hpp"

using namespace sequoiafs;
using namespace sdbclient;
using namespace engine;

sequoiaFSFileLobMgr::sequoiaFSFileLobMgr()
:_fromPos(0),
 _fsCacheQueue(this),
 _hashBucket(this),
 _cacheLoader(this)
{
   ossMemset(&(_fileLobs), 0, 4*FS_FILELOBS_NUMBER);
   ossMemset(&(_queueThreads), 0, 4*FS_QUEUE_THREAD_NUMBER);
   _thflId = NULL;
   _thOverTime = NULL;
}

sequoiaFSFileLobMgr::~sequoiaFSFileLobMgr()
{
   fini();
}

void sequoiaFSFileLobMgr::fini()
{
   _running = FALSE;

   for(INT32 i = 0; i < FS_QUEUE_THREAD_NUMBER; i++)  
   {
      if(_queueThreads[i] != NULL)
      {
         _queueThreads[i]->join();
      }
   }

   if(_thflId)
   {
      _thflId->join();
   }
   if(_thCache)
   {
      _thCache->join();
   }
   if(_thOverTime)
   {
      _thOverTime->join();
   }
   _dataSource = NULL;

   SAFE_OSS_DELETE(_recycleQueue);
   SAFE_OSS_DELETE(_bakQueue);
   _recycleQueueBuffer.finiBuffer() ;
   _bakQueueBuffer.finiBuffer() ;

   for(INT32 i = 0; i < FS_FILELOBS_NUMBER; i++)
   {
      if(_fileLobs[i] != NULL)
      {
         SAFE_OSS_DELETE(_fileLobs[i]);
         _fileLobs[i] = NULL;
      }
   }
}

INT32 sequoiaFSFileLobMgr::init(sdbConnectionPool* ds, 
                                INT32 fflag, 
                                INT32 preReadBlock,
                                INT32 capacity)
{
   INT32 rc = SDB_OK;
   _dataSource = ds;
   _running = TRUE;

   if(FS_DIRECT == fflag)
   {
      for(INT32 i = 0; i < FS_FILELOBS_NUMBER; i++)  
      {
         fileLob* fl = new fileLobDirect();
         if(NULL == fl)
         {
            PD_LOG(PDERROR, "new fileLobDirect failed.");
            rc = SDB_OOM ; 
            goto error ;
         }
         fl->init(i, _dataSource, this, preReadBlock);
         _fileLobs[i] = fl;
      }
   }
   else
   {
      for(INT32 i = 0; i < FS_FILELOBS_NUMBER; i++)
      {
         fileLob* fl =  new fileLob(); 
         if(NULL == fl)
         {
            PD_LOG(PDERROR, "new fileLob failed.");
            rc = SDB_OOM;
            goto error;
         }
         fl->init(i, _dataSource, this, preReadBlock);
         _fileLobs[i] = fl;
      }
   }

   if(! _recycleQueueBuffer.initBuffer( FS_FILELOBS_NUMBER ))
   {
      PD_LOG(PDERROR, "Failed to allocate queue buffer with flId number:%d.", FS_FILELOBS_NUMBER);
      rc = SDB_OOM;
      goto error;
   }

   if(! _bakQueueBuffer.initBuffer( FS_FILELOBS_NUMBER ))
   {
      PD_LOG(PDERROR, "Failed to allocate queue buffer with flId number:%d.", FS_FILELOBS_NUMBER);
      rc = SDB_OOM;
      goto error;
   }

   try
   {
      _recycleQueue = new FLID_QUEUE( QUEUE_CONTAINER(&_recycleQueueBuffer));
   }
   catch(std::exception &e)
   {
      PD_LOG(PDERROR, "bad_alloc _recycleQueue, error=%s", e.what());
      rc = SDB_OOM;
      goto error;
   }

   try
   {
      _bakQueue = new FLID_QUEUE( QUEUE_CONTAINER(&_bakQueueBuffer));
   }
   catch(std::exception &e)
   {
      PD_LOG(PDERROR, "bad_alloc _bakQueue, error=%s", e.what());
      rc = SDB_OOM;
      goto error;
   }
   
   for(INT32 i = 0; i < FS_QUEUE_THREAD_NUMBER; i++)  
   {
      try
      {
         boost::thread *th = new boost::thread( boost::bind( &fsCacheQueue::run, &_fsCacheQueue) );
         _queueThreads[i] = th; 
      }
      catch(std::bad_alloc &e1)
      {
         PD_LOG(PDERROR, "bad_alloc thread, error=%s", e1.what());
         rc = SDB_OOM;
         goto error;
      }
      catch ( boost::thread_resource_error &e2)
      {
         PD_LOG(PDERROR, "Failed to new thread, error=%s", e2.what());
         rc = SDB_SYS; 
         goto error;
      }
      catch(std::exception &e3)
      {
         PD_LOG(PDERROR, "Failed to new thread, error=%s", e3.what());
         rc = SDB_SYS; 
         goto error;
      }
   }

   //start the thread for recycle flId
   try
   {
      _thflId = new boost::thread( boost::bind( &sequoiaFSFileLobMgr::_recycleflId, this ) );
   }
   catch (std::exception &e4)
   {
      PD_LOG(PDERROR, "Failed to new thread, error=%s", e4.what());
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      _thOverTime = new boost::thread( boost::bind( &sequoiaFSHashBucket::cleanOverTimeCache, &_hashBucket ) );
   }
   catch (std::exception &e5)
   {
      PD_LOG(PDERROR, "Failed to new thread, error=%s", e5.what());
      rc = SDB_SYS ;
      goto error ;
   }

   try
   {
      _thCache = new boost::thread( boost::bind( &sequoiaFSFileLobMgr::_recycleCache, this ) );
   }
   catch (std::exception &e6)
   {
      PD_LOG(PDERROR, "Failed to new thread, error=%s", e6.what());
      rc = SDB_SYS ;
      goto error ;
   }

   _cacheLoader.init(capacity);

done:
   return rc;
   
error:
   goto done;   
}

INT32 sequoiaFSFileLobMgr::_getFreeId()   
{
   //遍历_fileLobbitmap，find a free id
   INT32 nextPos = 0;

   _mutex.get();
   nextPos = _fileLobbitmap.nextFreeBitPos(_fromPos); 
   if(-1 == nextPos)
   {
      nextPos = _fileLobbitmap.nextFreeBitPos(0);
   }
   if(nextPos >= 0)
   {
      _fileLobbitmap.setBit(nextPos);
      _fromPos = nextPos;
      ++_usedCount;
   }

   _mutex.release();
   return nextPos;
}

void sequoiaFSFileLobMgr::_releaseId(INT32 flId)
{
   _mutex.get();
   _fileLobbitmap.clearBit(flId);
   --_usedCount;
   _mutex.release();
}

fileLob* sequoiaFSFileLobMgr::allocFreeFileLob()  
{
   INT32 flId = _getFreeId();
   if(-1 == flId)
   {
      return NULL;
   }
   return _fileLobs[flId];
}

fileLob* sequoiaFSFileLobMgr::getFileLob(UINT32 flId)
{
   if(_fileLobbitmap.testBit(flId))
   {
      return _fileLobs[flId];
   }
   else
   {
      return NULL;
   }
}

void sequoiaFSFileLobMgr::addRecycle(INT32 flId)
{
   _recycleQueue->push(flId);   
} 

void sequoiaFSFileLobMgr::_recycleflId()
{
   while(_running)
   {
      INT32 flId = 0;
      if(! _recycleQueue->timed_wait_and_pop(flId, OSS_ONE_SEC)) 
      {
         while(_bakQueue->try_pop(flId))
         {
            _recycleQueue->push(flId);
         }
         continue;
      }
      
      fileLob *file = getFileLob(flId);
      if(NULL == file)
      {
         continue;
      }

      if(file->flClean())
      {
         _releaseId(flId);
      }
      else
      {
         _bakQueue->push(flId);
      }
      
   }
}

void sequoiaFSFileLobMgr::_recycleCache()
{
   while(_running)
   {
      utilGetGlobalMemPool()->shrink() ;
      ossSleepsecs(1);
   }
}

void sequoiaFSFileLobMgr::setReadCount(INT32 read, INT32 download)
{
   _readCount += read;
   _downLoadCount += download;
}

void sequoiaFSFileLobMgr::cleanReadCount()
{
   _countMutex.get();
   _readCount = 0;
   _downLoadCount = 0;
   _countMutex.release();
}



