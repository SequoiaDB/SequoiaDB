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

   Source File Name = sequoiaFS.cpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#include "sequoiaFS.hpp"
#include <sys/time.h>
#include <asm/ioctls.h>
#include <linux/errno.h>

#include "omagentDef.hpp"
#include "utilStr.hpp"
#include "ossVer.h"
#include "ossUtil.hpp"
#include "ossMem.hpp"

const string SEQUOIAFS_META_CS = "sequoiafs";
const string SEQUOIAFS_META_MAP_AUDIT_CL = "maphistory";
const string SEQUOIAFS_META_ID_CL = "sequenceid";
//const string SEQUOIAFS_MOUNT_ID_CL = "mountid";

const string SEQUOIAFS_META_MAP_AUDIT_CL_FULL = SEQUOIAFS_META_CS + "." +
                                                SEQUOIAFS_META_MAP_AUDIT_CL;
const string SEQUOIAFS_META_ID_CL_FULL = SEQUOIAFS_META_CS + "." +
                                         SEQUOIAFS_META_ID_CL;

#define SEQUOIAFS_SRC_CLNAME       "SourceCL"
#define SEQUOIAFS_DIR_META_CLNAME  "DirMetaCL"
#define SEQUOIAFS_FILE_META_CLNAME "FileMetaCL"
#define SEQUOIAFS_ADDRESS          "Address"
#define SEQUOIAFS_MOUNT_POINT      "MountPoint"
#define SEQUOIAFS_MOUNT_TIME       "MountTime"

#define SEQUOIAFS_SEQUENCEID       "Sequenceid"
#define SEQUOIAFS_MOUNTCLID        "Mountclid"
#define SEQUOIAFS_SEQUENCEID_VALUE "Value"

#define MAXCNT 1000
#define ENOIOCTLCMD 515

using namespace sdbclient;
using namespace sequoiafs;
using namespace bson;
using namespace engine;

const INT32 BUFSIZE=1000;

#define INIT_LOBHANDLE(lh) \
{\
   lh->flId = -1;\
   lh->isDirty = FALSE;\
   lh->parentId = 0;\
   lh->isCreate = FALSE;\
   lh->status = 2;\
   memset(lh->fileName, 0, FS_MAX_NAMESIZE + 1);\
}

INT32 getLocalIPs(string *localhosts)
{
   INT32 rc = SDB_OK;
   string hostStr;
   ossIPInfo ipInfo;
   if(ipInfo.getIPNum() > 0)
   {
      ossIP *ip = ipInfo.getIPs();
      for(INT32 i = ipInfo.getIPNum(); i > 0; i--)
      {
         if(0 != ossStrncmp(ip->ipAddr, OSS_LOOPBACK_IP,
                             ossStrlen(OSS_LOOPBACK_IP)))
         {
            hostStr = ip->ipName;
            hostStr += ":";
            hostStr += ip->ipAddr;
            hostStr += ";";
         }
         ip++;
      }
   }
   *localhosts = hostStr;

   return rc;
}

//get cs, it will create a new one if cs does not exist
INT32 getCollectionSpace(sdb &db, const CHAR *csName,
                         sdbCollectionSpace &cs)
{
   INT32 rc = SDB_OK;

   rc = db.getCollectionSpace(csName, cs);
   if(SDB_DMS_CS_NOTEXIST == rc)
   {
      rc = db.createCollectionSpace(csName, SDB_PAGESIZE_DEFAULT, cs);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collectionspace:%s. rc=%d", csName, rc ) ;
         goto error;
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collectionspace:%s. rc=%d", csName, rc ) ;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 getCollection(sdbCollectionSpace &cs, const CHAR *clName,
                    sdbCollection &cl, INT32 replsize)
{
   INT32 rc = SDB_OK;

   rc = cs.getCollection(clName, cl);
   if(SDB_DMS_NOTEXIST == rc)
   {
      try 
      {
         rc = cs.createCollection(clName, BSON("ReplSize" << replsize), cl);
         if(SDB_OK != rc)
         {
            PD_LOG( PDERROR, "Failed to create collection:%s. rc=%d", clName, rc ) ;
            goto error;
         }
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collection:%s. rc=%d", clName, rc ) ;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 listCollections(sdb &db)
{
   INT32 rc = SDB_OK;
   sdbCursor cursor;
   BSONObj clObj;

   rc=db.listCollections( cursor);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to list collection, rc=%d" OSS_NEWLINE, rc);
      goto error;
   }

   rc = cursor.next(clObj);
   while(SDB_DMS_EOC != rc)
   {
      if(SDB_OK != rc)
      {
          ossPrintf("Failed to get record in cursor, rc=%d" OSS_NEWLINE, rc);
          goto error;
      }
      else
      {
          rc = cursor.next(clObj);
      }
   }

   if(SDB_DMS_EOC == rc)
   {
      rc = SDB_OK;
   }

   cursor.close() ;

done:
   return rc;
error:
   cursor.close() ;
   goto done;
}

INT32 createIndex(sdbCollection &cl,
                  const CHAR *idxName,
                  const BSONObj &indexDef,
                  BOOLEAN isUnique=FALSE,
                  BOOLEAN isEnforced=FALSE)
{
   INT32 rc = SDB_OK;
   sdbCursor cursor;
   BSONObj record;

   //get the index if exist, or create index if do not exist
   rc = cl.getIndexes(cursor, idxName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get index, idx=%s, rc=%d",
             idxName, rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_IXM_NOTEXIST != rc && SDB_DMS_EOC != rc)
      {
          PD_LOG(PDERROR, "Error happened during do cursor current, idx=%s, "
                 "rc=%d", idxName, rc);
          goto error;
      }

      rc = cl.createIndex(indexDef, idxName, isUnique, isEnforced);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to create index, idx=%s, rc=%d",
                 idxName, rc);
          goto error;
      }
   }
done:
   return rc;
error:
   goto done;
}

sequoiaFS::sequoiaFS()
{
  _collection = "";
  _sysFileMetaCLFullName = "";
  _sysDirMetaCLFullName = "";
  _mountpoint = "";
  _replsize = SDB_SEQUOIAFS_REPLSIZE_DEFAULT_VALUE;
}

sequoiaFS::~sequoiaFS()
{
}

void sequoiaFS::fini()
{
   _running = FALSE;

   _fileCreatingMgr.fini();
   _fileLobMgr.fini();
   _metaCache.fini();
   closeDataSource();
   if(_thClean != NULL)
   {
      _thClean->join();
   }
}

sequoiafsOptionMgr * sequoiaFS::getOptionMgr()
{
   return &_optionMgr;
}

void sequoiaFS::setDataSourceConf(const CHAR* userName,
                                  const CHAR* passwd,
                                  const CHAR* cipherFile,
                                  const CHAR* token,
                                  const INT32 connNum)
{
   if(ossStrlen(cipherFile) > 0)
   {
      _conf.setAuthInfo(userName, cipherFile, token);
   }
   else 
   {
      _conf.setAuthInfo(userName, passwd);
   }
   _conf.setConnCntInfo(50, 10, 20, connNum);
   _conf.setCheckIntervalInfo( 60*1000, 0 );
   _conf.setSyncCoordInterval( 60*1000 );
   _conf.setConnectStrategy( SDB_CONN_STY_BALANCE );
   _conf.setValidateConnection( TRUE );
   _conf.setUseSSL( FALSE );
}

INT32 sequoiaFS::initDataSource(const CHAR *userName,
                                const CHAR *passwd,
                                const CHAR* cipherFile,
                                const CHAR* token,
                                const INT32 connNum)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: initCoonPool()");

   setDataSourceConf(userName, passwd, cipherFile, token, connNum);

   // init sdbConnectionPool
   getCoordHost();
   rc = _ds.init( _coordHostPort, _conf);
   if (SDB_OK != rc)
   {
      ossPrintf("Fail to init sdbDataSouce, rc=%d" OSS_NEWLINE, rc);
      goto error;
   }
   
done:
   return rc;

error:
   goto done;
}

void sequoiaFS::closeDataSource()
{
   PD_LOG(PDDEBUG, "Called: closeDataSource()");

   _ds.close(); 
}

INT32 sequoiaFS::getConnection(sdb **connection)
{
   INT32 rc = SDB_OK;

   rc = _ds.getConnection(*connection);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get a connection, rc=%d", rc);
      goto error;
   }

done:
   return rc;

error:
   goto done;
}

void sequoiaFS::getCoordHost()
{
   string hosts;
   size_t is_index = 0;

   hosts = _optionMgr.getHosts();

   for(;;)
   {
      is_index = hosts.find(',');
      if(is_index != std::string::npos)
      {
          _coordHostPort.push_back(hosts.substr(0, is_index));
          hosts = hosts.substr(is_index + 1);
      }
      else
      {
          _coordHostPort.push_back(hosts);
          break;
      }
   }
}

INT32 sequoiaFS::initMetaCSCL(sdb *db, const string csName,
                              const string clName,
                              const CHAR *idxName,
                              BOOLEAN createIndex,
                              const bson::BSONObj &indexDef,
                              BOOLEAN isUnique,
                              BOOLEAN isEnforced)
{
   INT32 rc = SDB_OK;
   sdbCursor cursor;
   sdbCollection cl;
   sdbCollection collection;
   sdbCollectionSpace cs;
   BSONObj idxObj;
   BSONObj record ;

   rc = getCollectionSpace(*db, csName.c_str(), cs);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = getCollection(cs, clName.c_str(), collection, replSize());
   if(SDB_OK != rc)
   {
      goto error;
   }

   if(!createIndex)
   {
      goto done;
   }

   //get the index if exist, or create index if do not exist
   rc = collection.getIndexes(cursor, idxName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get index, idx=%s, cl=%s, rc=%d",
             idxName, clName.c_str(), rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_IXM_NOTEXIST != rc && SDB_DMS_EOC != rc)
      {
          PD_LOG(PDERROR, "Error happened during do cursor current, idx=%s, "
                 "cl=%s, rc=%d", idxName, clName.c_str(), rc);
          goto error;
      }

      rc = collection.createIndex(indexDef, idxName, isUnique, isEnforced);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to create index, idx=%s, cl=%s, rc=%d",
                 idxName, clName.c_str(), rc);
          goto error;
      }
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::initDataCSCL(sdb *db, const string dataCollection)
{
   INT32 rc = SDB_OK;
   sdbCollection cl;
   sdbCollectionSpace cs;
   string csName;
   string clName;
   BSONObj option;

   rc = getOptionMgr()->parseCollection(dataCollection, &csName, &clName);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = getCollectionSpace(*db, csName.c_str(), cs);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = cs.getCollection(clName.c_str(), cl);
   if(SDB_DMS_NOTEXIST == rc)
   {
      try 
      {
         option = BSON("ReplSize" << replSize() << \
                       "AutoSplit" << true <<\
                       "ShardingKey" << BSON("_id" << 1)<<\
                       "ShardingType" << "hash");
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
      rc = cs.createCollection(clName.c_str(), option, cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collection:%s. rc=%d", clName.c_str(), rc ) ;
         goto error;
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collection:%s. rc=%d", clName.c_str(), rc ) ;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_getAndUpdateID(sdbCollection *cl, CHAR* name, INT64 *id)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   sdbCursor cursor;
   BSONObj record;

   try 
   {
      rule = BSON("$inc"<<BSON(SEQUOIAFS_SEQUENCEID_VALUE<<1));
      condition = BSON(SEQUOIAFS_NAME<<name);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = cl->queryAndUpdate(cursor, rule, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query and update sequenceid, rc=%d", rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, rc=%d", rc);
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SEQUENCEID_VALUE,
                       (void *)(id), NumberLong);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get value of sequoenceid, rc=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_initMetaID(sdb *db)
{
   INT32 rc = SDB_OK;
   INT32 ret = SDB_OK;
   BSONObj obj;
   sdbCollection cl;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   INT64 value = ROOT_ID;

   rc = db->getCollection(SEQUOIAFS_META_ID_CL_FULL.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                       SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      goto error;
   }
   
   try
   {
      condition = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_SEQUENCEID);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query collection, cl=%s, rc=%d",
                       SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC == rc)
      {
         try 
         {
            obj = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_SEQUENCEID<<\
                       SEQUOIAFS_SEQUENCEID_VALUE<<(INT64)(value + 1));
         }
         catch (std::exception &e)   
         {
            rc = SDB_DRIVER_BSON_ERROR;
            PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
            goto error; 
         }
         ret = cl.insert(obj);
         if(SDB_OK != ret)
         {
            PD_LOG(PDERROR, "Fail to insert to collection, cl=%s, rc=%d",
                             SEQUOIAFS_META_ID_CL_FULL.c_str(), ret);
            rc = ret;
            goto error;
         }
      }
      else
      {
         PD_LOG(PDERROR, "Error happened during cursor current, cl=%s, rc=%d",
                          SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
         goto error;
      }
   }

   if(SDB_DMS_EOC != rc)
   {
      rc = getRecordField(record, (CHAR *)SEQUOIAFS_SEQUENCEID_VALUE,
                          (void *)(&value), NumberLong);
      if(SDB_OK != rc)
      {
          goto error;
      }
   }

   rc = SDB_OK;

   if(value < ROOT_ID)
   {
      PD_LOG(PDERROR, "The Mountid of sequoiafs.sequenceid is %d, "
                      "should not small than %d", value, ROOT_ID);
      rc = -EINVAL;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_initMountID(sdb *db)
{
   INT32 rc = SDB_OK;
   INT32 ret = SDB_OK;
   BSONObj obj;
   sdbCollection cl;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   INT64 value = ROOT_ID;
   BSONObj emtpyObj;

   rc = initMetaCSCL(db, SEQUOIAFS_META_CS, SEQUOIAFS_META_ID_CL, "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create sequenceid collection, cs.cl=%s.%s, rc=%d", 
                       SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to create sequenceid collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                 SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = db->getCollection(SEQUOIAFS_META_ID_CL_FULL.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                       SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      goto error;
   }

   try 
   {
      condition = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_MOUNTCLID);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query collection, cl=%s, rc=%d",
             SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC == rc)
      {
         try 
         {
            obj = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_MOUNTCLID<<\
                       SEQUOIAFS_SEQUENCEID_VALUE<<(INT64)(value + 1));
         }
         catch (std::exception &e)   
         {
            rc = SDB_DRIVER_BSON_ERROR;
            PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
            goto error; 
         }     
         ret = cl.insert(obj);
         if(SDB_OK != ret)
         {
            PD_LOG(PDERROR, "Fail to insert to collection, cl=%s, rc=%d",
                             SEQUOIAFS_META_ID_CL_FULL.c_str(), ret);
            rc = ret;
            goto error;
         }
      }
      else
      {
          PD_LOG(PDERROR, "Error happened during cursor current, cl=%s, rc=%d",
                 SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
          goto error;
      }
   }

   if(SDB_DMS_EOC != rc)
   {
      rc = getRecordField(record, (CHAR *)SEQUOIAFS_SEQUENCEID_VALUE,
                          (void *)(&value), NumberLong);
      if(SDB_OK != rc)
          goto error;
   }

   rc = SDB_OK;

   if(value < ROOT_ID)
   {
      PD_LOG(PDERROR, "The sequenceid of sequoiafs.sequenceid is %d, "
             "should not small than %d", value, ROOT_ID);
      rc = -EINVAL;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_addMountID(sdb *db)
{
   INT32 rc = SDB_OK;
   INT32 ret = SDB_OK;
   BSONObj obj;
   sdbCollection cl;
   sdbCollection seqcl;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   BSONObj emtpyObj;
   INT64 id = 0L;
   string oldmountpoint;
   CHAR tempPath[OSS_MAX_PATHSIZE + 1] = {0};

   rc = initMetaCSCL(db, SEQUOIAFS_CS, SEQUOIAFS_MOUNTID_CL, "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create collection, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_CS, SEQUOIAFS_MOUNTID_CL, rc);
      ossPrintf("Failed to create sequenceid collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                SEQUOIAFS_CS, SEQUOIAFS_MOUNTID_CL, rc);
      goto error;
   }

   rc = db->getCollection(SEQUOIAFS_MOUNTID_FULLCL, cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             SEQUOIAFS_MOUNTID_FULLCL, rc);
      goto error;
   }

   try 
   {
      condition = BSON(FS_MOUNT_CL<<_collection.c_str());
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query collection, cl=%s, rc=%d",
             SEQUOIAFS_MOUNTID_FULLCL, rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC == rc)
      {
         ret = db->getCollection(SEQUOIAFS_META_ID_CL_FULL.c_str(), seqcl);
         if(SDB_OK != ret)
         {
            PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                   SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
            rc = ret;
            goto error;
         } 
   
         ret = _getAndUpdateID(&seqcl, SEQUOIAFS_MOUNTCLID, &id);
         if(SDB_OK != ret)
         {
            PD_LOG(PDERROR, "Fail to get and update id in sequence collecion, "
                   "rc=%d", ret);
            rc = ret;
            goto error;
         }

         try 
         {
            obj = BSON(FS_MOUNT_CL<<_collection.c_str()<<\
                       FS_MOUNT_PATH<<_mountpoint.c_str()<<\
                       FS_MOUNT_ID<<(id));
         }
         catch (std::exception &e)   
         {
            rc = SDB_DRIVER_BSON_ERROR;
            PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
            goto error; 
         }
         ret = cl.insert(obj);
         if(SDB_OK != ret)
         {
            PD_LOG(PDERROR, "Fail to insert to collection, cl=%s, rc=%d",
                   SEQUOIAFS_META_ID_CL_FULL.c_str(), ret);
            rc = ret;
            goto error;
         }

         rc = SDB_OK;
      }
      else
      {
         PD_LOG(PDERROR, "Fail to query collection, cl=%s, rc=%d",
             SEQUOIAFS_MOUNTID_FULLCL, rc);
         goto error;
      }
   }
   else
   {
      if(!_optionMgr.getForceMount())
      {
         //check mountpath is same with mountid cl
         BSONObjIterator itr(record);
         while (itr.more())
         {
            BSONElement ele = itr.next();
            if (0 == ossStrcmp(FS_MOUNT_PATH, ele.fieldName()))
            {
               PD_CHECK(String == ele.type(), SDB_INVALIDARG,
                     error, PDERROR, "The type of field:%s is not string",
                     FS_MOUNT_PATH);

               oldmountpoint = ele.valuestrsafe();
               if(NULL != ossGetRealPath(ele.valuestrsafe(), tempPath, OSS_MAX_PATHSIZE))
               {
                  oldmountpoint = tempPath;
               }

               if(_mountpoint != oldmountpoint)
               {
                  PD_LOG(PDERROR, "The mountpoint must be the same as an "
                                  "existing mountpoint using the same collection,"
                                  " existing mountpoint=%s, new mountpoint=%s",
                                  ele.valuestrsafe(), _mountpoint.c_str());
                  ossPrintf("The mountpoint must be the same as the "
                            "existing mountpoint using the same collection,"
                            " existing mountpoint=%s, new mountpoint=%s. exit." OSS_NEWLINE,
                            ele.valuestrsafe(), _mountpoint.c_str());                
                  rc = SDB_INVALIDARG;
                  goto error;
               }
               break;
            }
         }
      }
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_initRootPath(sdb *db)
{
   INT32 rc = SDB_OK;
   sdbCollection sysDirMetaCL;
   INT64 pid = 0;
   INT64 id = ROOT_ID;
   struct timeval tval;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   BSONObj options;
   INT32 pageSize = getpagesize();
   dirMeta newDirMeta;

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   gettimeofday(&tval, NULL);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   mtime = ctime;

   newDirMeta.setName("/");
   newDirMeta.setMode(S_IFDIR | 0755);
   newDirMeta.setUid(uid);
   newDirMeta.setGid(gid);
   newDirMeta.setNLink(2);
   newDirMeta.setPid(pid);
   newDirMeta.setId(id);
   newDirMeta.setSize(pageSize);
   newDirMeta.setCtime(ctime);
   newDirMeta.setMtime(mtime);
   newDirMeta.setAtime(mtime);
   newDirMeta.setSymLink( NULL );

   rc = _doSetDirNodeAttr(sysDirMetaCL, newDirMeta);
   if(SDB_OK != rc)
   {
      if (SDB_IXM_DUP_KEY == rc)
      {
         rc = SDB_OK;
      }
      else
      {
         PD_LOG(PDERROR, "Failed to set attr, rc=%d", rc);
         goto error;
      }
   }

done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::writeMapHistory(const CHAR *hosts)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   BSONObj obj;
   BSONObj timeObj;
   sdbCollection cl;
   BSONObjBuilder builder;
   ossTimestamp time;
   CHAR timestampStr[OSS_TIMESTAMP_STRING_LEN + 1] = {0};
   string localHosts;

   rc=getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = db->getCollection(SEQUOIAFS_META_MAP_AUDIT_CL_FULL.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             SEQUOIAFS_META_MAP_AUDIT_CL_FULL.c_str(), rc);
      goto error;
   }

   ossGetCurrentTime(time);
   ossTimestampToString(time, timestampStr);
   builder.append(SEQUOIAFS_MOUNT_TIME, timestampStr);
   timeObj = builder.obj();

   rc = getLocalIPs(&localHosts);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get local ips, rc=%d", rc);
      goto error;
   }

   try 
   {
      obj = BSON(SEQUOIAFS_SRC_CLNAME<<_collection<<\
                 SEQUOIAFS_DIR_META_CLNAME<<_sysDirMetaCLFullName<<\
                 SEQUOIAFS_FILE_META_CLNAME<<_sysFileMetaCLFullName<<\
                 SEQUOIAFS_ADDRESS<<localHosts<<\
                 SEQUOIAFS_MOUNT_POINT<<_mountpoint<<\
                 SEQUOIAFS_MOUNT_TIME<<timeObj);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = cl.insert(obj);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   releaseConnection(db);
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::buildDialogPathStartPD()
{
   INT32 rc = SDB_OK;
   CHAR diaglogPath[OSS_MAX_PATHSIZE + 1] = {0};
   CHAR verText[OSS_MAX_PATHSIZE + 1] = {0};
   string configs;

   rc = buildDialogPath(diaglogPath, _optionMgr.getDiaglogPath(), OSS_MAX_PATHSIZE + 1);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to build dialog path(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   // ./log/guestdir/diaglog/sequoiafs.log
   rc = engine::utilCatPath(diaglogPath, OSS_MAX_PATHSIZE,
                            SDB_SEQUOIAFS_LOG_FILE_NAME);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to build dialog path(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   sdbEnablePD(diaglogPath, _optionMgr.getDiagMaxNUm());
   setPDLevel((PDLEVEL(_optionMgr.getDiaglogLevel())));
   ossSprintVersion("Version", verText, OSS_MAX_PATHSIZE, FALSE);
   PD_LOG(((getPDLevel()>PDEVENT)?PDEVENT:getPDLevel()),
           "Start sequoiafs[%s]...", verText);

   //print configuration in log file
   //_optionMgr.toString(configs);
   //PD_LOG(PDEVENT, "ALL configs:\n%s", configs.c_str());

done:
   return rc;
error:
   goto done;  
}

INT32 sequoiaFS::init()
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;  
   BSONObj idxDefObj;
   BSONObj emtpyObj;   
   const CHAR *nameIdx = "NameIndex";
   const CHAR *idIdx = "IdIndex";
   const CHAR *pidIdx = "PidIndex";
   const CHAR *lobOidIdx = "LobOidIndex";
   
   //string configs;
   BSONObj options;
   dirMeta dMeta;
   
   setReplSize(_optionMgr.replsize());

   //3. init datasource
   rc= initDataSource(_optionMgr.getUserName(), 
                      _optionMgr.getPasswd(), 
                      _optionMgr.getCipherFile(),
                      _optionMgr.getToken(),
                      _optionMgr.getConnNum());
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to init connection pool, rc=%d", rc ) ;
      ossPrintf("Failed to init connection pool(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   //4. init data/meta cs and cl
   rc=getConnection(&db);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to get a connection(rc=%d), exit." OSS_NEWLINE, rc);
      goto error;
   }

   _collection = _optionMgr.getCollection();
   _mountpoint = _optionMgr.getmountpoint();
   rc = initDataCSCL(db, _collection);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init collection, cs.cl=%s, rc=%d", 
             _collection.c_str(), rc);
      ossPrintf("Failed to init collection, cs.cl=%s, rc=%d, exit." OSS_NEWLINE,
                _collection.c_str(), rc);
      goto error;
   }

   _sysFileMetaCLFullName = _optionMgr.getMetaFileCL();
   _sysDirMetaCLFullName = _optionMgr.getMetaDirCL();

   rc = _optionMgr.parseCollection(_sysDirMetaCLFullName, &_sysDirMetaCSName,
                                   &_sysDirMetaCLName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse dir meta collection, cs.cl=%s, rc=%d", 
             _sysDirMetaCLFullName.c_str(), rc);
      ossPrintf("Failed to parse dir meta collection, cs.cl=%s, rc=%d, exit." OSS_NEWLINE,
                _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = _optionMgr.parseCollection(_sysFileMetaCLFullName,
                                   &_sysFileMetaCSName, &_sysFileMetaCLName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse file meta collection, cs.cl=%s, rc=%d", 
             _sysFileMetaCLFullName.c_str(), rc);
      ossPrintf("Failed to parse file meta collection, cs.cl=%s, rc=%d, exit." OSS_NEWLINE,
                _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_NAME << 1 << SEQUOIAFS_PID << 1);
      rc = initMetaCSCL(db, _sysDirMetaCSName, _sysDirMetaCLName, nameIdx,
                        TRUE, idxDefObj, TRUE, TRUE);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create nameIdx for dir meta collection, cs.cl=%s.%s, rc=%d", 
                _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         ossPrintf("Failed to init dir meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_ID << 1);
      rc = initMetaCSCL(db, _sysDirMetaCSName, _sysDirMetaCLName, idIdx,
                        TRUE, idxDefObj, TRUE, TRUE);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create idIdx for dir meta collection, cs.cl=%s.%s, rc=%d", 
                _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         ossPrintf("Failed to create idIdx for dir meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_PID << 1);
      rc = initMetaCSCL(db, _sysDirMetaCSName, _sysDirMetaCLName, pidIdx,
                        TRUE, idxDefObj);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create pidIdx for dir meta collection, cs.cl=%s.%s, rc=%d", 
                _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         ossPrintf("Failed to create pidIdx for dir meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   rc = _initRootPath(db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init root path, rc=%d", rc);
      ossPrintf("Failed to init root path, rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_NAME << 1 << SEQUOIAFS_PID << 1);
      rc = initMetaCSCL(db, _sysFileMetaCSName, _sysFileMetaCLName, nameIdx,
                        TRUE, idxDefObj, TRUE, TRUE);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create nameIdx for file meta collection, cs.cl=%s.%s, rc=%d", 
                _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         ossPrintf("Failed to init create nameIdx for meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_LOBOID << 1);
      rc = initMetaCSCL(db, _sysFileMetaCSName, _sysFileMetaCLName, lobOidIdx,
                        TRUE, idxDefObj);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create lobidIdx for file meta collection, cs.cl=%s.%s, rc=%d", 
                _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         ossPrintf("Failed to init create lobidIdx for meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   try
   {
      idxDefObj = BSON(SEQUOIAFS_PID << 1);
      rc = initMetaCSCL(db, _sysFileMetaCSName, _sysFileMetaCLName, pidIdx,
                        TRUE, idxDefObj);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to create pidIdx for file meta collection, cs.cl=%s.%s, rc=%d", 
                _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         ossPrintf("Failed to create pidIdx for file meta collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                   _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
         goto error;
      }
   }
   catch (std::exception &e)
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      goto error;
   }

   rc = initMetaCSCL(db, SEQUOIAFS_META_CS, SEQUOIAFS_META_MAP_AUDIT_CL,
                     "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create maphistory collection, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_MAP_AUDIT_CL.c_str(), rc);
      ossPrintf("Failed to create maphistory collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_MAP_AUDIT_CL.c_str(), rc);
      goto error;
   }

   rc = initMetaCSCL(db, SEQUOIAFS_META_CS, SEQUOIAFS_META_ID_CL, "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create sequenceid collection, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to create sequenceid collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = _initMetaID(db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init sequenceid collection, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to init sequenceid collection, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = _initMountID(db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init mountid, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to init mountid, cs.cl=%s.%s, rc=%d, exit." OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = _addMountID(db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to add mountid, cs.cl=%s.%s, rc=%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = _fileCreatingMgr.init(this,
                              &_ds, 
                              _optionMgr.getCollection(), 
                              _optionMgr.getMetaFileCL(),
                              _optionMgr.isfileCreateCache(),
                              _optionMgr.getCreateFilePath(), 
                              _optionMgr.getfileCreateCacheSize());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init fileCreatingMgr. rc=%d", rc);
      ossPrintf("Failed to init fileCreatingMgr. rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }

   rc = _fileLobMgr.init(&_ds, 
                         _optionMgr.getFflag(), 
                         _optionMgr.getPreReadBlock(),
                         _optionMgr.getDataCacheSize());  
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init fileLobMgr. rc=%d", rc);
      ossPrintf("Failed to init fileLobMgr. rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }

   rc = _metaCache.init(&_ds, 
                        _sysDirMetaCLFullName, 
                        _sysFileMetaCLFullName, 
                        _collection, 
                        _mountpoint, 
                        _optionMgr.getDirCacheSize(),
                        _optionMgr.getStandAlone());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init metaCache. rc=%d", rc);
      ossPrintf("Failed to init metaCache. rc=%d, exit." OSS_NEWLINE, rc);
      goto error;
   }

   _running = TRUE;
   try
   {
      _thClean = new boost::thread( boost::bind( &sequoiaFS::_cleanCounts, this ) );
   }
   catch ( std::exception &e )
   {
      PD_LOG(PDERROR, "Exception[%s] occurs.", e.what());
      rc = SDB_SYS ;
      goto error ;
   }

done:
   releaseConnection(db);
   return rc;

error:
   goto done; 
}

INT32 sequoiaFS::getRecordField(BSONObj &record,
                                CHAR *fieldName,
                                void *value, BSONType type)
{
   INT32 rc = SDB_OK;
   BSONElement ele;
   BSONType bType;

   ele = record.getField(fieldName);
   bType = ele.type();
   switch(type)
   {
   //time,size
   case NumberLong:
      if(NumberLong == bType)
      {
          *((SINT64 *)value) = (SINT64)(ele.numberLong());
      }
      else
      {
          PD_LOG(PDERROR, "The type of field:%s is not NumberLong", fieldName);
          rc = SDB_INVALIDARG;
          goto error;
      }
      break;
   //uid
   case NumberInt:
      if(NumberInt == bType)
      {
          *((INT32 *)value) = (INT32)(ele.numberInt());
      }
      else
      {
          PD_LOG(PDERROR, "The type of field:%s is not NumberInt", fieldName);
          rc = SDB_INVALIDARG;
          goto error;
      }
      break;
   case String:
      if(String == bType)
      {
          *((string *)value) = ele.String();
      }
      else
      {
          PD_LOG(PDERROR, "The type of field:%s is not String", fieldName);
          rc = SDB_INVALIDARG;
          goto error;
      }
      break;
      
   default:
      break;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::doUpdateAttr(sdbCollection *cl,
                              const BSONObj &rule,
                              const BSONObj &condition,
                              const BSONObj &hint)
{
   INT32 rc = SDB_OK;

   rc = cl->update(rule, condition, hint);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, rc=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_doSetFileNodeAttr(sdbCollection &cl,
                                    fileMeta &fileNode)
{
   INT32 rc = SDB_OK;
   BSONObj obj;

   try 
   {
      obj = BSON(SEQUOIAFS_NAME<<fileNode.name()<<\
                 SEQUOIAFS_MODE<<fileNode.mode()<<\
                 SEQUOIAFS_UID<<fileNode.uid()<<\
                 SEQUOIAFS_GID<<fileNode.gid()<<\
                 SEQUOIAFS_NLINK<<fileNode.nLink()<<\
                 SEQUOIAFS_PID<<fileNode.pid()<<\
                 SEQUOIAFS_LOBOID<<fileNode.lobOid()<<\
                 SEQUOIAFS_SIZE<<fileNode.size()<<\
                 SEQUOIAFS_CREATE_TIME<<fileNode.ctime()<<\
                 SEQUOIAFS_MODIFY_TIME<<fileNode.mtime()<<\
                 SEQUOIAFS_ACCESS_TIME<<fileNode.atime()<<\
                 SEQUOIAFS_SYMLINK<<fileNode.symLink());
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   
   rc = cl.insert(obj);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::_doSetDirNodeAttr(sdbCollection &cl, dirMeta &dirNode)
{
   INT32 rc = SDB_OK;
   BSONObj obj;

   try 
   {
      obj = BSON(SEQUOIAFS_NAME<<dirNode.name()<<\
             SEQUOIAFS_MODE<<dirNode.mode()<<\
             SEQUOIAFS_UID<<dirNode.uid()<<\
             SEQUOIAFS_GID<<dirNode.gid()<<\
             SEQUOIAFS_PID<<dirNode.pid()<<\
             SEQUOIAFS_ID<<dirNode.id()<<\
             SEQUOIAFS_NLINK<<dirNode.nLink()<<\
             SEQUOIAFS_SIZE<<dirNode.size()<<\
             SEQUOIAFS_CREATE_TIME<<dirNode.ctime()<<\
             SEQUOIAFS_MODIFY_TIME<<dirNode.mtime()<<\
             SEQUOIAFS_ACCESS_TIME<<dirNode.atime()<<\
             SEQUOIAFS_SYMLINK<<dirNode.symLink());
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   
   rc = cl.insert(obj);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::getattr(const CHAR *path, struct stat *sbuf)
{
   INT32 rc = SDB_OK;
   INT32 blocks;
   INT32 pageSize = getpagesize();
   INT64 parentId = 1;
   string basePath;
   CHAR *fileName = NULL;
   dirMeta  dMeta;
   fileMeta fMeta;
   metaNode* meta = NULL;

   PD_LOG(PDDEBUG, "Called: getattr(). Path:%s", path);
   ossMemset(sbuf, 0, sizeof(struct stat));

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
                       path, rc);
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   
   rc = _fileCreatingMgr.query(parentId, fileName, fMeta);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC != rc)
      {
         PD_LOG(PDERROR, "query file info from filecreatingmgr failed. path=%s, rc=%d", 
                          path, rc);
         goto error;
      }
   }
   else 
   {
      meta = &fMeta;
   }

   if(NULL == meta)
   {
      rc = _metaCache.getDirInfo(parentId, fileName, dMeta);
      if(SDB_OK != rc)
      {
         if(SDB_DMS_EOC != rc)
         {
            PD_LOG(PDERROR, "query dir info from metacache failed. path=%s, rc=%d", path, rc);
            goto error;
         }
      }
      else 
      {
         meta = &dMeta;
      }
   }

   if(NULL == meta)
   {
      rc = _metaCache.getFileInfo(parentId, fileName, fMeta);
      if(SDB_OK != rc)
      {
         if(SDB_DMS_EOC != rc)
         {
            PD_LOG(PDERROR, "query file info from db failed. path=%s, rc=%d", path, rc);
            goto error;
         }
      }
      else 
      {
         meta = &fMeta;
      }
   }

   if(NULL == meta)
   {
      rc = SDB_DMS_EOC;
      goto error;
   }
   
   sbuf->st_uid = meta->uid();
   sbuf->st_gid = meta->gid();
   sbuf->st_size = meta->size();
   sbuf->st_ctime = meta->ctime() / 1000;
   sbuf->st_atime = meta->atime() / 1000;
   sbuf->st_mtime = meta->mtime() / 1000;
   sbuf->st_mode = meta->mode();
   sbuf->st_nlink = meta->nLink();
   
   blocks = sbuf->st_size / pageSize;
   blocks = (sbuf->st_size % pageSize == 0) ? blocks : blocks + 1;
   sbuf->st_blocks = blocks * 8;

done:
   PD_LOG(PDDEBUG, "getattr() rc=%d.", rc);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::readlink(const CHAR *path, CHAR * link, size_t size)
{
   INT32 rc = SDB_OK;
   string linkName;
   INT64 parentId = 1;
   string basePath;
   _fileMeta fMeta;

   PD_LOG(PDDEBUG, "Called: readlink(), path:%s", path);

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   rc = _fileCreatingMgr.query(parentId, (CHAR*)basePath.c_str(), fMeta);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC != rc)
      {
         PD_LOG(PDERROR, "query file info from filecreatingmgr failed." 
                         "path=%s, rc=%d", path, rc);
         goto error;
      }
      
      rc = _metaCache.getFileInfo(parentId, (CHAR*)basePath.c_str(), fMeta);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Fail to query file, name=%s, rc=%d", 
                          basePath.c_str(), rc);
         goto error;
      }
   }

   ossMemcpy(link, fMeta.symLink(), ossStrlen(fMeta.symLink()));

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

/*Deprecated, use readdir() instead*/
INT32 sequoiaFS::getdir(const CHAR *path,
                        fuse_dirh_t dirh,
                        fuse_dirfil_t dirfill)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDDEBUG, "Called: getdir(), path:%s", path);
   goto error;

done:
   return rc;
error:
   goto done;
}

/** Create a file node
 *
 * This is called for creation of all non-directory, non-symlink
 * nodes.  If the filesystem defines a create() method, then for
 * regular files that will be called instead.
 */

INT32 sequoiaFS::mknod(const CHAR *path, mode_t mode, dev_t dev)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDDEBUG, "Called: mknod(), path:%s", path);
   goto error;

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::mkdir(const CHAR *path, mode_t mode, struct fuse_context *context)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection sequenceCl;
   sdbCollection sysDirMetaCL;
   UINT64 ctime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct timeval tval;
   INT64 id = 0;
   INT64 parentId = 0;
   string basePath;
   BSONObj options;
   dirMeta newDirMeta;
   fsConnectionDao fsDao(&_ds);

   if(context != NULL)
   {
      uid = context->uid;
      gid = context->gid;
   }

   PD_LOG(PDDEBUG, "Called: mkdir(), path:%s, mode:%u", path, mode);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = db->getCollection(SEQUOIAFS_META_ID_CL_FULL.c_str(), sequenceCl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, rc=%d",
             SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      goto error;
   }

   rc = _getAndUpdateID(&sequenceCl, SEQUOIAFS_SEQUENCEID, &id);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get and update id in sequence collecion, "
             "rc=%d", rc);
      goto error;
   }

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }
   
   gettimeofday(&tval, NULL);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;

   newDirMeta.setName(basePath.c_str());
   newDirMeta.setMode(S_IFDIR | mode);
   newDirMeta.setUid(uid);
   newDirMeta.setGid(gid);
   newDirMeta.setNLink(2);
   newDirMeta.setPid(parentId);
   newDirMeta.setId(id);
   newDirMeta.setSize(4096);
   newDirMeta.setCtime(ctime);
   newDirMeta.setMtime(ctime);
   newDirMeta.setAtime(ctime);
   newDirMeta.setSymLink( NULL );

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transBegin, rc=%d", rc);
      goto error;
   }

   rc = _doSetDirNodeAttr(sysDirMetaCL, newDirMeta);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to insert attr, path:%s, parentId:%d, name:%s, rc=%d", path, basePath.c_str(), rc);
      goto error;
   }

   rc = _metaCache.incDirLink(&fsDao, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to inc parent dir Link, id:%d, rc=%d", parentId, rc);
      goto error;    
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   releaseConnection(db);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::unlink(const CHAR *path)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection cl;
   OID oid;
   BSONObj condition;
   BSONObj record;
   BSONObj rule;
   UINT32 newmode = 0;
   string lobOid;
   string basePath;
   INT64 parentId = 0;
   BSONObj options;
   _fileMeta fileMeta;
   BOOLEAN isProcessed = FALSE;

   PD_LOG(PDDEBUG, "Called: unlink(), path:%s", path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get connection, rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _collection.c_str(), rc);
      goto error;
   }
   
   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   rc = _fileCreatingMgr.unlink(parentId, (CHAR *)basePath.c_str(), isProcessed);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to unlink from fileceatingMgr, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   if(isProcessed)
   {
      goto done;
   }
   
   fileName = (CHAR *)basePath.c_str();
   rc = _metaCache.getFileInfo(parentId, fileName, fileMeta);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get file, parentId=%d, name=%s, rc=%d", 
                       parentId, fileName, rc);
      goto error;
   }

   try 
   {
      condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)parentId);
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc =  sysFileMetaCL.del(condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to delete file, name=%s, rc=%d", fileName, rc);
      goto error;
   }

   //is link file
   if(S_ISLNK(fileMeta.mode()))
   {
      try 
      {
         rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<-1));
         newmode = S_IFREG | 0644;
         //TODO: newnode=0644?  if newnode!= 0644 or loboid is null, check if canbe delete
         condition = BSON("$and"<<BSON_ARRAY(BSON(SEQUOIAFS_LOBOID<<fileMeta.lobOid())<<\
                          BSON(SEQUOIAFS_MODE<<newmode)));
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
      rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
      if(SDB_OK != rc)
      {
          goto error;
      }
   }

   //is regular file
   if(S_ISREG(fileMeta.mode()))
   {
      if(fileMeta.nLink() > 1)
      {
         try 
         {
            condition = BSON(SEQUOIAFS_LOBOID<<fileMeta.lobOid());
            rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<-1));
         }
         catch (std::exception &e)   
         {
            rc = SDB_DRIVER_BSON_ERROR;
            PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
            goto error; 
         }
          rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
          if(SDB_OK != rc)
          {
             PD_LOG(PDERROR, "Failed to update attr, rc=%d", rc);
             goto error;
          }
          goto done;
      }

      oid = bson::OID(fileMeta.lobOid());

      rc = cl.removeLob(oid);
      //if lob did not exist, ignore the error
      if(SDB_FNE == rc)
      {
          PD_LOG(PDWARNING, "Lob for file has alread removed, filename=%s, "
                 "oid:%s, rc=%d", fileName, fileMeta.lobOid(), rc);
          rc = SDB_OK;
      }
      else if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to remove lob for file, filename=%s, oid:%s, "
                 "rc=%d", fileName, fileMeta.lobOid(), rc);
          rc = SDB_OK;  // lodb delete failed, return success , lob residue 
      }
   }

   PD_LOG(PDDEBUG, "Finish to unlink file:%s", path);
   
done:
   releaseConnection(db);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::rmdir(const CHAR *path)
{
   INT32 rc = SDB_OK;
   INT64 parentId = 0;
   string basePath;
   INT64 id;
   BOOLEAN is_empty = TRUE;
   dirMeta pDirMeta;
   fsConnectionDao fsDao(&_ds);
   BOOLEAN isTransBegin = FALSE;
   
   PD_LOG(PDDEBUG, "Called: rmdir(), path:%s", path);

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   if(basePath == "/")
   {
      id = 1;
   }
   else
   {
      //get the ino of the dir
      rc = _metaCache.getDirInfo(parentId, basePath.c_str(), pDirMeta);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to query collection, cl=%s, rc=%d",
                _sysDirMetaCLFullName.c_str(), rc);
         goto error;
      }
      id = pDirMeta.id();
   }

   if(!_fileCreatingMgr.isDirEmpty(parentId))
   {
      PD_LOG(PDERROR, "Failed to remove dir:%s, directory not empty",
             basePath.c_str());
      rc = SDB_DIR_NOT_EMPTY;
      goto error;
   }
   
   rc = _metaCache.isDirEmpty(id, &is_empty);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, rc=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   if(!is_empty)
   {
      PD_LOG(PDERROR, "Failed to remove dir:%s, directory not empty",
             basePath.c_str());
      rc = SDB_DIR_NOT_EMPTY;
      goto error;
   }

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transBegin, rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   rc = _metaCache.delDir(&fsDao, parentId, (CHAR*)basePath.c_str());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, rc=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = _metaCache.decDirLink(&fsDao, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to decDirLink, id=%d, rc=%d",
                       parentId, rc);
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
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::symlink(const CHAR *path, const CHAR *link, struct fuse_context *context)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *linkName = NULL;
   sdbCollection sysFileMetaCL;
   OID oid;
   _fileMeta fileNode;
   BSONObj record;
   BSONObj rule;
   BSONObj obj;
   UINT64 ctime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct timeval tval;
   INT64 parentId = 1;
   string basePath;
   _fileMeta fMeta;

   if(context != NULL)
   {
      uid = context->uid;
      gid = context->gid;
   }

   PD_LOG(PDDEBUG, "Called: symlink(), path:%s, link:%s", path, link);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get connection, rc=%d", rc);
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = _metaCache.getParentIdName(link, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             link, rc);
      goto error;
   }
   
   linkName = (CHAR *)basePath.c_str();
   rc = _fileCreatingMgr.query(parentId, linkName, fMeta);
   if(SDB_OK == rc)
   {
      PD_LOG(PDERROR, "Failed to create symlink:%s, it does exist", linkName);
      rc = SDB_FE;
      goto error;
   }
   else if(SDB_DMS_EOC != rc)
   {
      PD_LOG(PDERROR, "Failed to query file info from filecreatingmgr, name=%s, rc=%d", 
                       linkName, rc);
      goto error;
   }

   rc = _metaCache.getFileInfo(parentId, linkName, fMeta);
   if(SDB_OK == rc)
   {
      PD_LOG(PDERROR, "Failed to create symlink:%s, it does exist", linkName);
      rc = SDB_FE;
      goto error;
   }
   else if(SDB_DMS_EOC != rc)
   {
      PD_LOG(PDERROR, "Failed to get file, name=%s, rc=%d", linkName, rc);
      goto error;
   }

   ossGetTimeOfDay(&tval);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   fileNode.setName(linkName);
   fileNode.setMode(S_IFLNK | 0777);
   fileNode.setNLink(1);
   fileNode.setPid(parentId);
   fileNode.setLobOid("");
   fileNode.setCtime(ctime);
   fileNode.setMtime(ctime);
   fileNode.setAtime(ctime);
   fileNode.setUid(uid);
   fileNode.setGid(gid);
   fileNode.setSize(ossStrlen(path));
   fileNode.setSymLink(path);

   rc = _doSetFileNodeAttr(sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to insert symlink, rc=%d", rc);
      goto error;
   }

done:
   releaseConnection(db);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::rename(const CHAR *path, const CHAR *newpath)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *name = NULL;
   CHAR *newName = NULL;
   string basePath;
   string newbasePath;
   INT64 parentId =0;
   INT64 newParentId = 0;
   BOOLEAN is_dir = true;
   _fileMeta fMeta;
   fsConnectionDao fsDao(&_ds);
   BOOLEAN isTransBegin = FALSE;

   PD_LOG(PDDEBUG, "Called: rename(), path:%s, newpath:%s", path, newpath);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = _metaCache.getParentIdName(newpath, &newbasePath, newParentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             newpath, rc);
      goto error;
   }

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }
   
   name = (CHAR *)basePath.c_str();
   newName = (CHAR *)newbasePath.c_str();
   rc = _fileCreatingMgr.flushfile(newParentId, newName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, newpath=%s, rc=%d", 
                       newpath, rc);
      goto error;
   }

   rc = _fileCreatingMgr.flushfile(parentId, name);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, path=%s, rc=%d", 
                       path, rc);
      goto error;
   }
   
   rc = _metaCache.isDir(parentId, name, &is_dir);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   //if it is a file, check the newfile exists or not, if exist, should delete it first
   if(!is_dir)
   {
      rc = _metaCache.getFileInfo(newParentId, newName, fMeta);
      if(SDB_OK == rc)
      {
         rc = unlink(newpath);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to remove file, name=%s, rc=%d", newpath, rc);
            goto error;
         }
      }
      else if(SDB_DMS_EOC != rc)
      {
         PD_LOG(PDERROR, "Failed to query file, name=%s, rc=%d", newName, rc);
         goto error;
      }
   }

   rc = fsDao.transBegin();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transBegin, rc=%d", rc);
      goto error;
   }
   isTransBegin = TRUE;

   rc = _metaCache.renameEntity(&fsDao, 
                                parentId, name, newParentId, newName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   if(is_dir && parentId != newParentId)
   {
      rc = _metaCache.incDirLink(&fsDao, newParentId);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to incDirLink, newid=%d, rc=%d",
                          newParentId, rc);
         goto error;
      }

      rc = _metaCache.decDirLink(&fsDao, parentId);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to decDirLink, id=%d, rc=%d",
                          parentId, rc);
         goto error;
      }
   }

   rc = fsDao.transCommit();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to transCommit, rc=%d", rc);
      goto error;
   }

done:
   releaseConnection(db);
   return rc;

error:
   if(isTransBegin)
   {
      fsDao.transRollback();
   }
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::link(const CHAR *path, const CHAR *newpath)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *linkName = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   OID oid;
   BSONObj condition;
   BSONObj rule;
   BSONObj obj;
   INT64 parentId = 1;
   INT64 newParentId = 1;
   string basePath;
   string newBasePath;
   dirMeta  pDirMeta;
   fileMeta fileNode;
   fileMeta newFile;

   PD_LOG(PDDEBUG, "Called: link(), path:%s, link:%s", path, newpath);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   rc = _metaCache.getParentIdName(newpath, &newBasePath, newParentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent parentId, dir=%s, rc=%d",
             newpath, rc);
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   linkName = (CHAR *)newBasePath.c_str();

   rc = _fileCreatingMgr.query(newParentId, linkName, newFile);
   if(SDB_OK == rc)
   {
      PD_LOG(PDERROR, "Failed to link file, name=%s, it does exist", newpath);
      rc = SDB_FE;
      goto error;
   }
   else if(SDB_DMS_EOC != rc)
   {
      PD_LOG(PDERROR, "Failed to query file info from filecreatingmgr, name=%s, rc=%d",
                       linkName, rc);
      goto error;
   }

   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, path=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   rc = _metaCache.getFileInfo(parentId, fileName, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get file, parentId=%d, fileName=%s, rc=%d",
             parentId, fileName, rc);
      goto error;
   }

   fileNode.setName(linkName);
   fileNode.setPid(newParentId);

   rc = _doSetFileNodeAttr(sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set attr on file:%s, rc=%d",
                       newpath, rc);
      goto error;
   }

   try 
   {
      condition = BSON(SEQUOIAFS_LOBOID<<fileNode.lobOid());
      rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<1));
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr on linked file:%s, rc=%d",
                       path, rc);
      goto error;
   }

done:
   releaseConnection(db);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::chmod(const CHAR *path, mode_t mode)
{
   INT32 rc = SDB_OK;
   CHAR *fileName = NULL;
   INT64 parentId = 0;
   string basePath;

   PD_LOG(PDDEBUG, "Called: chmod(), path:%s, mode:%u", path, mode);

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   rc = _metaCache.modMetaMode(parentId, fileName, mode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to mod meta mode, dir=%s, rc=%d",
             fileName, rc);
      goto error;
   }

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::chown(const CHAR *path, uid_t uid, gid_t gid)
{
   INT32 rc = SDB_OK;
   CHAR *fileName = NULL;
   INT64 parentId = 0;
   string basePath;

   PD_LOG(PDDEBUG, "Called: chown(), path:%s, uid:%d, gid:%d", path, uid, gid);

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   rc = _metaCache.modMetaOwn(parentId, fileName, uid, gid);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to mod meta own, dir=%s, rc=%d",
             fileName, rc);
      goto error;
   }

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::truncate(const CHAR *path, off_t newsize)
{
   INT32 rc = SDB_OK;
   CHAR *fileName = NULL;
   INT64 parentId = 0;
   string basePath;
   SINT64 lobSize = 0;
   OID oid;
   lobHandle *lh = NULL;
   struct fuse_file_info *fi = NULL;
   fileLob *fl;
   _fileMeta fMeta;
   
   PD_LOG(PDDEBUG, "Called: truncate(), path:%s, offset:%d", path, newsize);

   lh = new lobHandle;
   if(NULL == lh)
   {
      PD_LOG(PDERROR, "Failed to new lobHandle.");
      rc = SDB_OOM;
      goto error;
   }
   INIT_LOBHANDLE(lh);
   fi = new fuse_file_info;
   if(NULL == fi)
   {
      PD_LOG(PDERROR, "Failed to new fuse_file_info.");
      rc = SDB_OOM;
      goto error;
   }

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }
   
   fileName = (CHAR *)basePath.c_str();
   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }
   
   rc = _metaCache.getFileInfo(parentId, fileName, fMeta);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent file meta, dir=%s, rc=%d",
             path, rc);
      goto error;
   }

   oid = bson::OID(fMeta.lobOid());

   fl = _fileLobMgr.allocFreeFileLob();
   if(NULL == fl)
   {
      rc = SDB_TOO_MANY_OPEN_FD;
      PD_LOG(PDERROR, "Failed to get a free filelob");
      goto error;
   }
   rc = fl->flOpen(_collection.c_str(), oid, _optionMgr.getFflag(), fMeta.size());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "open the filelob failed, path=%s, rc=%d", path, rc);
      goto error;
   }
   
   //lh->hSysFileMetaCL = &sysFileMetaCL;
   lh->flId = fl->flGetFlId();
   ossStrncpy(lh->fileName, fileName, FS_MAX_NAMESIZE);
   lh->parentId = parentId;
   lh->oid = oid;
   fi->fh = (intptr_t)(uint64_t)((void *)lh);

   rc = ftruncate(path, newsize, fi);
   if(SDB_OK != rc)
   {
      fl->flClose(&lobSize);
      PD_LOG(PDERROR, "ftruncate failed, path=%s, rc=%d", path, rc);
      goto error;
   } 

   rc = fl->flClose(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to close file, name=%s, rc=%d", path, rc);
      goto error;
   } 

done:
   SAFE_OSS_DELETE(lh);
   SAFE_OSS_DELETE(fi);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

/*Deprecated, use utimes() instead*/
INT32 sequoiaFS::utime(const CHAR *path, struct utimbuf * ubuf)
{
   INT32 rc = SDB_OK;
   CHAR *fileName = NULL;
   INT64 parentId = 0;
   string basePath;

   PD_LOG(PDDEBUG, "Called: utime(), path:%s, actime:%d, modtime:%d",
          path, ubuf->actime, ubuf->modtime);

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   rc = _metaCache.modMetaUtime(parentId, fileName, 
                                (INT64)ubuf->modtime, 
                                (INT64)ubuf->actime);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to mod meta utime, dir=%s, rc=%d",
             path, rc);
      goto error;
   }

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::open(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollectionSpace cs;
   BSONObj condition;
   OID oid;
   INT64 parentId = 0;
   string basePath;
   lobHandle *lh = NULL;  
   BSONObj options;
   fileLob *fl = NULL;  
   _fileMeta fMeta;
   CHAR *fileName = NULL;

   PD_LOG(PDDEBUG, "Called: open(), path:%s, flags:%d", path, fi->flags);

   lh = new lobHandle;
   if(NULL == lh)
   {
      PD_LOG(PDERROR, "Failed to new lobHandle, path:%s", path);
      rc = SDB_OOM;
      goto error;
   }
   INIT_LOBHANDLE(lh);
   lh->isCreate = FALSE;
   
   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }
   
   fileName = (CHAR*)basePath.c_str();
   ossStrncpy(lh->fileName, fileName, FS_MAX_NAMESIZE);
   lh->parentId = parentId;

   rc = _fileCreatingMgr.flushfile(parentId, fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }
   
   rc = _metaCache.getFileInfo(parentId, fileName, fMeta);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent file meta, dir=%s, rc=%d",
             path, rc);
      goto error;
   }

   oid = bson::OID(fMeta.lobOid());
 
   fl = _fileLobMgr.allocFreeFileLob();
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to get flId");
      rc = SDB_TOO_MANY_OPEN_FD;
      goto error;
   }
   rc = fl->flOpen(_collection.c_str(), oid, _optionMgr.getFflag(), fMeta.size());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flOpen, name=%s, rc=%d", path, rc);
      goto error;
   }

   lh->oid = oid;
   lh->flId = fl->flGetFlId();
   fi->fh = (intptr_t)(uint64_t)((void *)lh);

done:
   PD_LOG(PDDEBUG, "Called: open() end, path:%s, rc=%d", path, rc);
   rc = _convertErrorCode(rc);
   if(db != NULL)
   {
      releaseConnection(db);
   }
   return rc;

error:
   SAFE_OSS_DELETE(lh);
   goto done;
}

INT32 sequoiaFS::read(const CHAR *path, CHAR *buf, size_t size, off_t offset ,
                      struct fuse_file_info *fi)
{
   sdbCollection cl;
   INT32 rc = SDB_OK;
   INT32 readlen = 0;
   lobHandle *lh = NULL;
   std::map<UINT64, INT8>::iterator it;
   bson::BSONObj sessionAttr;
   fileLob *fl;

   PD_LOG(PDDEBUG, "Called: read(), path:%s, offset:%d, size:%d",
          path, offset, size);

   lh = (lobHandle *)fi->fh;

   if(lh->isCreate)
   {
      rc = _fileCreatingMgr.flushfile(lh->parentId, lh->fileName);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                          path, rc);
         goto error;
      }
   }

   // if fileLob id is invalid(flId < 0 ), build a new filelob for the file
   // which include get a valid fileLob id and open the fileLob
   if(lh->flId < 0)
   {
      rc = buildFileLobForFile(lh);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to buildFileLobForFile, "
                         "name=%s, offset:%d, size:%d, rc=%d", 
                          path, offset, size, rc);
         goto error;
      }
   }

   fl = _fileLobMgr.getFileLob(lh->flId);
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to getFileLob, flId=%d", lh->flId);
      rc = SDB_SYS;
      goto error;
   }
   rc = fl->flRead( offset, size, buf, &readlen);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to read file, name=%s, offset:%d, size:%d, rc=%d", path, offset, size, rc);
      goto error;
   }

   rc = (INT32)readlen;

done:
   PD_LOG(PDDEBUG, "read end, path:%s, offset:%d, size:%d, readlen=%d",
             path, offset, size, readlen);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::write(const CHAR *path,
                       const CHAR *buf,
                       size_t size,
                       off_t offset,
                       struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   lobHandle *lh = NULL;
   fileLob *fl = NULL;
   BOOLEAN isProcessed = FALSE;

   PD_LOG(PDDEBUG, "Called: write(), path:%s, offset:%ld, size:%ld",
          path, offset, size);

   lh = (lobHandle *)fi->fh;

   rc = _fileCreatingMgr.write(lh->parentId,lh->fileName, buf, size, offset, lh, isProcessed);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to write file, name=%s, offset:%d, size:%d, rc=%d", 
                       path, offset, size, rc);
      goto error;
   }

   //filecreatingmgr has processeded this write
   if(isProcessed)
   {
      rc = size;
      goto done;
   }

   // if filecreatingmgr can not processed this write, use fileLobMgr to processed
   // if fileLob id is invalid(flId < 0 ), build a new filelob for the file
   // which include get a valid fileLob id and open the fileLob
   if(lh->flId < 0)
   {
      rc = buildFileLobForFile(lh);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to buildFileLobForFile, "
                         "name=%s, offset:%d, size:%d, rc=%d", 
                          path, offset, size, rc);
         goto error;
      }
   }
   
   fl = _fileLobMgr.getFileLob(lh->flId);
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to getFileLob, flId=%d", lh->flId);
      rc = SDB_SYS;
      goto error;
   }
   rc = fl->flwrite( offset, size, buf );
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to write file, name=%s, offset:%d, size:%d, rc=%d", path, offset, size, rc);
      goto error;
   }
   lh->isDirty = TRUE;
   
   rc = size;
done:
   PD_LOG(PDDEBUG, "write, rc=%d", rc);
   return rc;
   
error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::release(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   SINT64 lobSize = 0;
   UINT64 mtime = 0;
   BSONObj rule;
   BSONObj condition;
   lobHandle *lh = NULL;
   fileLob *fl;
   sdb *db = NULL;
   sdbCollection sysFileMetaCL;
   BOOLEAN isProcessed = FALSE;

   PD_LOG(PDDEBUG, "Called: release(), path:%s", path);

   lh = (lobHandle *)fi->fh;

   rc = _fileCreatingMgr.release(lh->parentId, lh->fileName, lh, isProcessed);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to release file, name=%s, offset:%d, size:%d, rc=%d", 
                       path, rc);
      goto error;
   }

   if(isProcessed)
   {
      goto done;
   }

   // if fileLob id is invalid(flId < 0 ), build a new filelob for the file
   // which include get a valid fileLob id and open the fileLob
   if(lh->flId < 0)
   {
      rc = buildFileLobForFile(lh);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to buildFileLobForFile, name=%s, rc=%d", 
                          path, rc);
         goto error;
      }
   }
  
   fl = _fileLobMgr.getFileLob(lh->flId);
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to getFileLob, flId=%d", lh->flId);
      rc = SDB_SYS;
      goto error;
   }
   rc = fl->flClose(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to close file, name=%s, rc=%d", path, rc);
      goto error;
   } 

   if(lh->isDirty)
   {
      rc = getConnection(&db);
      if(SDB_OK != rc)
      {
         goto error;
      }
      
      rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                _sysFileMetaCLFullName.c_str(), rc);
         goto error;
      }
      
      mtime = ossGetCurrentMilliseconds();
      try 
      {
         rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<lobSize<<SEQUOIAFS_MODIFY_TIME<<\
                     (SINT64)mtime<<SEQUOIAFS_ACCESS_TIME<<(SINT64)mtime));
         condition = BSON(SEQUOIAFS_LOBOID<<(lh->oid).toString());
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
      rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "update file meta failed, name=%s, rc=%d", path, rc);
         goto error;
      }
      lh->isDirty = FALSE;
   }
   
done:
   SAFE_OSS_DELETE(lh);
   if(db != NULL)
   {
      releaseConnection(db);
   }
   PD_LOG(PDDEBUG, "release, rc=%d", rc);
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}
/*get filesystem statistics*/
INT32 sequoiaFS::statfs(const CHAR *path, struct statvfs *statfs)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: statfs(), path:%s", path);
   goto error;
   
done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::flush(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   lobHandle *lh = NULL;
   fileLob *fl = NULL;
   SINT64 lobSize = 0;
   UINT64 mtime = 0;
   BSONObj rule;
   BSONObj condition;
   sdb *db = NULL;
   sdbCollection sysFileMetaCL;
   BOOLEAN isProcessed;

   PD_LOG(PDDEBUG, "Called: flush(), path:%s", path);

   lh = (lobHandle *)fi->fh;

   _fileCreatingMgr.flush(lh->parentId, lh->fileName, lh, isProcessed);
   if(isProcessed)
   {
      goto done;
   }

   // if fileLob id is invalid(flId < 0 ), build a new filelob for the file
   // which include get a valid fileLob id and open the fileLob
   if(lh->flId < 0)
   {
      rc = buildFileLobForFile(lh);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to buildFileLobForFile, "
                         "name=%s, offset:%d, size:%d, rc=%d", 
                          path, rc);
         goto error;
      }
   }
   
   fl = _fileLobMgr.getFileLob(lh->flId);
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to getFileLob, flId=%d", lh->flId);
      rc = SDB_SYS;
      goto error;
   }
   rc = fl->flFlush(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flush file, name=%s, rc=%d", path, rc);
      goto error;
   }

   if(lh->isDirty)
   {
      rc = getConnection(&db);
      if(SDB_OK != rc)
      {
         goto error;
      }
      
      rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
                _sysFileMetaCLFullName.c_str(), rc);
         goto error;
      }
   
      mtime = ossGetCurrentMilliseconds();
      try 
      {
         rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<lobSize<<SEQUOIAFS_MODIFY_TIME<<\
                     (SINT64)mtime<<SEQUOIAFS_ACCESS_TIME<<(SINT64)mtime));
         condition = BSON(SEQUOIAFS_LOBOID<<(lh->oid).toString());
      }
      catch (std::exception &e)   
      {
         rc = SDB_DRIVER_BSON_ERROR;
         PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
         goto error; 
      }
      rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "update file meta failed, name=%s, rc=%d", path, rc);
         goto error;
      }

      lh->isDirty = FALSE;
   }
   
done:
   if(db != NULL)
   {
      releaseConnection(db);
   }

   PD_LOG(PDDEBUG, "flush end, rc=%d", rc);
   return rc;
error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::fsync(const CHAR *path,
                       INT32 datasync,
                       struct fuse_file_info *fi)
{
   PD_LOG(PDDEBUG, "Called: fsync(), path:%s", path);
   return 0;
}

INT32 sequoiaFS::opendir(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   lobHandle *lh = NULL;
   INT64 id;
   BSONObj options;
   dirMeta pDirMeta;
   INT64 parentId = 0;
   string basePath;

   PD_LOG(PDDEBUG, "Called: opendir(), path:%s", path);

   lh = new lobHandle;
   if(NULL == lh)
   {
      PD_LOG(PDERROR, "Failed to new lobHandle, path:%s", path);
      rc = SDB_OOM;
      goto error;
   }
   INIT_LOBHANDLE(lh);

   if(path == "/")
   {
      id = 1;
   }
   else
   {
      rc = _metaCache.getParentIdName(path, &basePath, parentId);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get parent dir meta info, path=%s, rc=%d",
                path, rc);
         goto error;
      }
      //get the ino of the dir
      rc = _metaCache.getDirInfo(parentId, (CHAR*)(basePath.c_str()), pDirMeta);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to getDirInfo, basePath=%s, rc=%d",
                basePath.c_str(), rc);
         goto error;
      }
      id = pDirMeta.id();
   }
   
   lh->parentId = id;
   fi->fh = (intptr_t)(uint64_t)((void *)lh);

done:
   return rc;

error:
   SAFE_OSS_DELETE(lh);
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::readdir(const CHAR *path,
                         void *buf,
                         fuse_fill_dir_t filler,
                         off_t offset,
                         struct fuse_file_info *fi)
{
   lobHandle *lh = NULL;
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: readdir(), path:%s", path);

   rc = filler(buf, ".", NULL, 0);
   if(0 != rc)
   {
      rc = SDB_NOSPC;
      goto error;
   }

   rc = filler(buf, "..", NULL, 0);
   if(0 != rc)
   {
      rc = SDB_NOSPC;
      goto error;
   }

   lh = (lobHandle*)fi->fh;

   rc = _fileCreatingMgr.flushDir(lh->parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushDir, parentId=%d, it does exist", lh->parentId);
      goto error;
   }

   rc = _metaCache.readDir(lh->parentId, buf, filler);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to readDir id:%d, rc=%d",
             lh->parentId, rc);
      goto error;
   }
   
done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::releasedir(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   lobHandle *lh = NULL;

   PD_LOG(PDDEBUG, "Called: releasedir(), path:%s", path);

   lh = (lobHandle*)fi->fh;
   SAFE_OSS_DELETE(lh);

   return rc;
}

INT32 sequoiaFS::fsyncdir(const CHAR *path,
                          INT32 datasync,
                          struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: fsyncdir(), path:%s", path);
   goto error;

done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::_do_access(struct stat *stbuf)
{
   INT32 rc = SDB_OK;

   if(S_ISREG(stbuf->st_mode) && !(stbuf->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
   {
      rc = -EACCES;
      PD_LOG(PDERROR, "Failed to access file, rc=%d", rc);
      goto error;
   }

   if(S_ISDIR(stbuf->st_mode) && !(stbuf->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
   {
      rc = -EACCES;
      PD_LOG(PDERROR, "Failed to access dir, rc=%d", rc);
      goto error;
   }
   
done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::access(const CHAR *path, INT32 mask)
{
   struct stat stbuf;
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: access(), path:%s, mask:%d", path, mask);
   if(mask & X_OK)
   {
      rc = getattr(path,&stbuf);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to getattr, rc=%d", rc);
          goto error;
      }

      rc = _do_access(&stbuf);
   }

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

//when I call creat(filename, 0740), It seams it will call open to create
//->getattr():/testlob3 ->
INT32 sequoiaFS::create(const CHAR *path,
                        mode_t mode,
                        struct fuse_file_info *fi, 
                        struct fuse_context *context)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbLob lob;
   sdbCollection cl;
   sdbCollection sysFileMetaCL;
   OID oid;
   string basePath;
   lobHandle *lh = NULL;
   _fileMeta fileNode;
   UINT64 ctime = 0;
   INT64 parentId = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   fileLob *fl;
   _fileMeta fMeta;
   BOOLEAN isProcessed = FALSE;

   if(context != NULL)
   {
      uid = context->uid;
      gid = context->gid;
   }

   PD_LOG(PDDEBUG, "Called: create(), path:%s, mode:%u, flags:%d", path, mode, fi->flags);
   
   lh = new lobHandle;
   if(NULL == lh)
   {
      PD_LOG(PDERROR, "Failed to new lobHandle, path:%s", path);
      rc = SDB_NOSPC;
      goto error;
   }
   INIT_LOBHANDLE(lh);
   fi->fh = (intptr_t)(uint64_t)((void *)lh);
   lh->isCreate = TRUE;
  
   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
             path, rc);
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   ossStrncpy(lh->fileName, fileName, FS_MAX_NAMESIZE);
   lh->parentId = parentId;

   rc = _metaCache.getFileInfo(parentId, fileName, fMeta);
   if(SDB_OK == rc)
   {
      PD_LOG(PDERROR, "Failed to create file, name=%s, it does exist", fileName);
      rc = SDB_FE;
      goto error;
   }
   else if(SDB_DMS_EOC != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent file meta, dir=%s, rc=%d",
             path, rc);
      goto error;
   }

   rc = _fileCreatingMgr.create(parentId, fileName, path, mode, uid, gid,
                                lh, isProcessed);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create file in filecreatingmgr, name=%s. rc=%d", fileName, rc);
      goto error;
   }

   if(isProcessed)
   {
      goto done;
   }

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _collection.c_str(), rc);
      goto error;
   }

   rc = cl.createLob(lob);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create lob, rc=%d", rc);
      goto error;
   }

   rc = lob.getOid(oid);
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

   ctime = ossGetCurrentMilliseconds();

   fileNode.setName(fileName);
   fileNode.setMode(S_IFREG | mode);
   fileNode.setNLink(1);
   fileNode.setPid(parentId);
   fileNode.setLobOid(oid.toString().c_str());
   fileNode.setCtime(ctime);
   fileNode.setMtime(ctime);
   fileNode.setAtime(ctime);
   fileNode.setUid(uid);
   fileNode.setGid(gid);
   fileNode.setSize(0);
   fileNode.setSymLink(path);

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }
   
   rc = _doSetFileNodeAttr(sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to insert file meta, rc=%d", rc);
      goto error;
   }

   fl = _fileLobMgr.allocFreeFileLob();
   if(NULL == fl)
   {
      rc = SDB_TOO_MANY_OPEN_FD;
      PD_LOG(PDERROR, "There are no fileLobs available, open too many files");
      goto error;
   }
   
   rc = fl->flOpen(_collection.c_str(), oid, _optionMgr.getFflag(), 0);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open file, name=%s, rc=%d", fileName, rc);
      goto error;
   }

   lh->oid = oid;
   lh->flId = fl->flGetFlId();

   PD_LOG(PDDEBUG, "Called: create() successfully, Name:%s, Pid:%d", 
                   fileName, parentId);

done:
   if(db != NULL)
   {
      releaseConnection(db);
   }
   return rc;

error:
   rc = _convertErrorCode(rc);
   SAFE_OSS_DELETE(lh);
   goto done;
}

INT32 sequoiaFS::ftruncate(const CHAR *path,
                           off_t offset,
                           struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   SINT64 lobSize = 0;
   lobHandle *lh = NULL;
   sdbCollection cl;
   BSONObj rule;
   BSONObj condition;
   UINT64 mtime = 0;
   bson::OID oid;
   struct timeval tval;
   fileLob *fl;
   sdb *db = NULL;
   sdbCollection sysFileMetaCL;

   PD_LOG(PDDEBUG, "Called: ftruncate(), path:%s, offset:%d", path, offset);

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, rc=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   lh = (lobHandle *)fi->fh;

   rc = _fileCreatingMgr.flushfile(lh->parentId, lh->fileName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   // if fileLob id is invalid(flId < 0 ), build a new filelob for the file
   // which include get a valid fileLob id and open the fileLob
   if(lh->flId < 0)
   {
      rc = buildFileLobForFile(lh);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to buildFileLobForFile, name=%s, lobOid:%d, rc=%d", 
                          path, lh->oid.toString().c_str(), rc);
         goto error;
      }
   }
   
   fl = _fileLobMgr.getFileLob(lh->flId);
   if(NULL == fl)
   {
      PD_LOG(PDERROR, "Failed to getFileLob, flId=%d", lh->flId);
      rc = SDB_SYS;
      goto error;
   }
   rc = fl->fltruncate(offset); 
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to truncate file, name=%s, rc=%d", path, rc);
      goto error;
   }

   oid = lh->oid;

   gettimeofday(&tval, NULL);
   mtime = tval.tv_sec * 1000 + tval.tv_usec/1000;

   lobSize = offset;
   //sysFileMetaCL = (sdbCollection *)lh->hSysFileMetaCL;
   try 
   {
      rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<lobSize<<\
                  SEQUOIAFS_MODIFY_TIME<<(SINT64)mtime));
      condition = BSON(SEQUOIAFS_LOBOID<<oid.toString());
   }
   catch (std::exception &e)   
   {
      rc = SDB_DRIVER_BSON_ERROR;
      PD_LOG(PDERROR, "Exception[%s] occurs when build bson obj.", e.what());
      goto error; 
   }
   rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, rc=%d", rc);
      goto error;
   }

done:
   if(db != NULL)
   {
      releaseConnection(db);
   }
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::fgetattr(const CHAR *path, struct stat *buf,
                          struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   _fileMeta fMeta;
   lobHandle *lh = NULL;

   PD_LOG(PDDEBUG, "Called: fgetattr(), path:%s", path);

   lh = (lobHandle *)fi->fh;

   ossMemset(buf, 0, sizeof(struct stat));

   rc = _fileCreatingMgr.query(lh->parentId, lh->fileName, fMeta);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC != rc)
      {
         PD_LOG(PDERROR, "query file info from filecreatingmgr failed." 
                         "path=%s, rc=%d", path, rc);
         goto error;
      }
      
      rc = _metaCache.getFileInfo(lh->parentId, lh->fileName, fMeta);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to get file info, path=%s. rc=%d ", path, rc);
         goto error;
      }
   }
   
   buf->st_size = fMeta.size();
   buf->st_ctime /= fMeta.ctime() / 1000;
   buf->st_mtime /= fMeta.mtime() / 1000;
   buf->st_mode = fMeta.mode();
   buf->st_nlink = fMeta.nLink();
   buf->st_uid = fMeta.uid();
   buf->st_gid = fMeta.gid();
   rc = SDB_OK;

done:
   return rc;
error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::lock(const CHAR *path, struct fuse_file_info *fi,
                      INT32 cmd, struct flock *lock)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDDEBUG, "Called: lock(), path:%s", path);
   goto error;

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::utimes(const CHAR *path, const struct timespec ts[2])
{
   INT32 rc = SDB_OK;
   INT64 parentId = 0;
   string basePath;
   INT64 atime = (INT64)(ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000);
   INT64 mtime = (INT64)(ts[1].tv_sec * 1000 + ts[1].tv_nsec/1000000);

   PD_LOG(PDDEBUG, "Called: utimes(), path:%s, actime:%d, modtime:%d",
          path, (ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000),
          (ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000));
 
   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }
   
   rc = _metaCache.getParentIdName(path, &basePath, parentId);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get parent id and name, path=%s, rc=%d",
                       path, rc);
      goto error;
   }

   rc = _fileCreatingMgr.flushfile(parentId, (CHAR*)basePath.c_str());
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to flushfile, name=%s, rc=%d", 
                       path, rc);
      goto error;
   }

   rc = _metaCache.modMetaUtime(parentId, (CHAR*)basePath.c_str(), 
                                (INT64)mtime, 
                                (INT64)atime);  
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to mod meta utime, dir=%s, rc=%d",
             path, rc);
      goto error;
   }

done:
   return rc;

error:
   rc = _convertErrorCode(rc);
   goto done;
}

INT32 sequoiaFS::bmap(const CHAR *path, size_t blocksize, uint64_t *idx)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDDEBUG, "Called: bmap(), path:%s", path);
   goto error;

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::ioctl(const CHAR *path, INT32 cmd, void *arg,
                       struct fuse_file_info *fi, UINT32 flags, void *data)
{
   INT32 rc = SDB_OK;

   switch(cmd)
   {
   default:
     rc = - ENOIOCTLCMD;
   }

   return rc;
}

INT32 sequoiaFS::flock(const CHAR *path, struct fuse_file_info *fi, INT32 op)
{
   INT32 rc = SDB_OK;
   PD_LOG(PDDEBUG, "Called: flock(), path:%s", path);
   goto error;

done:
   return rc;
error:
   goto done;
}

void sequoiaFS::getSysInfo()
{
   _optionMgr.getDataCacheSize();
   INT32 usedFlId = _fileLobMgr.getUsedFlIdCount();
   INT32 cacheUsed = _fileLobMgr.getCacheLoader()->getUsedCount();
   INT32 queueSize = _fileLobMgr.getCacheQueue()->getQueueSize();
   INT64 createFileUsed = _fileCreatingMgr.getUsedCount();
   INT32 createFileQueue = _fileCreatingMgr.getUploadQueueSize();
   INT32 coldSize = _metaCache.getDirMetaCache()->getLRUList()->getColdSize();
   INT32 hotSize = _metaCache.getDirMetaCache()->getLRUList()->getHotSize();
   
   INT32 readCount = _fileLobMgr.getReadCount();
   INT32 downLoadCount = _fileLobMgr.getDownloadCount();
   INT32 dataHashCallCount = _fileLobMgr.getHashBucket()->getCallCount();
   INT32 dataHashConflictCount = _fileLobMgr.getHashBucket()->getConflictCount();
   INT32 metaGetHashCallCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getGetCallCount();
   INT32 metaGetHashConflictCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getGetConflictCount();
   INT32 metaAddHashCallCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getAddCallCount();
   INT32 metaAddHashConflictCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getAddConflictCount();
   INT32 metaDelHashCallCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getDelCallCount();
   INT32 metaDelHashConflictCount = _metaCache.getDirMetaCache()->getMetaHashBucket()->getDelConflictCount();

   PD_LOG(PDEVENT, "\nSysInfo: \n"
                   "Open files = %d \n"
                   "Data cache used(MB) = %d \n"
                   "Queue length = %d \n"
                   "creating file used = %ld \n"
                   "create file queue length = %d \n"
                   "Data cache hit rate = %.1f%%, hit count = %d, total count = %d \n"
                   "Data hash bucket conflict rate = %.1f%%, conflict count = %d, total count = %d \n"
                   "Meta hash bucket get conflict rate = %.1f%%, get conflict count = %d, get total count = %d \n"
                   "Meta hash bucket add conflict rate = %.1f%%, add conflict count = %d, add total count = %d \n"
                   "Meta hash bucket del conflict rate = %.1f%%, del conflict count = %d, del total count = %d \n"
                   "Meta link hotSize = %d, coldSize = %d \n",
                   usedFlId,
                   cacheUsed * 4,
                   queueSize,
                   createFileUsed,
                   createFileQueue,
                   readCount == 0 ? 0 : (((FLOAT32)readCount - (FLOAT32)downLoadCount) * 100/readCount),
                   (readCount - downLoadCount), readCount,
                   dataHashCallCount == 0 ? 0 : ((FLOAT32)dataHashConflictCount * 100/dataHashCallCount),
                   dataHashConflictCount, dataHashCallCount,
                   metaGetHashCallCount == 0 ? 0 : ((FLOAT32)metaGetHashConflictCount * 100/metaGetHashCallCount),
                   metaGetHashConflictCount, metaGetHashCallCount,
                   metaAddHashCallCount == 0 ? 0 : ((FLOAT32)metaAddHashConflictCount * 100/metaAddHashCallCount),
                   metaAddHashConflictCount, metaAddHashCallCount,
                   metaDelHashCallCount == 0 ? 0 : ((FLOAT32)metaDelHashConflictCount * 100/metaDelHashCallCount),
                   metaDelHashConflictCount, metaDelHashCallCount,
                   hotSize, coldSize);

   _fileLobMgr.cleanReadCount();
   _fileLobMgr.getHashBucket()->cleanDataHashCounts();
   _metaCache.getDirMetaCache()->getMetaHashBucket()->cleanMetaHashCounts();
}

void sequoiaFS::_cleanCounts()
{
   INT32 count = 0;
   INT32 oneDay = 60 * 60 * 24; //60sec * 60min * 24 = one day

   //clean counts every day
   while(_running)
   {
      ossSleepsecs(1);
      ++count;
      if(count >= oneDay)
      {
         getSysInfo();
         count = 0;
      }
   }
}

INT32 sequoiaFS::_convertErrorCode(INT32 rc)
{
   INT32 errorCode = rc;
   
   switch(rc)
   {
      case SDB_OK:
      {
         errorCode = SDB_OK;
         break;
      }
      case SDB_DMS_EOC:
      {
         errorCode = -ENOENT;
         break;
      }
      case SDB_OOM:
      {
         errorCode = -ENOMEM;
         break;
      }
      case SDB_NOSPC:
      {
         errorCode = -ENOSPC;
         break;
      }
      case SDB_DIR_NOT_EMPTY:
      {
         errorCode = -ENOTEMPTY;
         break;
      }
      case SDB_FE:
      {
         errorCode = -EEXIST;
         break;
      }
      case SDB_TOO_MANY_OPEN_FD:
      {
         errorCode = -EMFILE;
         break;
      }
      case SDB_INVALIDARG:
      {
         errorCode = -EPERM;
         break;
      }
      default:
         errorCode = -EIO;
         break;
   }

   return errorCode;
}

INT32 sequoiaFS::buildFileLobForFile(lobHandle* lh)
{
   INT32 rc = SDB_OK;
   fileLob *fl;
   
   fl = _fileLobMgr.allocFreeFileLob();
   if(NULL == fl)
   {
      rc = SDB_TOO_MANY_OPEN_FD;
      PD_LOG(PDERROR, "Failed to get flId");
      goto error;
   }
   
   rc = fl->flOpen(_collection.c_str(), lh->oid, _optionMgr.getFflag(), 0);
   if(SDB_OK != rc)
   {
      _fileLobMgr.addRecycle(fl->flGetFlId());
      PD_LOG(PDERROR, "Failed to flOpen, name=%s, rc=%d", lh->fileName, rc);
      goto error;
   }

   lh->flId = fl->flGetFlId();

done:
   return rc;
error:
   goto done;   
}


