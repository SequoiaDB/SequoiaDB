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

   Source File Name = sequoiaFSDirMetaCache.cpp

   Descriptive Name = sequoiafs fuse file lru cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSDirMetaCache.hpp"
#include "sequoiaFSMetaCache.hpp"
#include "sequoiaFSLRUList.hpp"

#include "sequoiaFSDao.hpp"
#include "sequoiaFSCommon.hpp"

using namespace sequoiafs;
using namespace sdbclient;
using namespace bson;

INT32 dMetaFromBSON(BSONObj record, dirMeta &meta)
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
         meta.setSize(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_CREATE_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_CREATE_TIME);
         meta.setCtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_ACCESS_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_ACCESS_TIME);
         meta.setAtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODIFY_TIME, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODIFY_TIME);
         meta.setMtime(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_MODE, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_MODE);
         meta.setMode(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NLINK, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_NLINK);
         meta.setNLink(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_UID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_UID);
         meta.setUid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_GID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_GID);
         meta.setGid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_SYMLINK, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_SYMLINK);
         meta.setSymLink((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_NAME, ele.fieldName()))
      {
         PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not string",
                  SEQUOIAFS_NAME);
         meta.setName((CHAR*)ele.valuestrsafe());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_PID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_PID);
         meta.setPid(ele.numberLong());
      }
      if ( 0 == ossStrcmp(SEQUOIAFS_ID, ele.fieldName()))
      {
         PD_CHECK(NumberLong == ele.type() || NumberInt == ele.type(), SDB_INVALIDARG,
                  error, PDERROR, "The type of field:%s is not number",
                  SEQUOIAFS_ID);
         meta.setId(ele.numberLong());
      }
   }

done:
   return rc;
error:
   goto done; 
}

dirMetaCache::dirMetaCache(fsMetaCache* mCache)
:_dirList(),
 _dirHashBucket(this)
{
   _mCache = mCache;
   _standalone = TRUE;
}

dirMetaCache::~dirMetaCache()
{
}

INT32 dirMetaCache::init(sdbConnectionPool* ds, 
                         string dirCLName,
                         INT32 capacity,
                         BOOLEAN standalone)
{
   INT32 rc = SDB_OK;
   BSONObj record;
   dirMeta meta;

   _ds = ds;
   _dirCLName = dirCLName;
   _standalone = standalone;
   
   _dirDB = SDB_OSS_NEW fsConnectionDao(ds); 
   if(NULL == _dirDB)
   {
      rc = SDB_OOM;
      PD_LOG(PDERROR, "Fail to get connection, rc=%d", rc);
      goto error;
   }

   //TODO:待实现按需加锁放锁功能后再开启此事务
   /*
   rc = _dirDB->transBegin(TRUE);  
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to transBegin, cl=%s, rc=%d",
             dirCLName.c_str(), rc);
      goto error;
   }
   */

   rc = _dirDB->getCL(_dirCLName.c_str(), _dMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             dirCLName.c_str(), rc);
      goto error;
   }

   _dirList.init(capacity);

done:
   return rc;
error:
   goto done;
}

INT32 dirMetaCache::getDirInfo(INT64 parentId, const CHAR* dirName, dirMeta &meta)
{
   INT32 rc = SDB_OK;
   BOOLEAN isFind = FALSE;
   dirMetaNode* cur = NULL;
   BSONObj condition;
   BSONObj record;

   PD_LOG(PDDEBUG, "dirmeta getDirInfo. parentId:%d, path:%s", parentId, dirName);

   INT32 hash = _dirHashBucket.hashDirPid(parentId, dirName);
   if(_mCache->isCacheMeta())
   {
      _dirHashBucket.lockBucketR(hash);
      cur = _dirHashBucket.get(hash, dirName, parentId);
      if(cur != NULL)
      {
         isFind = TRUE;
         meta = cur->meta;
         cur->count++;
      }
      _dirHashBucket.unLockBucketR(hash);
   }

   if(!isFind)
   {
      _dirDBMutex.get();
      if(_mCache->isCacheMeta())
      {
         _dirHashBucket.lockBucketR(hash);
         cur = _dirHashBucket.get(hash, dirName, parentId);
         if(cur != NULL)
         {
            isFind = TRUE;
            meta = cur->meta;
            cur->count++;
         }
         _dirHashBucket.unLockBucketR(hash);
      }

      if(!isFind)
      {  
         try
         {
            condition = BSON(SEQUOIAFS_NAME<<dirName<<\
                             SEQUOIAFS_PID<<(INT64)parentId);
         }
         catch (std::exception &e)   
         {
            _dirDBMutex.release();
            rc = SDB_DRIVER_BSON_ERROR;
            PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
            goto error; 
         }
         rc = _dirDB->queryMeta(_dMetaCL, condition, record, TRUE);
         if(rc != SDB_OK)
         {
            if(SDB_DMS_EOC != rc) 
            {
               if(!_dirDB->isValid())
               {
                  _updateConnection();
                  releaseAllLocalCache(FALSE);   
               }
            }
            _dirDBMutex.release();
            goto error;
         }
      }
      _dirDBMutex.release();

      rc = dMetaFromBSON(record, meta); 
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "convert meta from BSON failed. rc=%d", rc);
         goto error;
      }

      if(_mCache->isCacheMeta())
      {
         _addDirInfo(meta);
      }
   }

done:
   return rc;
error:
   goto done;    
}

INT32 dirMetaCache::_addDirInfo(dirMeta &newMeta)
{
   INT32 rc = SDB_OK;
   dirMetaNode* cur = NULL;
   dirMetaNode *del = NULL;
   
   INT32 hash = _dirHashBucket.hashDirPid(newMeta.pid(), newMeta.name());
   _dirHashBucket.lockBucketW(hash);
   
   cur = _dirHashBucket.get(hash, newMeta.name(), newMeta.pid());
   if(cur != NULL)
   {
      cur->count++;
      cur->meta = newMeta;
      _dirHashBucket.unLockBucketW(hash);
      PD_LOG(PDERROR, "Dup add dir info dirname:%s, parentId:%d.", newMeta.name(), newMeta.pid());
   }
   else
   {
      dirMetaNode *node = new dirMetaNode(hash, &newMeta);
      if(NULL == node)
      {
         rc = SDB_OOM;
         _dirHashBucket.unLockBucketW(hash);
         PD_LOG(PDERROR, "Fail to new a dirMetaNode. rc=%d", rc);
         goto error;
      }
      _dirHashBucket.add(hash, node);
      _dirHashBucket.unLockBucketW(hash);
      
      _listMutex.get();
      _dirList.addCold(node);
      del = _dirList.findReduceCold(); 
      _listMutex.release();
      while(del != NULL)
      {
         _delDirInfo(del); 
         
         _listMutex.get();
         del = _dirList.findReduceCold();  
         _listMutex.release();
      }
   }
   
done:
   return rc;
   
error:
   goto done; 
}

void dirMetaCache::releaseAllLocalCache(BOOLEAN isReleseLock)
{ //TODO:分成两个函数，一个给内部在锁内调用，一个给外部调用
   if(isReleseLock)
   {
      _dirDBMutex.get();
      _updateConnection();
      _dirDBMutex.release();
   }
   
   while(_dirList._coldTail != NULL)
   {
      _delDirInfo(_dirList._coldTail);
   }
}

INT32 dirMetaCache::releaseLocalDirCache(INT64 parentId, const CHAR* dirName)
{
   INT32 rc = SDB_OK;
   dirMetaNode* cur = NULL;
   INT32 hash = _dirHashBucket.hashDirPid(parentId, dirName);

   if(!_mCache->isCacheMeta())
   {
      goto done;
   }

   _dirHashBucket.lockBucketR(hash);
   cur = _dirHashBucket.get(hash, dirName, parentId);
   _dirHashBucket.unLockBucketR(hash);
   if(cur != NULL)
   {
      rc = _delDirInfo(cur);
      if(rc != SDB_OK)
      {
         PD_LOG(PDERROR, "_delDirInfo failed. parentId:%d, dirName:%s, rc=%d", parentId, dirName, rc);
         goto error;
      }
   }

done:
   return rc;
   
error:
   goto done; 
}

INT32 dirMetaCache::_delDirInfo(dirMetaNode* node)
{
   INT32 rc = SDB_OK;
   dirMetaNode* delNode = NULL;
   
   if(NULL == node)
   {
      goto done;
   }

   _dirHashBucket.lockBucketW(node->getKey());
   delNode = _dirHashBucket.del(node->getKey(), node);
   _dirHashBucket.unLockBucketW(node->getKey());

   if(delNode != NULL)
   {
      _listMutex.get();
      _dirList.del(node);
      _listMutex.release();

      _releaseLock(node->meta.pid(), node->meta.name());  
      SAFE_OSS_DELETE(node);
   }
   
done:
   return rc;
}

INT32 dirMetaCache::_updateConnection()
{
   INT32 rc = SDB_OK;
   
   SAFE_OSS_DELETE(_dirDB);
   
   _dirDB = SDB_OSS_NEW fsConnectionDao(_ds);
   if(NULL == _dirDB)
   {
      rc = SDB_OOM;
      PD_LOG(PDERROR, "Fail to get connection, rc=%d", rc);
      goto error;
   }

   //TODO:待实现按需加锁放锁功能后再开启此事务
   /*
   rc = _dirDB->transBegin(TRUE);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transBegin. rc=%d", rc);
      goto error;
   }
   */

   rc = _dirDB->getCL(_dirCLName.c_str(), _dMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             _dirCLName.c_str(), rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done; 
}


void dirMetaCache::_releaseLock(INT64 parentId, const CHAR* dirName)
{
   INT32 rc = SDB_OK;
   BSONObj condition;

   if(_standalone)
   {
      return;
   }

   try 
   {
     condition = BSON(SEQUOIAFS_NAME<<dirName<<SEQUOIAFS_PID<<parentId);
   }
   catch(std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   
   _dirDBMutex.get();
   rc = _dirDB->releaseSLock(_dMetaCL, condition);
   if(rc != SDB_OK)
   {
      if(SDB_DMS_EOC != rc)
      {
         if(!_dirDB->isValid())
         {
            _updateConnection();
            releaseAllLocalCache(FALSE);
         }
      }
      _dirDBMutex.release();
      goto error;
   }
   _dirDBMutex.release();

done:
   return;
error:
   goto done;
}

