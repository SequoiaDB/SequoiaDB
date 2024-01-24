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

   Source File Name = sequoiaFSFileLobMgr.hpp

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

#ifndef __SEQUOIAFSFILELOBMGR_HPP__
#define __SEQUOIAFSFILELOBMGR_HPP__

#include "ossLatch.hpp"
#include "dms.hpp"
#include "utilBitmap.hpp"
#include "ossQueue.hpp"

#include "utilCircularQueue.hpp"

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "sequoiaFSFileLob.hpp"
#include "sequoiaFSHashBucket.hpp"
#include "sequoiaFSCacheQueue.hpp"
#include "sdbConnectionPool.hpp"
#include "sequoiaFSCacheLoader.hpp"

#define FS_QUEUE_THREAD_NUMBER 20
#define FS_FILELOBS_NUMBER 8192

using namespace sdbclient;
using namespace engine;

namespace sequoiafs
{      
   class sequoiaFSFileLobMgr: public SDBObject
   {
      public:
         sequoiaFSFileLobMgr();
         ~sequoiaFSFileLobMgr();
         void fini();
         INT32 init(sdbConnectionPool* ds, 
                    INT32 fflag, 
                    INT32 preReadBlock, 
                    INT32 capacity);
         fileLob* allocFreeFileLob();
         fileLob* getFileLob(UINT32 flId);
         
         sequoiaFSHashBucket* getHashBucket(){return &_hashBucket;}
         fsCacheQueue* getCacheQueue(){return &_fsCacheQueue;}
         cacheLoader* getCacheLoader(){return &_cacheLoader;}
         sdbConnectionPool* getDataSouce(){return _dataSource;}
         void addRecycle(INT32 flId);
         BOOLEAN getRunning(){return _running;}

         INT32 getUsedFlIdCount(){return _usedCount;}
         void setReadCount(INT32 read, INT32 download);
         INT32 getReadCount(){return _readCount;}
         INT32 getDownloadCount(){return _downLoadCount;}
         void cleanReadCount();
         
      private:
         INT32 _getFreeId();
         void _recycleflId();
         void _releaseId(INT32 flId);
         void _recycleCache();

      private:
         _utilStackBitmap<FS_FILELOBS_NUMBER> _fileLobbitmap;
         UINT32 _fromPos;
         ossSpinXLatch _mutex;
         fileLob* _fileLobs[FS_FILELOBS_NUMBER];
         INT32 _usedCount;
         
         sdbConnectionPool* _dataSource;
         fsCacheQueue _fsCacheQueue;
         sequoiaFSHashBucket _hashBucket;
         cacheLoader _cacheLoader;

         boost::thread* _queueThreads[FS_QUEUE_THREAD_NUMBER];
         boost::thread *_thflId;
         boost::thread *_thCache;
         boost::thread *_thOverTime;

         typedef _utilCircularBuffer<INT32>       QUEUE_BUFFER ;
         typedef _utilCircularQueue<INT32>         QUEUE_CONTAINER ;
         typedef ossQueue< INT32, QUEUE_CONTAINER > FLID_QUEUE ;

         FLID_QUEUE             *_recycleQueue ;
         FLID_QUEUE             *_bakQueue ;
         QUEUE_BUFFER           _recycleQueueBuffer ;
         QUEUE_BUFFER           _bakQueueBuffer ;
      
         BOOLEAN _running;

         INT32 _readCount;
         INT32 _downLoadCount;
         ossSpinXLatch _countMutex;
   };
}

#endif

