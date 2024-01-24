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

   Source File Name = sequoiaFSMetaCache.cpp

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

#include "sequoiaFSMetaCache.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSDao.hpp"

using namespace bson;
using namespace sdbclient;
using namespace sequoiafs;

INT32 metaFromBSON(BSONObj record, metaNode *meta)
{
   INT32 rc = SDB_OK;
   
   BSONObjIterator itr(record);
   while (itr.more())
   {
      BSONElement ele = itr.next();
      if ( 0 == ossStrcmp(SEQUOIAFS_SIZE, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_SIZE);
         meta->setSize(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_CREATE_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_CREATE_TIME);
         meta->setCtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_ACCESS_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_ACCESS_TIME);
         meta->setAtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODIFY_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODIFY_TIME);
         meta->setMtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODE, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODE);
         meta->setMode(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NLINK, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_NLINK);
         meta->setNLink(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_UID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_UID);
         meta->setUid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_GID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_GID);
         meta->setGid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_SYMLINK, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_SYMLINK);
         meta->setSymLink((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NAME, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_NAME);
         meta->setName((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_PID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_PID);
         meta->setPid(ele.numberLong());
      }
   }

done:
   return rc;
error:
   goto done; 
}

INT32 fMetaFromBSON(BSONObj record, fileMeta *meta)
{
   INT32 rc = SDB_OK;
   
   BSONObjIterator itr(record);
   while (itr.more())
   {
      BSONElement ele = itr.next();
      if ( 0 == ossStrcmp(SEQUOIAFS_SIZE, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_SIZE);
         meta->setSize(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_CREATE_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_CREATE_TIME);
         meta->setCtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_ACCESS_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_ACCESS_TIME);
         meta->setAtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODIFY_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODIFY_TIME);
         meta->setMtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODE, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODE);
         meta->setMode(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NLINK, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_NLINK);
         meta->setNLink(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_UID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_UID);
         meta->setUid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_GID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_GID);
         meta->setGid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_SYMLINK, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_SYMLINK);
         meta->setSymLink((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NAME, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_NAME);
         meta->setName((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_PID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_PID);
         meta->setPid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_LOBOID, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_NAME);
         meta->setLobOid((CHAR*)ele.valuestrsafe());
      }
   }

done:
   return rc;
error:
   goto done; 
}

void metaFromMeta(const metaNode& oldMeta, metaNode *newMeta)
{
   newMeta->setName(oldMeta.name());
   newMeta->setMode(oldMeta.mode());
   newMeta->setUid(oldMeta.uid());
   newMeta->setGid(oldMeta.gid());
   newMeta->setNLink(oldMeta.nLink());
   newMeta->setPid(oldMeta.pid());
   newMeta->setSize(oldMeta.size());
   newMeta->setCtime(oldMeta.ctime());
   newMeta->setMtime(oldMeta.mtime());
   newMeta->setAtime(oldMeta.atime());
   newMeta->setSymLink(oldMeta.symLink());
}

fsMetaCache::fsMetaCache() 
:_dirCache(this),
 _fsReg(this),
 _msgHandler(this)
{
}

fsMetaCache::~fsMetaCache()
{
}

INT32 fsMetaCache::init(sdbConnectionPool* ds, 
                        string dirCLName, 
                        string fileCLName,
                        string collection,
                        string mountpoint,
                        INT32 capacity,
                        BOOLEAN standalone)
{
   INT32 rc = SDB_OK;
   dirMeta dMeta;

   rc = _dirCache.init(ds, dirCLName, capacity, standalone);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "_dirCache init failed. dirCLName:%s, fileCLName:%s", 
                      dirCLName.c_str(), fileCLName.c_str());
      goto error;
   }

   _ds = ds;
   _dirCLName = dirCLName;
   _fileCLName = fileCLName;

   if(standalone)
   {
      setCacheMeta();
   }
   else
   {
      rc = _fsReg.start(ds, (CHAR *)collection.c_str(), (CHAR *)mountpoint.c_str());
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Fail to start register. rc=%d", rc);
         rc = SDB_OK;
      }

      try
      {
         _thFSReg = new boost::thread(boost::bind( &fsRegister::hold, &_fsReg));
      }
      catch(std::bad_alloc &e1)
      {
         PD_LOG(PDERROR, "bad_alloc thread, error=%s", e1.what());
         rc = SDB_OOM;
         goto error;
      }
      catch ( boost::thread_resource_error &e2)
      {
         PD_LOG(PDERROR, "Failed to start register, error=%s", e2.what());
         rc = SDB_SYS; 
         goto error;
      }
      catch(std::exception &e3)
      {
         PD_LOG(PDERROR, "Failed to start register, error=%s", e3.what());
         rc = SDB_SYS; 
         goto error;
      }
   }

done:
   return rc;
error:
   goto done;
}

void fsMetaCache::fini()
{
   _fsReg.fini();
   if(_thFSReg != NULL)
   {
      _thFSReg->join();
   }
}

INT32 fsMetaCache::getDirInfo(INT64 parentId, const CHAR* name, dirMeta &meta)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "metacache getDirInfo. parentId:%d, path:%s", parentId, name);
   
   rc = _dirCache.getDirInfo(parentId, name, meta);
   if(rc != SDB_OK)
   {
      PD_LOG(PDDEBUG, "getDirInfo failed. parentId:%d, path:%s, rc=%d", parentId, name, rc);
   }

   return rc;
}

INT32 fsMetaCache::getFileInfo(INT64 parentId, const CHAR* name, fileMeta &meta)
{
   INT32 rc = SDB_OK;
   dirMeta sDir;
   fileMeta file;
   BSONObj condition;
   BSONObj record;
   fsConnectionDao db(_ds);
   sdbCollection   fileCL;

   PD_LOG(PDDEBUG, "metacache getFileInfo. parentId:%d, file name:%s", parentId, name);

   try   
   {
      condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentId);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = db.getCL(_fileCLName.c_str(), fileCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _fileCLName.c_str(), rc);
      goto error;
   }
   rc = db.queryMeta(fileCL, condition, record);
   if(rc != SDB_OK)
   {
      if(SDB_DMS_EOC == rc) 
      {
         PD_LOG(PDINFO, "query File Meta failed. path:%s, id:%d, rc=%d", 
                      name, parentId, rc);
      }
      else
      {
         PD_LOG(PDERROR, "query File Meta failed. path:%s, id:%d, rc=%d", 
                      name, parentId, rc);
      }
      goto error;
   }
   
   rc = fMetaFromBSON(record, &meta);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "metaFromBSON failed. path:%s", name);
      goto error;
   }
   
done:
   return rc;
error:
   goto done;    
}

INT32 fsMetaCache::getParentIdName(const CHAR* path, string *basePath, INT64 &parentId)
{
   INT32 rc = SDB_OK;
   CHAR *dir;
   dirMeta meta;
   const CHAR *delimiter = "/";
   CHAR *pTemp = NULL;
   vector<string> dirName;
   parentId = 0;
   CHAR *pathStr = NULL;
   
   PD_LOG(PDDEBUG, "metacache getPIdName. path:%s", path);

   pathStr = ossStrdup(path);
   if(NULL == pathStr)
   {
      PD_LOG(PDERROR, "Failed to backup path, path:%s", path);
      rc = SDB_OOM;
      goto error;
   }
   dir = ossStrtok(pathStr, delimiter, &pTemp);
   while(dir)
   {
      dirName.push_back(dir);
      dir = ossStrtok(NULL, delimiter, &pTemp);
   }

   if(dirName.size() == 0)
   {
      *basePath = "/";
      goto done;
   }

   parentId = ROOT_ID;

   for(UINT32 i = 0; i < dirName.size() - 1; i++)
   {
      rc = _dirCache.getDirInfo(parentId, dirName[i].c_str(), meta);
      if(rc == SDB_OK)
      {
         parentId = meta.id();
         continue;
      }
      else
      {
         goto error;
      }
   }
      
   *basePath = dirName[dirName.size() - 1];

done:
   SAFE_OSS_FREE(pathStr);
   return rc;
error:
   goto done;   
}

INT32 fsMetaCache::isDir(INT64 parentId, const CHAR* name, BOOLEAN *isDir)
{
   INT32 rc = SDB_OK;
   dirMeta meta;
   BSONObj condition;
   BSONObj record;
   fsConnectionDao db(_ds);
   sdbCollection   fileCL;

   *isDir = TRUE;
   rc = _dirCache.getDirInfo(parentId, name, meta);
   if(SDB_DMS_EOC == rc)   
   {
      try   
      {
         condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentId);
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error;
      }
      
      rc = db.getCL(_fileCLName.c_str(), fileCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",   //TODO:日志提示error还是rc要统一
                _fileCLName.c_str(), rc);
         goto error;
      }
      rc = db.queryMeta(fileCL, condition, record);
      if(rc != SDB_OK)
      {
         if(SDB_DMS_EOC == rc)
         {
            PD_LOG(PDINFO, "The file or directory does not exist. path:%s, id:%d", 
                            name, parentId);
         }
         else
         {
            PD_LOG(PDERROR, "query File Meta failed. path:%s, id:%d", 
                            name, parentId);
         }
         goto error;
      }
      *isDir = FALSE;
   }
   else if(rc != SDB_OK)
   {
      goto error;
   }

done:
   return rc;
error:
   goto done; 
}

INT32 fsMetaCache::isDirEmpty(INT64 id, BOOLEAN *isEmpty)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   sdbCursor cursorDir;
   sdbCursor cursorFile;
   BSONObj record;
   BSONElement ele;
   fsConnectionDao db(_ds);
   sdbCollection   fileCL;
   sdbCollection   dirCL;

   *isEmpty = TRUE;

   try
   {
      condition = BSON(SEQUOIAFS_PID<<id);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = db.getCL(_dirCLName.c_str(), dirCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _dirCLName.c_str(), rc);
      goto error;
   }

   rc = db.getCL(_fileCLName.c_str(), fileCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _fileCLName.c_str(), rc);
      goto error;
   }

   rc = dirCL.query(cursorDir, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, error=%d",
             rc);
      goto error;
   }

   if(SDB_OK == cursorDir.next(record))
   {
       *isEmpty = FALSE;
       cursorDir.close();
       goto done;
   }

   rc = fileCL.query(cursorFile, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file collection, error=%d",
             rc);
      goto error;
   }

   if(SDB_OK == cursorFile.next(record))
   {
       *isEmpty = FALSE;
       cursorFile.close();
       goto done;
   }
   
done:
   return rc;
error:
   goto done;   
}

INT32 fsMetaCache::getAttr(const CHAR* path, metaNode *meta)
{
   INT32 rc = SDB_OK;
   string basePath;
   INT64 parentId;
   dirMeta dir;
   fileMeta file;
   BSONObj condition;
   BSONObj record;
   fsConnectionDao db(_ds);
   sdbCollection   fileCL;
// /a/b/c/d
   PD_LOG(PDDEBUG, "metacache getAttr. path:%s", path);
   
   rc = getParentIdName(path, &basePath, parentId);  
   if(rc != SDB_OK)
   {
      PD_LOG(PDDEBUG, "Failed to get parent id and name, path:%s", path);
      goto error;
   }

   rc = _dirCache.getDirInfo(parentId, basePath.c_str(), dir);
   if(rc == SDB_OK)
   {
      metaFromMeta(dir, meta);
   }
   else if(SDB_DMS_EOC == rc)
   {
      try
      {
         condition = BSON(SEQUOIAFS_NAME<<basePath.c_str()<<SEQUOIAFS_PID<<parentId);
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error;
      }
      
      rc = db.getCL(_fileCLName.c_str(), fileCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
                _dirCLName.c_str(), rc);
         goto error;
      }

      rc = db.queryMeta(fileCL, condition, record);
      if(rc != SDB_OK)
      {
         if(SDB_DMS_EOC == rc)
         {
            PD_LOG(PDINFO, "The file or directory does not exist. path:%s, id:%d", 
                            basePath.c_str(), parentId);
         }
         else
         {
            PD_LOG(PDERROR, "query File Meta failed. path:%s, id:%d", 
                            basePath.c_str(), parentId);
         }
         goto error;
      }
      
      rc = metaFromBSON(record, meta);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "metaFromBSON failed. path:%s", path);
         goto error;
      }
   }
   else
   {
      PD_LOG(PDERROR, "query dir Meta failed. path:%s, id:%d", 
                            basePath.c_str(), parentId);
      goto error;
   }

done:
   return rc;
error:
   goto done;    
}

INT32 fsMetaCache::readDir(INT64 parentId, void *buf, fuse_fill_dir_t filler)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   sdbCursor cursorDir;
   sdbCursor cursorFile;
   BSONObj record;
   BSONElement ele;
   fsConnectionDao db(_ds);
   sdbCollection    fileCL;
   sdbCollection    dirCL;

   PD_LOG(PDDEBUG, "metacache readDir. parentId:%d", parentId);
   
   try   
   {
      condition = BSON(SEQUOIAFS_PID<<parentId);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = db.getCL(_fileCLName.c_str(), fileCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _fileCLName.c_str(), rc);
      goto error;
   }

   rc = db.getCL(_dirCLName.c_str(), dirCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _dirCLName.c_str(), rc);
      goto error;
   }
   
   rc = dirCL.query(cursorDir, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query dir collection, error=%d",
             rc);
      goto error;
   }

   rc = fileCL.query(cursorFile, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file collection, error=%d",
             rc);
      goto error;
   }

   while(SDB_OK == cursorDir.next(record))
   {
       ele = record.getField("Name");
       rc = filler(buf, ele.String().c_str(), NULL, 0);
       if(SDB_OK != rc)
       {
          PD_LOG(PDERROR, "Failed to fill dir to buf, error=%d",
             rc);
          goto error;
       }
   }

   while(SDB_OK == cursorFile.next(record))
   {
      ele = record.getField("Name");
      rc = filler(buf, ele.String().c_str(), NULL, 0);
      if(SDB_OK != rc)
      {
         goto error;
      }
   }

done:
   return rc;
error:
   goto done;   
}

INT32 fsMetaCache::delDir(fsConnectionDao* connection,
                          INT64 parentId, 
                          const CHAR* name)
{
   INT32 rc = SDB_OK;
   BSONObj hint;
   fsConnectionDao *db = connection;
   
   if(NULL == connection)
   {
      rc = SDB_SYS;
      PD_LOG(PDERROR, "connection is null.");
      goto error;
   }

   rc = _fsReg.sendNotify(parentId, name);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
      goto error;
   }
   
   _dirCache.releaseLocalDirCache(parentId, name);

   rc = db->delMeta(_dirCLName.c_str(), name, parentId, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to delMeta. error=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done; 
}

INT32 fsMetaCache::incDirLink(fsConnectionDao* connection,
                              INT64 id)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BSONObj record;
   metaNode dirMeta;
   fsConnectionDao *fsDao = connection;
   sdbCollection    dirCL;
   
   if(NULL == connection)
   {
      rc = SDB_SYS;
      PD_LOG(PDERROR, "connection is null.");
      goto error;
   }
   
   rc = fsDao->getCL(_dirCLName.c_str(), dirCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _dirCLName.c_str(), rc);
      goto error;
   }

   try   
   {
      condition = BSON(SEQUOIAFS_ID<<(INT64)id);
      rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<1));
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao->queryMeta(dirCL, condition, record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDINFO, "queryMeta failed. id:%d， rc=%d", id, rc); 
      goto error;
   }

   rc = metaFromBSON(record, &dirMeta);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "metaFromBSON failed. id:%s", id);
      goto error;
   }

   rc = _fsReg.sendNotify(dirMeta.pid(), (CHAR*)dirMeta.name());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
      goto error;
   }

   _dirCache.releaseLocalDirCache(dirMeta.pid(), (CHAR*)dirMeta.name());

   rc = fsDao->updateMeta(_dirCLName.c_str(), condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;    
}

INT32 fsMetaCache::decDirLink(fsConnectionDao* connection, INT64 id)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BSONObj record;
   metaNode dirMeta;
   fsConnectionDao *fsDao = connection;
   sdbCollection    dirCL;

   if(NULL == connection)
   {
      rc = SDB_SYS;
      PD_LOG(PDERROR, "connection is null.");
      goto error;
   }

   try   
   {
      condition = BSON(SEQUOIAFS_ID<<(INT64)id);
      rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<-1));
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao->getCL(_dirCLName.c_str(), dirCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _dirCLName.c_str(), rc);
      goto error;
   }

   rc = fsDao->queryMeta(dirCL, condition, record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDINFO, "The file or directory does not exist. id:%d", 
                      id);
      goto error;
   }

   rc = metaFromBSON(record, &dirMeta);
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "metaFromBSON failed. id:%s", id);
      goto error;
   }

   rc = _fsReg.sendNotify(dirMeta.pid(), (CHAR*)dirMeta.name());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
      goto error;
   }

   _dirCache.releaseLocalDirCache(dirMeta.pid(), (CHAR*)dirMeta.name());
   
   rc = fsDao->updateMeta(_dirCLName.c_str(), condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;    
}

INT32 fsMetaCache::renameEntity(fsConnectionDao* connection, 
                                INT64 oldParentId, CHAR* oldName, 
                                INT64 newParentId, CHAR* newName)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BOOLEAN is_dir;
   fsConnectionDao *fsDao = connection;

   if(NULL == connection)
   {
      rc = SDB_SYS;
      PD_LOG(PDERROR, "connection is null.");
      goto error;
   }

   rc = isDir(oldParentId, oldName, &is_dir);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to isDir, error=%d", rc);
      goto error;
   }

   if(is_dir)
   {
      rc = _fsReg.sendNotify(oldParentId, oldName);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
         goto error;
      }

      _dirCache.releaseLocalDirCache(oldParentId, oldName);
   }
   
   rc = fsDao->transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin. rc=%d", rc);
      goto error;
   }

   try   
   {
      rule = BSON("$set"<<BSON(SEQUOIAFS_NAME<<newName<<SEQUOIAFS_PID<<newParentId));
      condition = BSON(SEQUOIAFS_NAME<<oldName<<SEQUOIAFS_PID<<(INT64)oldParentId);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao->updateMeta(is_dir ? _dirCLName.c_str() : _fileCLName.c_str(), condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   fsDao->transCommit();

done:
   return rc;
error:
   goto done; 
}

INT32 fsMetaCache::modMetaMode(INT64 parentId, 
                               CHAR* name, 
                               INT32 newMode)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BOOLEAN is_dir;
   fsConnectionDao fsDao(_ds);
   BOOLEAN isTransBegin = FALSE;
   
   rc = isDir(parentId, name, &is_dir);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to isDir, error=%d", rc);
      goto error;
   }

   if(is_dir)
   {
      // send notify to mcs
      rc = _fsReg.sendNotify(parentId, name);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
         goto error;
      }

      _dirCache.releaseLocalDirCache(parentId, name);
   }

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin. rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   try   
   {
      condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentId);
      rule = BSON("$set"<<BSON(SEQUOIAFS_MODE<<newMode));
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao.updateMeta(is_dir ? _dirCLName.c_str() : _fileCLName.c_str(), 
                           condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   if(isTransBegin)
   {
      fsDao.transRollback();
   }
   goto done;
}

INT32 fsMetaCache::modMetaOwn(INT64 parentId, 
                              CHAR* name, 
                              UINT32 newUid, 
                              UINT32 newGid)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BOOLEAN is_dir;
   fsConnectionDao fsDao(_ds);
   BOOLEAN isTransBegin = FALSE;

   rc = isDir(parentId, name, &is_dir);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get dir, error=%d", rc);
      goto error;
   }

   if(is_dir)
   {
      rc = _fsReg.sendNotify(parentId, name);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to sendNotify, error=%d", rc);
         goto error;
      }

      _dirCache.releaseLocalDirCache(parentId, name);
   }
   
   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin. rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   try   
   {
      condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentId);
      if(newUid != INVALID_UID_GID && newGid != INVALID_UID_GID)
      {
         rule = BSON("$set"<<BSON(SEQUOIAFS_UID<<newUid<<SEQUOIAFS_GID<<newGid));
      }
      else if(newUid != INVALID_UID_GID)
      {
         rule = BSON("$set"<<BSON(SEQUOIAFS_UID<<newUid));
      }
      else if(newGid != INVALID_UID_GID)
      {
         rule = BSON("$set"<<BSON(SEQUOIAFS_GID<<newGid));
      }
      else 
      {
         rc = SDB_INVALIDARG;
         PD_LOG(PDERROR, "chown must specify uid or gid. parentid:%d, name:%s", 
                          parentId, name);
         goto error;
      }
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao.updateMeta(is_dir ? _dirCLName.c_str() : _fileCLName.c_str(), 
                           condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   if(isTransBegin)
   {
      fsDao.transRollback();
   }
   goto done; 
}

INT32 fsMetaCache::modMetaUtime(INT64 parentId, 
                                CHAR* name, 
                                INT64 newMtime, 
                                INT64 newAtime)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   BOOLEAN is_dir;
   fsConnectionDao fsDao(_ds);
   BOOLEAN isTransBegin = FALSE;
  
   rc = isDir(parentId, name, &is_dir);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to is_dir, rc=%d", rc);
      goto error;
   }

   if(is_dir)
   {
      // send notify to mcs
      rc = _fsReg.sendNotify(parentId, name);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to sendNotify, rc=%d", rc);
         goto error;
      }

      _dirCache.releaseLocalDirCache(parentId, name);
   }

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin. rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   try   
   {
      condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentId);
      rule = BSON("$set"<<BSON(SEQUOIAFS_MODIFY_TIME<<newMtime<<\
                               SEQUOIAFS_ACCESS_TIME<<newAtime));//TODO:createTime不能修改
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }
   
   rc = fsDao.updateMeta(is_dir ? _dirCLName.c_str() : _fileCLName.c_str(),
                           condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   if(isTransBegin)
   {
      fsDao.transRollback();
   }
   goto done;
}

INT32 fsMetaCache::modMetaSize(CHAR* lobId, 
                               INT64 newSize, 
                               INT64 newMtime, 
                               INT64 newAtime)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   BSONObj hint;
   fsConnectionDao fsDao(_ds);
   BOOLEAN isTransBegin = FALSE;

   try   
   {
      rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<newSize<<\
                               SEQUOIAFS_MODIFY_TIME<<newMtime<<\
                               SEQUOIAFS_ACCESS_TIME<<newAtime));
      condition = BSON(SEQUOIAFS_LOBOID<<lobId);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error;
   }

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin. rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   rc = fsDao.updateMeta(_fileCLName.c_str(), 
                          condition, rule, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   if(isTransBegin)
   {
      fsDao.transRollback();
   }
   goto done;
}


