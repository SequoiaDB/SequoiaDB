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

   Source File Name = sequoiaFSDao.cpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          10/30/2020  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFSDao.hpp"

using namespace sequoiafs;
using namespace sdbclient;
using namespace bson;

fsConnectionDao::fsConnectionDao(sdbConnectionPool* dataSource)
{
   _ds = dataSource;
   _db = NULL;
}
fsConnectionDao::~fsConnectionDao()
{
   if(_db != NULL)
   {
      _ds->releaseConnection(_db);
      _db = NULL;
   }
}

INT32 fsConnectionDao::getFSConn(sdb** db)
{
   INT32 rc = SDB_OK;
   
   if(_db == NULL)
   {
      rc = _ds->getConnection(_db);
      if(SDB_OK != rc)
      {
         _db = NULL;
         PD_LOG(PDERROR, "Failed to get a connection, rc=%d", rc);
         goto error;
      }
   }

   *db = _db;

done:   
   return rc;

error:
   goto done;
}

INT32 fsConnectionDao::writeLob(const CHAR *buf,
                                const CHAR *clFullName,
                                const OID &lobId, 
                                INT64 offset,
                                INT32 len,
                                INT64 *lobSizeNew)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "writeLob(), oid:%s, offset:%ld, len:%d", 
                    lobId.toString().c_str(), offset, len);

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                       clFullName, rc);
      goto error;
   }

   rc = cl.openLob(lob, lobId, SDB_LOB_WRITE);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob for file, oid=%s, rc=%d",
                       lobId.toString().c_str(), rc);
      goto error;
   }
   
   rc = lob.lockAndSeek(offset, len);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to lockAndseek lob, oid=%s, offset:%ld, len:%d, rc=%d", 
                       lobId.toString().c_str(), offset, len, rc);
      goto error;
   }
   
   rc = lob.write(buf, len);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to write lob, oid:%s, offset:%ld, len:%d, rc=%d", 
                       lobId.toString().c_str(), offset, len, rc);
      goto error;
   }

   *lobSizeNew = lob.getSize();

done:
   lob.close();
   PD_LOG(PDDEBUG, "lob close, oid:%s", lobId.toString().c_str());
   return rc;
   
error:
   goto done;   
}

//create and write a lob
INT32 fsConnectionDao::writeNewLob(const CHAR *buf,
                                   const CHAR *clFullName,
                                   OID &lobId, 
                                   INT64 offset,
                                   INT32 len,
                                   INT64 &lobSizeNew)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "writeNewLob(), offset:%ld, len:%d", offset, len);

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                       clFullName, rc);
      goto error;
   }

   rc = cl.createLob(lob);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create lob for file, rc=%d", rc);
      goto error;
   }

   if(len > 0)
   {
      rc = lob.write(buf, len);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to write lob, oid:%s, offset:%ld, len:%d, rc=%d", 
                          lobId.toString().c_str(), offset, len, rc);
         goto error;
      }
   }

   lobSizeNew = lob.getSize(); 
   
   rc = lob.getOid(lobId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get lob oid, rc=%d", rc);
      goto error;
   }

   rc = lob.close();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to close lob, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   lob.close();
   goto done;   
}


INT32 fsConnectionDao::readLob(const CHAR *clFullName, 
                               const OID &lobId, 
                               INT64 offset, 
                               INT32 size, 
                               CHAR *buf, 
                               INT32 *len)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;
   UINT32 readlen = 0;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "readLob(), oid:%s, size:%d, offset:%ld", 
                    lobId.toString().c_str(), size, offset);

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             clFullName, rc);
      goto error;
   }

   //open lob according to the mode of open
   rc = cl.openLob(lob, lobId, SDB_LOB_SHAREREAD);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob for file, oid=%s, rc=%d",
                       lobId.toString().c_str(), rc);
      goto error;
   }

   rc = lob.lockAndSeek(offset, size);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to lockAndseek lob, oid=%s, rc=%d", 
                       lobId.toString().c_str(), rc);
      goto error;
   }
   
   rc = lob.read(size, buf, &readlen);
   if(SDB_EOF == rc)
   {
      PD_LOG(PDDEBUG, "Reach the end of the file, oid=%s, readlen=%d",
                       lobId.toString().c_str(), readlen);
      rc = SDB_OK;
   }
   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to read lob, oid=%s, rc=%d", 
                       lobId.toString().c_str(), rc);
      goto error;
   }

   *len = readlen;

done:
   lob.close();
   return rc;
   
error:
   goto done;      
}

INT32 fsConnectionDao::truncateLob(const CHAR *clFullName,
                                   const OID &lobId, 
                                   INT64 offset)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "truncateLob(), oid:%s, offset:%ld", 
                    lobId.toString().c_str(), offset);

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             clFullName, rc);
      goto error;
   }

   rc = cl.truncateLob(lobId, offset);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to truncate lob, rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;   
}

INT32 fsConnectionDao::removeLob(const CHAR *clFullName,
                                 const OID &lobId)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "removeLob(), oid:%s", lobId.toString().c_str());

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
      
   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             clFullName, rc);
      goto error;
   }

   rc = cl.removeLob(lobId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Remvoe lob failed, name=%s, rc=%d",
             lobId.toString().c_str(), rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done; 
}


INT32 fsConnectionDao::getLobSize(const CHAR *clFullName, 
                                  const OID &lobId, 
                                  SINT64 *len)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbLob lob;
   SINT64 lobSize = 0;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "getLobSize(), oid:%s", 
                    lobId.toString().c_str());

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(clFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             clFullName, rc);
      goto error;
   }

   //open lob according to the mode of open
   rc = cl.openLob(lob, lobId, SDB_LOB_SHAREREAD);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob for file, name=%s, rc=%d",
             lobId.toString().c_str(), rc);
      goto error;
   }
   
   rc = lob.getSize(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get size, rc=%d", rc);
      goto error;
   }
   
   *len = lobSize;
   PD_LOG(PDDEBUG, "getLobSize(), oid:%s, size:%d", 
                    lobId.toString().c_str(), lobSize);

done:
   lob.close();
   return rc;

error:
   goto done;      
}

INT32 fsConnectionDao::transBegin(BOOLEAN readonly)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->transactionBegin();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transactionBegin. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;  
}

INT32 fsConnectionDao::transCommit()
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->transactionCommit();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transactionCommit. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;  
}

INT32 fsConnectionDao::transRollback()
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   rc = db->transactionRollback();
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to transactionRollback. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;  
}

BOOLEAN fsConnectionDao::isValid()
{
   if(NULL == _db)
   {
      return FALSE;
   }

   if(! _db->isValid())
   {
      return FALSE;
   }

   return TRUE;
}

INT32 fsConnectionDao::releaseSLock(sdbCollection &cl, BSONObj &condition)
{
   INT32 rc = SDB_OK;

   return rc;
}

INT32 fsConnectionDao::getCS(const CHAR *pCollectionSpacelName,
                             sdbCollectionSpace &cs)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollectionSpace(pCollectionSpacelName, cs);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to getCollectionSpace(%s). rc=%d", pCollectionSpacelName, rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fsConnectionDao::getCL(const CHAR *pCollectionFullName,
                             sdbCollection &collection)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollection(pCollectionFullName, collection);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to getCollection. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fsConnectionDao::createCollectionSpace(const CHAR *pCollectionSpaceName,
                                             const bson::BSONObj &options, 
                                             sdbCollectionSpace &cs )
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = _db->createCollectionSpace(pCollectionSpaceName, options, cs);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to createCollectionSpace. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fsConnectionDao::createCollection(sdbCollectionSpace &cs,
                                        const CHAR *pCollectionName,
                                        sdbCollection &collection)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = cs.createCollection(pCollectionName, collection);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to createCollection. rc=%d", rc);
      goto error;
   }

done:
   return rc;
   
error:
   goto done;
}

INT32 fsConnectionDao::queryForUpdate(const CHAR *pCLFullName, 
                                      sdbCursor &cursor, 
                                      const bson::BSONObj &condition)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection cl;
   BSONObj bsonNull;

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollection(pCLFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
      goto error;
   }

   rc = cl.query(cursor, condition, bsonNull, bsonNull, bsonNull, 0, -1, QUERY_FOR_UPDATE);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
      goto error;
   }
   
done:
   return rc;
   
error:
   goto done;        
}

INT32 fsConnectionDao::insertMeta(const CHAR *pCLFullName,
                                  BSONObj &obj)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection cl;
   
   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollection(pCLFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
      goto error;
   }

   rc = cl.insert(obj);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to insert meta. cl:%s rc=%d", pCLFullName, rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}


INT32 fsConnectionDao::updateMeta(const CHAR *pCLFullName,
                                  BSONObj &condition, 
                                  BSONObj &rule,
                                  BSONObj &hint)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection cl;
   
   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollection(pCLFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
      goto error;
   }

   rc = cl.update(rule, condition, hint, 0, NULL);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to update meta. cl:%s rc=%d", pCLFullName, rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 fsConnectionDao::delMeta(const CHAR *pCLFullName,
                               const CHAR* name, 
                               INT64 parentid,
                               BSONObj &hint)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection cl;

   BSONObj condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<parentid);

   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }
   
   rc = db->getCollection(pCLFullName, cl);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to query and lock mcs service. rc=%d", rc);
      goto error;
   }
   
   rc = cl.del(condition, hint);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "del failed. name:%s, parentid:%d, rc=%d", name, parentid, rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

sdbCollection* fsConnectionDao::getDirCL(const CHAR *pCLFullName)
{
   INT32 rc = SDB_OK;
   
   if(NULL == _dirCL)
   {
      sdb *db = NULL;
      rc = getFSConn(&db);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
         goto error;
      }

      rc = db->getCollection(pCLFullName, *_dirCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                pCLFullName, rc);
         goto error;
      }
   }

done:
   return _dirCL;

error:
   goto done;
}

INT32 fsConnectionDao::queryMeta(sdbCollection &cl, 
                                 BSONObj &condition, 
                                 BSONObj &record,
                                 BOOLEAN lockS)
{
   INT64 rc = SDB_OK;
   sdbCursor cursor;
   BSONObj selected;
   BSONObj orderBy;
   BSONObj hint;
   INT32 flags = 0;

   rc = cl.query(cursor, condition, selected, orderBy, hint, 0, -1, flags);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query directory, rc=%d", rc);
      goto error;
   }

   rc = cursor.next(record);
   if(SDB_DMS_EOC == rc)
   {
      PD_LOG(PDINFO, "The meta does not exist, rc=%d", rc);
      goto error;
   }
   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, "
                      "rc=%d", rc);
      goto error;
   }

done:
   cursor.close();
   return rc;
error:
   goto done;
}

INT32 fsConnectionDao::findOne(const CHAR* clName, 
                               INT64 parentid, 
                               BSONObj &record)
{
   INT64 rc = SDB_OK;
   BSONObj condition;
   sdbCursor cursor;
   sdbCollection cl;
   sdb *db = NULL;
   
   rc = getFSConn(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
      goto error;
   }

   condition = BSON(SEQUOIAFS_PID<<(INT64)parentid);

   rc = db->getCollection(clName, cl, false);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             clName, rc);
      goto error;
   }

   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query directory, parentid=%d, rc=%d",
             parentid, rc);
      goto error;
   }

   rc = cursor.next(record);
   if(SDB_DMS_EOC == rc)
   {
      PD_LOG(PDERROR, "The meta does not exist, parentid=%d, rc=%d",
             parentid, rc);
      goto error;
   }
   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, "
                      "parentid=%d, rc=%d", parentid, rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

sdbCollection* fsConnectionDao::getFileCL(const CHAR *pCLFullName)
{
   INT32 rc = SDB_OK;
   
   if(NULL == _fileCL)
   {
      sdb *db = NULL;
      rc = getFSConn(&db);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get a connection. rc=%d", rc);
         goto error;
      }

      rc = db->getCollection(pCLFullName, *_fileCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                pCLFullName, rc);
         goto error;
      }
   }

done:
   return _fileCL;

error:
   goto done;
}

