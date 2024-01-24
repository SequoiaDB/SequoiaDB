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

   Source File Name = sequoiaFSFileCreatingMgr.cpp

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

#include "sequoiaFSFileCreatingMgr.hpp"
#include "sequoiaFS.hpp"
#include "ossFile.hpp"
#include "ossPath.hpp"

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

using namespace sequoiafs;
using namespace sdbclient;
using namespace engine;

namespace fs = boost::filesystem ;

fileCreatingMgr::fileCreatingMgr()
:_createBucket(this),
 _fileCount( 0 )
{
}

fileCreatingMgr::~fileCreatingMgr()
{
}

INT32 fileCreatingMgr::init(sequoiaFS* fs, 
                            sdbConnectionPool* ds, 
                            const CHAR* clFullName, 
                            const CHAR* fileCLName, 
                            BOOLEAN useCache, 
                            const CHAR* filePath, 
                            INT32 cacheSize)
{
   INT32 rc = SDB_OK;
   
   _fs = fs;
   _ds = ds;
   _clFullName = clFullName;
   _fileCLName = fileCLName;
   _useCache = useCache;
   _filePath = filePath;
   _fileWorkPath = filePath;
   //8k meta + 128k data, cachesize uint:MB
   _fileMaxSize = (UINT64)cacheSize * 1024 * 1024 / (CACHE_META_SIZE + CACHE_DATA_SIZE);  

   //check and upload tmp files
   rc = _checkAndUploadTmpFiles();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "_checkAndUploadTmpFiles failed. rc=%d", rc);
      goto error;
   }

   _running = TRUE;

   if(_useCache)
   {
      //start upload threads
      for(INT32 i = 0; i < UPLOAD_QUEUE_THREAD_NUMBER; i++) 
      {
         try
         {
            boost::thread *th = new boost::thread( boost::bind( &fileCreatingMgr::_uploadTask, this) );
            _uploadQueueThreads[i] = th; 
         }
         catch(std::exception &e1)
         {
            PD_LOG(PDERROR, "Failed to new thread. error=%s", e1.what());
            ossPrintf("Failed to new thread. error=%s" OSS_NEWLINE, e1.what());
            rc = SDB_SYS; 
            goto error;
         }
      }
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fileCreatingMgr::_checkAndUploadTmpFiles()
{
   INT32 rc = SDB_OK;
   multimap<string, string>  mapFiles;
   multimap<string, string>::iterator it;
   CHAR filter[10] =  "*";
   ossFile createFile;
   CHAR* tmpFileBuf = NULL;

   if(_useCache)
   {
      //check filepath 
      if(!_filePath.empty())
      { 
         rc = _checkAndMkDir(_filePath);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "_makePDir failed, _filePath:%s. rc=%d", 
                             _filePath.c_str(), rc);
            goto error;
         }

         //makedir conflict dir
         if(!utilStrEndsWith(_filePath, OSS_FILE_SEP))
         {
            _filePath = _filePath + OSS_FILE_SEP ;
         }

         _fileWorkPath = _filePath + WORK_FILE_PATH + OSS_FILE_SEP;
         rc = _checkAndMkDir(_fileWorkPath);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "check and mkdir(%s) failed. rc=%d", 
                             _fileConflictPath.c_str(), rc);
            goto error;
         }

         _fileConflictPath = _filePath + CONFLICT_FILE_PATH + OSS_FILE_SEP;
         rc = _checkAndMkDir(_fileConflictPath);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "check and mkdir(%s) failed. rc=%d", 
                             _fileConflictPath.c_str(), rc);
            goto error;
         }

         //upload tmp files
         rc = ossEnumFiles(_fileWorkPath, mapFiles, filter);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "get files failed, _filePath:%s. rc=%d", 
                             _fileWorkPath.c_str(), rc);
            ossPrintf("get files failed, _filePath:%s. rc=%d" OSS_NEWLINE, 
                       _fileWorkPath.c_str(), rc);
            goto error;
         }
         
         tmpFileBuf = (CHAR*) SDB_OSS_MALLOC(CACHE_META_SIZE + CACHE_DATA_SIZE);
         if(NULL == tmpFileBuf)
         {
            rc = SDB_OOM ;
            PD_LOG(PDERROR, "buf malloc failed");
            ossPrintf("malloc failed, size:%d. rc=%d" OSS_NEWLINE, 
                       CACHE_META_SIZE + CACHE_DATA_SIZE, rc);
            goto error;
         }
         
         for(it = mapFiles.begin(); it != mapFiles.end(); it++)
         {
            rc = _uploadFileToDB(tmpFileBuf, it->second.c_str());
            if(SDB_OK != rc)
            {
               if(SDB_IXM_DUP_KEY == rc)
               {
                  rc = _moveTmpFileToBak(it->first.c_str());
                  if(SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "Move conflict file failed, filename:%s. rc=%d", 
                                      it->second.c_str(), rc);
                     ossPrintf("Move conflict file failed, filename:%s. rc=%d" OSS_NEWLINE,
                                it->second.c_str(), rc);
                     goto error;
                  }
               }
               else
               {
                  PD_LOG(PDERROR, "upload tmp file to db failed, filename:%s. rc=%d", 
                                   it->second.c_str(), rc);
                  ossPrintf("upload tmp file to db failed, filename:%s. rc=%d" OSS_NEWLINE, 
                             it->second.c_str(), rc);
                  goto error;
               }
            }
         }
      }
      else 
      {
         //goto error;
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "filecreatecachepath must be specified, rc=%d", rc);
         ossPrintf("filecreatecachepath must be specified. rc=%d" OSS_NEWLINE, rc);
         goto error;
      }
   }
   else 
   {
      //check filepath is empty
      if(!_filePath.empty())
      {
         rc = ossEnumFiles( _filePath, mapFiles, filter);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "get files failed. path(%s). rc=%d", 
                             _filePath.c_str(), rc);
            ossPrintf("get files failed. path(%s). rc=%d" OSS_NEWLINE,
                       _filePath.c_str(), rc);
            goto error;
         }

         if (!mapFiles.empty())
         {
            rc = SDB_INVALIDARG;
            PD_LOG(PDERROR, "Path(%s) is not empty. rc=%d", _filePath.c_str(), rc);
            ossPrintf("Path[%s] is not empty. rc=%d" OSS_NEWLINE,
                       _filePath.c_str(), rc);
            goto error;
         }
      }
   }

done:
   SAFE_OSS_FREE(tmpFileBuf);
   return rc;
error:
   goto done;
}

INT32 fileCreatingMgr::_checkAndMkDir(string dirName)
{
   INT32 rc = SDB_OK;
   fs::path dir(dirName);
   
   rc = ossAccess(dirName.c_str(), OSS_MODE_ACCESS | OSS_MODE_READWRITE);
   if(rc != SDB_OK)
   {
      if(SDB_FNE == rc)
      {
         rc = ossMkdir(dirName.c_str());
         if(rc != SDB_OK)
         {
            PD_LOG( PDERROR, "Make filePath[%s] failed. rc=%d", 
                              dirName.c_str(), rc);
            ossPrintf("Make filePath[%s] failed. rc=%d" OSS_NEWLINE,
                       dirName.c_str(), rc);
            goto error;
         }
      }
      else 
      {
         PD_LOG( PDERROR, "Access filePath[%s] failed. rc=%d", dirName.c_str(), rc);
         ossPrintf("Access filePath[%s] failed. rc=%d" OSS_NEWLINE,
                    dirName.c_str(), rc);
         goto error;
      }
   }

   if(!( fs::is_directory ( dir ) )) 
   {
      rc = SDB_INVALIDARG;
      PD_LOG( PDERROR, "Path[%s] is not a directory. rc=%d", dirName.c_str(), rc);
      ossPrintf("Path[%s] is not a directory. rc=%d" OSS_NEWLINE,
                 dirName.c_str(), rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;   
}

void fileCreatingMgr::fini()
{
   _createBucket.waitBucketClean();
   _running = FALSE;

   for(INT32 i = 0; i < UPLOAD_QUEUE_THREAD_NUMBER; i++)
   {
      if(_uploadQueueThreads[i] != NULL)
      {
         _uploadQueueThreads[i]->join();
      }
   }
}

INT32 fileCreatingMgr::create(INT64 parentId, 
                              CHAR* fileName, 
                              const CHAR* path, 
                              UINT32 mode, 
                              UINT32 uid, 
                              UINT32 gid, 
                              lobHandle* handle, 
                              BOOLEAN &isProcessed)
{
   INT32 rc = SDB_OK;
   fileMeta meta;
   UINT64 ctime = 0;
   createFileNode* fileNode = NULL; 
   INT32 hashKey = 0;
   INT64 fileCount = 0;
   BOOLEAN isInc = FALSE;

   isProcessed = FALSE;
   
   //check handle
   if(NULL == handle)
   {
      rc = SDB_OOM;
      SDB_ASSERT(FALSE, "handle is null.");
      goto error;
   }

   if(!_useCache)
   {
      goto done;
   }

   PD_LOG(PDDEBUG, "fileCreatingMgr create, parentId=%d, fileName=%s, path:%s", 
                    parentId, fileName, path);

   fileCount = _fileCount.inc();
   isInc = TRUE;
   if(fileCount >= _fileMaxSize)
   {
      PD_LOG(PDWARNING, "fileCreatingMgr is busy. _fileCount=%ld, _fileMaxSize=%d",
                         fileCount, _fileMaxSize);
      goto done;
   }

   if(!_parentIdMapMutex.try_get())
   {
      PD_LOG(PDWARNING, "fileCreatingMgr parenIdMap is busy. _fileCount=%ld", 
                         fileCount);
      goto done;
   }
   
   rc = _addFileToDirMap(parentId, fileName);
   _parentIdMapMutex.release();
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "add to map failed. parentId=%d, fileName=%s", 
                       parentId, fileName);
      goto error;
   }

   hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketW(hashKey);
   fileNode = _createBucket.get(hashKey, parentId, fileName);
   if(fileNode != NULL)
   {
      PD_LOG(PDERROR, "create file failed, file exists. parentId=%d, fileName=%s", 
                       parentId, fileName);
      _createBucket.unLockBucketW(hashKey);
      _mapRemoveFile(parentId, fileName);
      rc = SDB_FE;
      goto error;
   }

   //init a node
   ctime = ossGetCurrentMilliseconds();
   meta.setName(fileName);
   meta.setMode(S_IFREG | mode);
   meta.setNLink(1);
   meta.setPid(parentId);
   meta.setCtime(ctime);
   meta.setMtime(ctime);
   meta.setAtime(ctime);
   meta.setUid(uid);
   meta.setGid(gid);
   meta.setSize(0);
   meta.setSymLink(path);
   
   fileNode = SDB_OSS_NEW createFileNode(hashKey, &meta);
   if(NULL == fileNode)
   {
      rc = SDB_OOM;
      PD_LOG(PDERROR, "new failed. parentId=%d, fileName=%s", 
                       parentId, fileName);
      _createBucket.unLockBucketW(hashKey);
      _mapRemoveFile(parentId, fileName);
      goto error;
      
   }
   fileNode->handle = handle;
   
   _createBucket.add(hashKey, fileNode);
   
   handle->status = CREATE_STATUS_CACHEMETA;
   isProcessed = TRUE;
   _createBucket.unLockBucketW(hashKey);

done:
   if(isInc && !isProcessed)
   {
      _fileCount.dec();
   }
   return rc;

error:
   goto done;
}

INT32 fileCreatingMgr::write(INT64 parentId, CHAR* fileName, const CHAR *buf, 
                             size_t size, off_t offset, lobHandle* handle,
                             BOOLEAN &isProcessed)
{
   INT32 rc = SDB_OK;
   BOOLEAN toFlush = FALSE;
   createFileNode* node = NULL;
   INT32 hashKey;
   CHAR* data = NULL;

   PD_LOG(PDDEBUG, "fileCreatingMgr write, "
                   "parentId=%d, fileName=%s, offset:%d, size:%d", 
                    parentId, fileName, offset, size);

   isProcessed = FALSE;

   if(NULL == handle)
   {
      goto done;
   }

   if(!(handle->isCreate))
   {
      goto done;
   }

   hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketW(hashKey);
   node = _createBucket.get(hashKey, parentId, fileName);
   if(node != NULL)
   {
      if(CREATE_STATUS_CACHEMETA == handle->status)
      {
         if(0 == offset && size < CACHE_DATA_SIZE)
         {
            data =  (CHAR*)SDB_THREAD_ALLOC(size);
            if(data == NULL)
            {
               _createBucket.unLockBucketW(hashKey);
               rc = SDB_OOM;
               goto error;
            }
            ossMemcpy(data, buf, size);
      
            node->data = data;
            node->meta.setSize(size);
            handle->status = CREATE_STATUS_CACHEDATA;
            isProcessed = TRUE;
         }
         else 
         {
            toFlush = TRUE;
         }
      }
      else if(CREATE_STATUS_CACHEDATA == handle->status)
      {
         toFlush = TRUE;
      }
      else 
      {
         SDB_ASSERT(FALSE, "status is invalid");
      }
   }
   _createBucket.unLockBucketW(hashKey);

   if(toFlush)
   {
      rc = _uploadCachetoDB(parentId, fileName);
      if(rc != SDB_OK)  
      {
         PD_LOG(PDERROR, "_upload cache to DB failed."
                         "parentId=%d, fileName=%s. rc=%d", 
                          parentId, fileName, rc);
         goto error;
      }
      _mapRemoveFile(parentId, fileName);
     
      goto done;
   }
   
done:
   return rc;

error:
   goto done;
}

void fileCreatingMgr::flush(INT64 parentId, 
                            CHAR* fileName, 
                            lobHandle* handle, 
                            BOOLEAN &isProcessed)
{
   isProcessed = FALSE;
   createFileNode* fileNode = NULL;
   INT32 hashKey = 0;

   if(!(handle->isCreate))
   {
      goto done;
   }

   hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketR(hashKey); 
   fileNode = _createBucket.get(hashKey, parentId, fileName);
   if(fileNode != NULL)
   {
      SDB_ASSERT(handle == fileNode->handle, "fileNode->handle must same with handle");
      if(fileNode->handle != NULL &&
         (CREATE_STATUS_CACHEMETA == fileNode->handle->status 
         || CREATE_STATUS_CACHEDATA == fileNode->handle->status))
      {
         isProcessed = TRUE;
      }
   }
   _createBucket.unLockBucketR(hashKey);

done:
   return;
}

INT32 fileCreatingMgr::release(INT64 parentId, CHAR* fileName, 
                               lobHandle* handle, BOOLEAN &isProcessed)
{
   INT32 rc = SDB_OK;
   createFileNode* fileNode = NULL;
   INT32 hashKey = _createBucket.hash(parentId, fileName);
   BOOLEAN toFlush = FALSE;

   PD_LOG(PDDEBUG, "fileCreatingMgr release, parentId=%d, fileName=%s", 
                    parentId, fileName);

   isProcessed = FALSE;

   if(!(handle->isCreate))
   {
      goto done;
   }

   _createBucket.lockBucketW(hashKey);
   fileNode = _createBucket.get(hashKey, parentId, fileName);
   if(NULL == fileNode)
   {
      _createBucket.unLockBucketW(hashKey);
      goto done;
   }

   SDB_ASSERT(handle == fileNode->handle, "fileNode->handle must same with handle");

   if(CREATE_STATUS_CACHEMETA != handle->status 
      && CREATE_STATUS_CACHEDATA != handle->status)
   {
      _createBucket.unLockBucketW(hashKey);
      goto done;
   }
   
   //write file to disk
   rc = writeTmpFile(fileNode);
   if(SDB_OK == rc)
   {
      handle->status = CREATE_STATUS_INDISK;
      if(SDB_OK != addTask(parentId, fileName))
      {
         toFlush = TRUE;
      }
   }
   else 
   {
      toFlush = TRUE;
   }
   fileNode->handle = NULL;
   _createBucket.unLockBucketW(hashKey);

   if(toFlush)
   {
      // upload cache to db 
      // and delete file in map and bucket whether upload is succeeds or fails
      rc = _uploadCachetoDB(parentId, fileName);
      _mapRemoveFile(parentId, fileName);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "upload cache to db failed, the file will be lost."
                         "parentId=%d, fileName=%s, filePath=%s. rc=%d", 
                          parentId, fileName, fileNode->meta.symLink(), rc);
         _createBucket.lockBucketW(hashKey);
         fileNode = _createBucket.del(hashKey, parentId, fileName);
         SAFE_OSS_DELETE(fileNode);
         _createBucket.unLockBucketW(hashKey);
         _removeTmpFile(parentId, fileName);
         goto error;
      }
   }
   
   isProcessed = TRUE;
   
done:
   return rc;

error:
   goto done;
}
                       
INT32 fileCreatingMgr::query(INT64 parentId, CHAR* fileName, fileMeta &file)
{
   INT32 rc = SDB_OK;
   createFileNode* fileNode = NULL;
   INT32 hashKey = 0;

   PD_LOG(PDDEBUG, "fileCreatingMgr query, parentId=%d, fileName=%s", 
                    parentId, fileName);

   if(!_useCache)
   {
      rc = SDB_DMS_EOC;
      goto error;
   }

   hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketR(hashKey);
   fileNode = _createBucket.get(hashKey, parentId, fileName);
   if(fileNode != NULL)
   {
      file = fileNode->meta;
   }
   else 
   {
      rc = SDB_DMS_EOC;
      _createBucket.unLockBucketR(hashKey);
      goto error;
   }

   _createBucket.unLockBucketR(hashKey);

done:
   return rc;
error:
   goto done;
}

BOOLEAN fileCreatingMgr::isDirEmpty(INT64 parentId)
{
   BOOLEAN isEmpty = TRUE;
   ossPoolMap<int, fileItem*>::iterator iter;

   PD_LOG(PDDEBUG, "fileCreatingMgr isDirEmpty, parentId=%d", 
                    parentId);
   
   //check if fileCreateCache and running
   if(!_useCache)
   {
      goto done;
   }

   //find from parentIdmap
   _parentIdMapMutex.get();
   if(_parentIdMap.count(parentId) > 0)
   {
      isEmpty = FALSE;
   }
   _parentIdMapMutex.release();

done:
   return isEmpty;
}

INT32 fileCreatingMgr::flushfile(INT64 parentId, CHAR* fileName)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "fileCreatingMgr flushfile, parentId=%d, fileName=%s", 
                    parentId, fileName);

   if(!_useCache)
   {
      goto done;
   }

   rc = _uploadCachetoDB(parentId, fileName);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "_upload cache to DB failed, parentId=%d, fileName=%s", 
                       parentId, fileName);
      goto error;
   }

   _mapRemoveFile(parentId, fileName);

done:
   return rc;

error:
   goto done;   
}

INT32 fileCreatingMgr::flushDir(INT64 parentId) 
{
   INT32 rc = SDB_OK;
   multimap<int, string> fileMap;
   multimap<int, string>::iterator fileIter;
   INT32 count = 0;

   PD_LOG(PDDEBUG, "fileCreatingMgr flushDir, parentId=%d", parentId);
   
   //check fileCreateCache
   if(!_useCache)
   {
      goto done;
   }

   rc = _getFilesFromDirMap(parentId, fileMap);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "_getFilesFromDirMap failed, parentId=%d. rc=%d", 
                          parentId, rc);
      goto error;
   }

   for(fileIter = fileMap.begin(); fileIter != fileMap.end(); fileIter++)
   {
      rc = _uploadCachetoDB(parentId, fileIter->second.c_str());
      if(rc != SDB_OK && rc != SDB_IXM_DUP_KEY)
      {
         PD_LOG(PDERROR, "_upload cache to DB failed."
                         "parentId=%d, fileName=%s. rc=%d", 
                          parentId, fileIter->second.c_str(), rc);
         goto error;
      }

      _mapRemoveFile(parentId, fileIter->second.c_str());
      count++;
   }

   PD_LOG(PDDEBUG, "fileCreatingMgr flushDir, parentId=%d, flushFiles=%d",
                    parentId, count);

done:
   return rc;

error:
   goto done;     
}

INT32 fileCreatingMgr::unlink(INT64 parentId, CHAR* fileName, BOOLEAN &isProcessed)
{
   INT32 rc = SDB_OK;
   createFileNode* fileNode = NULL;
   INT32 hashKey = 0;

   PD_LOG(PDDEBUG, "fileCreatingMgr unlink, parentId=%d, fileName=%s", 
                    parentId, fileName);

   isProcessed = FALSE;

   if(!_useCache)
   {
      goto done;
   }

   hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketW(hashKey);
   fileNode = _createBucket.get(hashKey, parentId, fileName);
   if(NULL == fileNode)
   {
      _createBucket.unLockBucketW(hashKey);
      goto done;
   }

   if(fileNode->handle != NULL)
   {
      _createBucket.unLockBucketW(hashKey);
      rc = SDB_OPERATION_CONFLICT;
      goto error;
   }
   else
   {
      _createBucket.del(hashKey, parentId, fileName);
      SAFE_OSS_DELETE(fileNode);
      _removeTmpFile(parentId, fileName);
      isProcessed = TRUE;
   }

   _createBucket.unLockBucketW(hashKey);

   if(isProcessed)
   {
      _mapRemoveFile(parentId, fileName);
   }

done:
   return rc;

error:
   goto done;
}

INT32 fileCreatingMgr::_uploadCachetoDB(INT64 parentId, const CHAR* fileName) 
{
   INT32 rc = SDB_OK;
   createFileNode *node = NULL;
   fsConnectionDao uploadDb(_ds);
   OID lobId;
   INT64 lobSizeNew;

   INT32 hashKey = _createBucket.hash(parentId, fileName);
   _createBucket.lockBucketW(hashKey);
   node = _createBucket.get(hashKey, parentId, fileName);
   if(node == NULL)
   {
      _createBucket.unLockBucketW(hashKey);
      PD_LOG(PDDEBUG, "The file is not in the filecreatingmgr,"
                      "parentId=%d, fileName=%s", 
                       parentId, fileName);
      goto done;
   }

   rc = uploadDb.writeNewLob(node->data, _clFullName.c_str(), lobId, 0,
                             node->meta.size(), lobSizeNew);
   if(rc != SDB_OK)
   {
      _createBucket.unLockBucketW(hashKey);
      PD_LOG(PDERROR, "writeNewLob failed, parentId=%d, fileName=%s", 
                       parentId, fileName);
      goto error;
   }

   node->meta.setLobOid(lobId.toString().c_str());
   rc = _insertMeta(uploadDb, &(node->meta));
   if(rc != SDB_OK)
   {
      _createBucket.unLockBucketW(hashKey);
      PD_LOG(PDERROR, "_insertMeta failed, parentId=%d, fileName=%s", 
                       parentId, fileName);
      goto error;
   }
   //delete tmp file
   _removeTmpFile(parentId, fileName);
   //dlelete datacache
   if(node->data)
   {
      SDB_THREAD_FREE(node->data);
      node->data = NULL;
   }

   //handle is not null，set loboid and status
   if(node->handle)
   {
      node->handle->oid = lobId;
      node->handle->status = CREATE_STATUS_INDB;
   }

   _createBucket.del(hashKey, parentId, fileName);
   SAFE_OSS_DELETE(node);

   _fileCount.dec();
   _createBucket.unLockBucketW(hashKey);
   
done:
   return rc;
error:
   goto done;
}

void fileCreatingMgr::_uploadTask()
{
   CHAR tmpFileName[FS_MAX_NAMESIZE + 1] = {0};
   INT32 tryTimes = 0;
   
   while(_running)
   {
      fileItem item;
      if(_uploadQueue.timed_wait_and_pop(item, OSS_ONE_SEC)) 
      {
         tryTimes = 0;
         while(_running)
         {
            INT32 rc = _uploadCachetoDB(item._parentId, item._fileName);
            if(rc != SDB_OK)
            {
               if(SDB_IXM_DUP_KEY == rc)
               {
                  ossMemset(tmpFileName, 0, FS_MAX_NAMESIZE + 1);
                  ossSnprintf(tmpFileName, FS_MAX_NAMESIZE, "%ld.%s.file", 
                              item._parentId, item._fileName);
                  rc = _moveTmpFileToBak(tmpFileName);
                  if(SDB_OK != rc)
                  {
                     PD_LOG(PDERROR, "Move conflict file failed, filename:%s. rc=%d", 
                                      tmpFileName, rc);
                  }
                  break;
               }
               else     
               {
                  ++tryTimes;
                  PD_LOG(PDERROR, "_upload cache to DB failed, try again. trytimes=%d"
                                  "parentId=%d, fileName=%s. rc=%d", 
                                   tryTimes, item._parentId, item._fileName, rc);
                  ossSleep(OSS_ONE_SEC);
                  continue;
               }
            }
            else 
            {
               _mapRemoveFile(item._parentId, item._fileName);
               break;
            }
         }
      }
   }
}

INT32 fileCreatingMgr::_uploadFileToDB(CHAR* buf, const CHAR* filePath)
{
   INT32 rc = SDB_OK;
   ossFile readFile;
   fsConnectionDao uploadDb(_ds);
   OID lobId;
   INT64 lobSizeNew;
   CHAR* bufData = buf + CACHE_META_SIZE;
   INT64 readLen = 0;
   fileMeta* meta = (fileMeta*)buf;

   rc = readFile.open(filePath, OSS_READONLY, OSS_DEFAULTFILE | OSS_RO);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Open file failed, filePath=%s. rc=%d", 
                       filePath, rc);
      goto error;
   }

   rc = readFile.readN((CHAR*)meta, sizeof(fileMeta), readLen);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Read file failed, filePath=%s. rc=%d", 
                       filePath, rc);
      goto error;
   }

   if(readLen < 0 || (UINT64)readLen < sizeof(fileMeta))
   { 
      rc = SDB_INVALIDSIZE;
      PD_LOG(PDERROR, "Read file meta failed, "
                      "readLen=%ld, needLen=%ld, filePath=%s", 
                       readLen, sizeof(fileMeta), filePath);
      goto error;
   }

   if(meta->size() > 0)
   {
      rc = readFile.seekAndReadN(CACHE_META_SIZE, bufData, meta->size(), readLen);
      if(SDB_OK != rc)
      {
         goto error;
      }

      if(readLen < meta->size())
      {
        rc = SDB_INVALIDSIZE;
        PD_LOG(PDERROR, "read file data failed, "
                        "readLen=%ld, filesize=%d, filePath=%s", 
                         readLen, meta->size(), filePath);
         goto error;
      }
   }

   rc = uploadDb.writeNewLob(bufData, _clFullName.c_str(), lobId, 0,
                             meta->size(), lobSizeNew);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "writeNewLob failed, fileName=%s", 
                       filePath);
      goto error;
   }

   meta->setLobOid(lobId.toString().c_str());
   rc = _insertMeta(uploadDb, meta);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "_insertMeta failed, fileName=%s", 
                       filePath);
      goto error;
   }

   rc = readFile.close();  
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "close file failed, fileName=%s", 
                       filePath);
      goto error;
   }

   rc = readFile.deleteFile(filePath);  
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "del file(%s) failed, rc: %d", filePath, rc );
      goto error;
   }
   
done:
   return rc;
error:
   goto done;
}

INT32 fileCreatingMgr::addTask(INT64 parentId, CHAR* fileName) 
{
   INT32 rc = SDB_OK;
   
   fileItem task(parentId, fileName);
   try
   {
      _uploadQueue.push(task);
   }
   catch(std::exception &e2)
   {
      PD_LOG(PDERROR, "bad_alloc, error=%s", e2.what()); 
      rc = SDB_OOM;
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fileCreatingMgr::_getFilesFromDirMap(INT64 parentId, 
                                           multimap<int, string> &dirFiles)
{
   INT32 rc = SDB_OK;
   ossPoolMap<int, fileItem*>::iterator iter;
   fileItem* curFile = NULL;

   _parentIdMapMutex.get();
   iter = _parentIdMap.find(parentId);
   if(iter != _parentIdMap.end())
   {
      curFile = iter->second;
      while(curFile != NULL)
      {
         try
         {
            dirFiles.insert( pair<int, string>(parentId, curFile->_fileName));
         }
         catch(std::exception &e)
         {
            PD_LOG(PDERROR, "bad_alloc, error=%s", e.what());
            rc = SDB_OOM;
            goto error;
         }
         curFile = curFile->_next;
      }
   }
   _parentIdMapMutex.release();

done:
   return rc;
   
error:
   _parentIdMapMutex.release();
   goto done;   
}

INT32 fileCreatingMgr::_addFileToDirMap(INT64 parentId, CHAR* fileName)
{
   INT32 rc = SDB_OK;
   ossPoolMap<int, fileItem*>::iterator iter;
   fileItem* curfile = NULL;
   fileItem* prefile = NULL;
   fileItem* newfile = NULL;
   
   iter = _parentIdMap.find(parentId);
   if(iter != _parentIdMap.end())
   {
      curfile = iter->second;
      while(curfile != NULL)
      {
         prefile = curfile;
         if(0 == ossStrncmp(curfile->_fileName, fileName, FS_MAX_NAMESIZE))
         {
            PD_LOG(PDERROR, "dup file, parentId=%d, fileName=%s", 
                             parentId, fileName);
            rc = SDB_FE;
            goto error;
         }
         curfile = curfile->_next;
      }
      
      newfile = SDB_OSS_NEW fileItem(parentId, fileName); 
      if(NULL == newfile)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "new fileItem failed, parentId=%d, fileName=%s. rc=%d", 
                          parentId, fileName, rc);
         goto error;
      }

      prefile->_next = newfile;
      newfile->_pre = prefile;
   }
   else 
   {
      newfile = SDB_OSS_NEW fileItem(parentId, fileName);
      if(NULL == newfile)
      {
         rc = SDB_OOM;
         PD_LOG(PDERROR, "new fileItem failed, parentId=%d, fileName=%s. rc=%d", 
                          parentId, fileName, rc);
         goto error;
      }
      
      try
      {
         _parentIdMap[parentId] = newfile;
      }
      catch(std::exception &e)
      {
         SAFE_OSS_DELETE(newfile);
         PD_LOG(PDERROR, "bad_alloc, error=%s", e.what());
         rc = SDB_OOM;
         goto error;
      }
   }

done:
   return rc;
   
error:
   goto done;   
}

void fileCreatingMgr::_mapRemoveFile(INT64 parentId, const CHAR* fileName)
{
   _parentIdMapMutex.get();
   fileItem* fileInMap = _removeFileFromDirMap(parentId, fileName);
   SAFE_OSS_DELETE(fileInMap);
   _parentIdMapMutex.release();
}

fileItem* fileCreatingMgr::_removeFileFromDirMap(INT64 parentId, const CHAR* fileName)
{
   ossPoolMap<int, fileItem*>::iterator iter;
   fileItem* curfile = NULL;
   
   iter = _parentIdMap.find(parentId);
   if(iter != _parentIdMap.end())
   {
      curfile = iter->second;
      while(curfile != NULL)
      {
         if(0 == ossStrncmp(curfile->_fileName, fileName, FS_MAX_NAMESIZE))
         {
            break;
         }
         curfile = curfile->_next;
      }
      
      if(curfile)
      {
         if(curfile->_pre == NULL && curfile->_next == NULL)
         {
            _parentIdMap.erase(parentId);
         }
         else
         {
            if(curfile->_next)
            {
               curfile->_next->_pre = curfile->_pre;
            }

            if(curfile->_pre)
            {
               curfile->_pre->_next = curfile->_next;
            }
            else
            {
               _parentIdMap[parentId] = curfile->_next;  
            }
         }
      }
   }

   return curfile;
}

INT32 fileCreatingMgr::writeTmpFile(createFileNode* fileNode)
{
   INT32 rc = SDB_OK;
   CHAR fileName[FS_MAX_NAMESIZE + 1] = {0};
   string filePath;
   ossFile createFile;
   INT64 writeLen = 0;
   BOOLEAN isCreateTmpFile = FALSE;

   if(NULL == fileNode)
   {
      rc = SDB_SYS;
      goto error;
   }

   ossSnprintf(fileName, FS_MAX_NAMESIZE, "%ld.%s.file", 
               fileNode->meta.pid(),  fileNode->meta.name());
   filePath = _fileWorkPath + fileName;  
   
   //check file is exist
   rc = ossAccess( filePath.c_str(), OSS_MODE_READ | OSS_MODE_WRITE);
   if(SDB_OK == rc)
   {
      rc = SDB_FE;
      PD_LOG( PDERROR, "file(%s) is exist, rc=%d", filePath.c_str(), rc);
      goto error;
   }      
 
   rc = createFile.open(filePath, OSS_REPLACE | OSS_WRITEONLY,
                        OSS_DEFAULTFILE | OSS_RO);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "open file(%s) failed, rc=%d", filePath.c_str(), rc);
      goto error ;
   }
   isCreateTmpFile = TRUE;

   rc = createFile.writeN((CHAR*)&(fileNode->meta), sizeof(fileMeta));
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "write meta to file(%s) failed, rc=%d", 
                        filePath.c_str(), rc);
      goto error ;
   }

   if(fileNode->meta.size() > 0)
   {      
      rc = createFile.seekAndWrite(CACHE_META_SIZE, 
                                   fileNode->data, 
                                   fileNode->meta.size(), 
                                   writeLen);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "write data to file(%s) failed, rc=%d", 
                           filePath.c_str(), rc);
         goto error ;
      }
   }

   rc = createFile.close();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "close file(%s) failed, rc=%d", filePath.c_str(), rc);
      goto error ;
   }

   PD_LOG(PDDEBUG, "_writeTmpFile (%s)", filePath.c_str());
done:
   return rc;
   
error:
   if(isCreateTmpFile)
   {
      createFile.close();
      createFile.deleteFileIfExists(filePath);
   }
   goto done;        
}

INT32 fileCreatingMgr::_removeTmpFile(INT64 parentId, const CHAR* fileName)
{
   INT32 rc = SDB_OK;
   CHAR tmpName[FS_MAX_NAMESIZE + 1] = {0};
   string filePath;
   ossFile createFile;

   ossSnprintf( tmpName, FS_MAX_NAMESIZE,
                "%ld.%s.file", parentId, fileName);
   filePath = _fileWorkPath + tmpName; 
   
   rc = createFile.deleteFileIfExists(filePath);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "del file(%s) failed, rc=%d", filePath.c_str(), rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;    
}

INT32 fileCreatingMgr::_moveTmpFileToBak(const CHAR* tmpFileName)
{
   INT32 rc = SDB_OK;
   string srcFilePathName = _fileWorkPath + tmpFileName;
   string dstFilePathName = _fileConflictPath + tmpFileName;
   ossFile moveFile;   

   rc = moveFile.rename(srcFilePathName, dstFilePathName);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "rename (%s) to (%s) failed, rc=%d", 
                        srcFilePathName.c_str(), dstFilePathName.c_str(), rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 fileCreatingMgr::_insertMeta(fsConnectionDao &db,
                                   fileMeta* meta)
{
   INT32 rc = SDB_OK;
   BSONObj obj;

   try
   {
      obj = BSON(SEQUOIAFS_NAME<<meta->name()<<\
                 SEQUOIAFS_MODE<<meta->mode()<<\
                 SEQUOIAFS_UID<<meta->uid()<<\
                 SEQUOIAFS_GID<<meta->gid()<<\
                 SEQUOIAFS_NLINK<<meta->nLink()<<\
                 SEQUOIAFS_PID<<meta->pid()<<\
                 SEQUOIAFS_LOBOID<<meta->lobOid()<<\
                 SEQUOIAFS_SIZE<<meta->size()<<\
                 SEQUOIAFS_CREATE_TIME<<meta->ctime()<<\
                 SEQUOIAFS_MODIFY_TIME<<meta->mtime()<<\
                 SEQUOIAFS_ACCESS_TIME<<meta->atime()<<\
                 SEQUOIAFS_SYMLINK<<meta->symLink());
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }

   rc = db.insertMeta(_fileCLName.c_str(), obj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "write file meta to db failed. rc=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}


