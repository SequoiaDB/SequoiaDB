/*******************************************************************************


   Copyright (C) 2023-present SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Affero General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   Source File Name = sequoiaFSFileCreatingMgr.hpp

   Descriptive Name = sequoiafs fuse file create file manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/17/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSFILECREATINGMGR_HPP__
#define __SEQUOIAFSFILECREATINGMGR_HPP__

#include "sequoiaFSFileCreatingMgr.hpp"
#include "sequoiaFSFileCreateHashBucket.hpp"
#include "sdbConnectionPool.hpp"
#include "sequoiaFSDao.hpp"

#include "ossQueue.hpp"
#include "ossLatch.hpp"

#include "ossAtomic.hpp"

using namespace sdbclient;
using namespace engine;

#define CREATE_STATUS_CACHEMETA   0    //file meta in cache
#define CREATE_STATUS_CACHEDATA   1    //file meta and data in cache
#define CREATE_STATUS_INDB        2    //file meta and data has uploaded to db, no cache
#define CREATE_STATUS_INDISK      3    //file meta and data in cache and disk

#define CONFLICT_FILE_PATH  "conflict"  //store conflict file
#define WORK_FILE_PATH  "work"  //store tmp file

#define UPLOAD_QUEUE_THREAD_NUMBER 20

#define CACHE_META_SIZE           8192     //unit:8K
#define CACHE_DATA_SIZE           131072   //unit:128k

namespace sequoiafs
{
   class fileItem : public SDBObject
   {
      public:
         INT64 _parentId;
         CHAR  _fileName[FS_MAX_NAMESIZE + 1];
         fileItem* _pre;
         fileItem* _next;

      public:
         fileItem(INT64 parentId, CHAR* filename)
         {
            _parentId = parentId;
            ossStrncpy(_fileName, filename, FS_MAX_NAMESIZE);

            int len = ossStrlen(filename) < FS_MAX_NAMESIZE ?
                     ossStrlen(filename) : FS_MAX_NAMESIZE;
            _fileName[len] = '\0';
            
            _pre = NULL;
            _next = NULL;
         }

         fileItem()
         {
         }
   };

   class sequoiaFS;
   class fileCreatingMgr : public SDBObject
   {
      public:
         fileCreatingMgr();
         ~fileCreatingMgr();
         INT32 init(sequoiaFS* fs, 
                    sdbConnectionPool* ds, 
                    const CHAR* clFullName,
                    const CHAR* fileCLName,
                    const BOOLEAN useCache,
                    const CHAR* filePath, 
                    INT32 cacheSize);
         void fini();

         INT32 create(INT64 parentId, 
                      CHAR*  fileName, 
                      const CHAR* path, 
                      UINT32 mode, 
                      UINT32 uid, 
                      UINT32 gid,
                      lobHandle* handle, 
                      BOOLEAN &isProcessed);
         INT32 write(INT64 parentId, 
                     CHAR* fileName, 
                     const CHAR *buf, 
                     size_t size, 
                     off_t offset,
                     lobHandle* handle, 
                     BOOLEAN &isProcessed);
         void flush(INT64 parentId, 
                    CHAR* fileName, 
                    lobHandle* handle, 
                    BOOLEAN &isProcessed);            
         INT32 release(INT64 parentId, 
                       CHAR* fileName, 
                       lobHandle* handle, 
                       BOOLEAN &isProcessed);
         INT32 query(INT64 parentId, CHAR* fileName, fileMeta &file); 
         BOOLEAN isDirEmpty(INT64 parentId);
         INT32 flushfile(INT64 parentId, CHAR* fileName);
         INT32 flushDir(INT64 parentId);
         INT32 unlink(INT64 parentId, CHAR* fileName, BOOLEAN &isProcessed);
         INT32 addTask(INT64 parentId, CHAR* fileName);

         INT32 writeTmpFile(createFileNode* fileNode);

         INT64 getUsedCount(){return _fileCount.fetch();}
         UINT32 getUploadQueueSize(){return _uploadQueue.size();}
         fileCreateHashBucket* getCreateBucket(){return &_createBucket;}

      private:
         INT32 _checkAndUploadTmpFiles();
         INT32 _checkAndMkDir(string dirName);
         BOOLEAN _addFileToDirMap(INT64 parentId, CHAR* fileName);
         INT32 _getFilesFromDirMap(INT64 parentId, multimap<int, string> &dirFiles);
         void _mapRemoveFile(INT64 parentId, const CHAR* fileName);
         fileItem* _removeFileFromDirMap(INT64 parentId, const CHAR* fileName);
         INT32 _uploadCachetoDB(INT64 parentId, const CHAR* fileName);
         void _uploadTask();
         void _tryAddQueue(INT64 parentId, CHAR* fileName);
         INT32 _uploadFileToDB(CHAR* buf, const CHAR* filePath);
         INT32 _removeTmpFile(INT64 parentId, const CHAR* fileName);
         INT32 _moveTmpFileToBak(const CHAR* tmpFileName);
         INT32 _insertMeta(fsConnectionDao &db, fileMeta* fileNode);
         
      private:
         fileCreateHashBucket             _createBucket;
         ossPoolMap<int, fileItem*>       _parentIdMap;
         ossSpinXLatch                    _parentIdMapMutex;

         ossQueue<fileItem> _uploadQueue;
         boost::thread* _uploadQueueThreads[UPLOAD_QUEUE_THREAD_NUMBER];
         
         ossAtomicSigned64 _fileCount;
         ossSpinXLatch _fileCountMutex;
         INT64 _fileMaxSize;
         BOOLEAN _running;

         sequoiaFS* _fs;

         sdbConnectionPool* _ds;
         string _clFullName;
         string _fileCLName;
         BOOLEAN _useCache;
         string _filePath;
         string _fileWorkPath;
         string _fileConflictPath;
   };
}

#endif

