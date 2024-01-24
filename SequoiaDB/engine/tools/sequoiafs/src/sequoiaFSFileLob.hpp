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

   Source File Name = sequoiaFSFileLob.hpp

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

#ifndef __SEQUOIAFSFILELOB_HPP__
#define __SEQUOIAFSFILELOB_HPP__

#include "ossLatch.hpp"
#include "ossRWMutex.hpp"
#include "utilBitmap.hpp"
#include "ossQueue.hpp"
#include "dms.hpp"

#include <boost/bind.hpp>
#include <boost/thread/thread.hpp>

#include "sequoiaFSDataCache.hpp"
#include "sdbConnectionPool.hpp"

using namespace engine;

using namespace sdbclient;
using namespace bson;

#define FS_SYNC   0
#define FS_ASYNC  1
#define FS_DIRECT 2

namespace sequoiafs
{      
   class sequoiaFSFileLobMgr;
   class fileLob : public SDBObject
   {
      public:  
         fileLob(){}
         virtual ~fileLob(){}
         void init(INT32 flId, 
                   sdbConnectionPool* ds, 
                   sequoiaFSFileLobMgr* mgr, 
                   INT32 preReadBlock);
         INT32 flOpen(const CHAR *clFullName, 
                      const OID &oid, INT32 fflag, 
                      INT64 size);
         virtual INT32 flwrite(INT64 offset,
                               INT64 size,
                               const CHAR *buf);
         INT32 flRead(INT64 offset, 
                      INT64 size, 
                      CHAR *buf, 
                      INT32 *len);
         INT32 fltruncate(INT64 offset);
         INT32 flFlush(INT64 *fileSize);
         INT32 flClose(INT64 *fileSize);
         BOOLEAN flClean();
         void flUpdateSize(INT64 newSize);
         
         SINT64 flGetFileSize(){return _fileSize;}
         UINT32 flGetFlId(){return _flId;}

         void addNode(dataCache* node);
         void delNode(dataCache* node, BOOLEAN needMutex = TRUE);
         BOOLEAN tryDelNode(dataCache* node);

         void releaseCache(dataCache* node);
         
         void addQueue(INT32 key, dataCache* node);

         INT32 writeLobDirect(const CHAR *buf,
                              INT64 size, 
                              INT64 offset,
                              INT64 *lobSizeNew);
         INT32 readLobDirect(CHAR *buf,
                             INT64 size, 
                             INT64 offset,
                             INT32 *len);

         void expiedCaches(INT64 offset, INT64 size);

         CHAR* getFullCL(){return _fullCL;}
         OID getOid(){return _oid;}
         void releaseShardRWLock(){_rwlock.release_shared();} 
         void setErrorCode(INT32 errCode){_errCode = errCode;}

      private:
         void _preRead(INT32 size, INT64 offset);
         INT32 _lwrite(INT64 offset, INT64 size, const CHAR *buf);
         INT32 _lread(INT64 offset, INT64 size, CHAR *buf, INT32 *len);
         INT32 _readInCache(UINT32 hashKey, INT64 offset, INT64 size, CHAR *buf, INT32 *len);
         INT32 _writeToCache(UINT32 hashKey, INT64 offset, INT64 size, const CHAR *buf);

         void _addQuickQueue(INT32 key, dataCache* node);
         void _addrecycleQueue(INT32 flId);

         BOOLEAN _tryLockFileW();
         void _unLockFileW();

         void _waitFileCacheUnused(); 
         void _fileCacheUserIncrease();
         void _fileCacheUserDecrease();
         
   
      private:
         UINT32   _flId;
         INT32    _errCode;
         OID      _oid;
         SINT64   _fileSize;
         ossSpinXLatch _fileSizeMutex;   //保护_fileHead
         INT32    _fflag;  //SYNC:0, ASYNC :1,  DIRECT:2
         INT32    _preReadBlock;
         BOOLEAN  _writeFlag;
         CHAR     _fullCL[DMS_COLLECTION_FULL_NAME_SZ + 1]; 
         
         ossSpinSLatch _rwlock;  //判断是否还有缓存块未刷新
         
         ossSpinXLatch _fileMutex;   //保护_fileHead 
         dataCache*    _fileHead;
         
         sdbConnectionPool*      _dataSource;
         sequoiaFSFileLobMgr* _mgr;

         INT32 _readCount;
         INT32 _downLoadCount;
   };

   class fileLobDirect : public fileLob
   {
      public:
         INT32 flwrite(INT64 offset, 
                       INT64 size, 
                       const CHAR *buf);
   };
}

#endif

