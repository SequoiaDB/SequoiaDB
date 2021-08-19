/*******************************************************************************


   Copyright (C) 2011-2018 SequoiaDB Ltd.

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
#include "sequoiaFSLruCache.hpp"
#include "sequoiaFSOptionMgr.hpp"
#include "omagentDef.hpp"
#include "utilStr.hpp"
#include "ossVer.h"

#define SEQUOIAFS_LOG_DIR "sequoiafslog"
const string SEQUOIAFS_META_MAP_AUDIT_CL= "maphistory";
const string SEQUOIAFS_META_ID_CL = "sequenceid";

const string SEQUOIAFS_META_MAP_AUDIT_CL_FULL = SEQUOIAFS_META_CS + "." +
                                                SEQUOIAFS_META_MAP_AUDIT_CL;
const string SEQUOIAFS_META_ID_CL_FULL = SEQUOIAFS_META_CS + "." +
                                         SEQUOIAFS_META_ID_CL;

#define SEQUOIAFS_NAME          "Name"
#define SEQUOIAFS_MODE          "Mode"
#define SEQUOIAFS_UID           "Uid"
#define SEQUOIAFS_GID           "Gid"
#define SEQUOIAFS_NLINK         "NLink"
#define SEQUOIAFS_PID           "Pid"
#define SEQUOIAFS_ID            "Id"
#define SEQUOIAFS_LOBOID        "LobOid"
#define SEQUOIAFS_SIZE          "Size"
#define SEQUOIAFS_CREATE_TIME      "CreateTime"
#define SEQUOIAFS_MODIFY_TIME      "ModifyTime"
#define SEQUOIAFS_ACCESS_TIME      "AccessTime"
#define SEQUOIAFS_SYMLINK          "SymLink"

#define SEQUOIAFS_SRC_CLNAME       "SourceCL"
#define SEQUOIAFS_DIR_META_CLNAME  "DirMetaCL"
#define SEQUOIAFS_FILE_META_CLNAME "FileMetaCL"
#define SEQUOIAFS_ADDRESS          "Address"
#define SEQUOIAFS_MOUNT_POINT      "MountPoint"
#define SEQUOIAFS_MOUNT_TIME       "MountTime"

#define SEQUOIAFS_SEQUENCEID       "Sequenceid"
#define SEQUOIAFS_SEQUENCEID_VALAUE "Value"

#define NUM_OF_META_CL 2
#define CURSOR_OF_META_DIR_CL 0
#define CURSOR_OF_META_FILE_CL 1


#define MAXCNT 1000
#define ENOIOCTLCMD 515

using namespace sequoiafs;
#define ROOT_ID 1
const INT32 BUFSIZE=1000;

BOOLEAN enableDataSource = FALSE;

LRUCache *lrucache;

pthread_mutex_t mutex;
pthread_mutexattr_t attr;

struct lobHandle
{
   sdb *hSdb;
   sdbLob *hLob;
   OID oid;
   sdbCollection *hSysFileMetaCL;
   sdbCollection *hSysDirMetaCL;
   sdbCursor *hCursor[NUM_OF_META_CL];
   pthread_mutex_t lock;
};

struct times
{
   INT64 getattr;
   INT64 opendir;
   INT64 readdir;
   INT64 releasedir;
};

#define INIT_NODE(node) \
{\
   node.lobName = "";\
   node.lobMode = 0;\
   node.uid = 0;\
   node.gid = 0;\
   node.nlink = 0;\
   node.lobIno = 0;\
   node.lobOid = "";\
   node.lobSize= 0;\
   node.lobCTime = 0;\
   node.lobMTime = 0;\
   node.lobATime = 0;\
}

#define INIT_DIR_NODE(node) \
{\
   node.name = "";\
   node.mode = 0;\
   node.uid = 0;\
   node.gid = 0;\
   node.pid = 0;\
   node.id = 0;\
   node.size= 0;\
   node.ctime = 0;\
   node.mtime = 0;\
   node.atime = 0;\
   node.symLink = "";\
}

#define INIT_FILE_NODE(node) \
{\
   node.name = "";\
   node.mode = 0;\
   node.uid = 0;\
   node.gid = 0;\
   node.nLink = 0;\
   node.lobOid = "";\
   node.pid = 0;\
   node.size= 0;\
   node.ctime = 0;\
   node.mtime = 0;\
   node.atime = 0;\
   node.symLink = "";\
}

#define INIT_LOBHANDLE(lh) \
{\
   lh->hSdb = NULL;\
   lh->hCursor[0] = NULL;\
   lh->hCursor[1] = NULL;\
   lh->hSysFileMetaCL = NULL;\
   lh->hSysDirMetaCL = NULL;\
   lh->hLob = NULL;\
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
         PD_LOG( PDERROR, "Failed to create collectionspace:%s. rc:%d", csName, rc ) ;
         goto error;
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collectionspace:%s. rc:%d", csName, rc ) ;
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
      rc = cs.createCollection(clName, BSON("ReplSize" << replsize), cl);
      if(SDB_OK != rc)
      {
         PD_LOG( PDERROR, "Failed to create collection:%s. rc:%d", clName, rc ) ;
         goto error;
      }
   }
   else if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to get collection:%s. rc:%d", clName, rc ) ;
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
      ossPrintf("Failed to list collection, error=%d"OSS_NEWLINE, rc);
      goto error;
   }

   rc = cursor.next(clObj);
   while(SDB_DMS_EOC != rc)
   {
      if(SDB_OK != rc)
      {
          ossPrintf("Failed to get record in cursor, error=%d"OSS_NEWLINE, rc);
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

static size_t namePidHash(INT64 pid, const char *fullpath)
{
   uint64_t hash = 5381;
   const char *name;
   char str[100] = {0};

   sprintf(str, "%lld", pid);
   name = fullpath;

   for (; *name; name++)
   {
      hash = hash * 31 + (unsigned char) *name;
   }
   name = str;

   for (; *name; name++)
   {
       hash = hash * 31 + (unsigned char) *name;
   }
   return hash;
}

void InitLruCace(INT32 size)
{
   lrucache = new LRUCache(size);
   lrucache->initMutex();
}

INT32 buildDialogPath(CHAR *diaglogPath, CHAR *diaglogPathFromCmd,
                      UINT32 bufSize)
{
   INT32 rc = SDB_OK;
   CHAR *logPath;
   CHAR currentPath[OSS_MAX_PATHSIZE + 1] = {0};

   if(bufSize < OSS_MAX_PATHSIZE + 1)
   {
      ossPrintf("Path buffer size is too small: %u"OSS_NEWLINE, bufSize);
      goto error;
   }

   rc = ossGetEWD(currentPath, OSS_MAX_PATHSIZE);
   if(rc)
   {
      ossPrintf("Get working directory failed: %d"OSS_NEWLINE, rc);
      goto error;
   }

   logPath = (ossStrcmp(diaglogPathFromCmd, "") == 0) ?
             currentPath :  diaglogPathFromCmd;
   //./diaglog/sequoiafs.log
   rc = engine::utilBuildFullPath(logPath, PMD_OPTION_DIAG_PATH,
                                  OSS_MAX_PATHSIZE, diaglogPath);
   if(rc)
   {
      ossPrintf("Build log path failed: %d"OSS_NEWLINE, rc);
      goto error;
   }

   rc = ossMkdir(diaglogPath);
   if(rc)
   {
      if(SDB_FE != rc)
      {
          ossPrintf("Make diralog path [%s] faild: %d"OSS_NEWLINE,
                    diaglogPath, rc);
          goto error;
      }
      else
      {
          rc = SDB_OK;
      }
   }

done:
   return rc;
error:
   goto done;
}

sequoiafsOptionMgr * sequoiaFS::getOptionMgr()
{
   return &_optionMgr;
}

void sequoiaFS::setDataSourceConf(const CHAR * userName,
                                  const CHAR *passwd,
                                  const INT32 connNum)
{
   conf.setUserInfo(userName, passwd);
   conf.setConnCntInfo(50, 10, 20, connNum);
   conf.setCheckIntervalInfo( 60*1000, 0 );
   conf.setSyncCoordInterval( 60*1000 );
   conf.setConnectStrategy( DS_STY_BALANCE );
   conf.setValidateConnection( TRUE );
   conf.setUseSSL( FALSE );
}

INT32 sequoiaFS::initDataSource(const CHAR * userName,
                                const CHAR *passwd,
                                const INT32 connNum)
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: initCoonPool()");

   setDataSourceConf(userName, passwd, connNum);

   // init sdbDataSource
   getCoordHost();
   rc = ds.init( _coordHostPort, conf);
   if (SDB_OK != rc)
   {
      ossPrintf("Fail to init sdbDataSouce, error=%d"OSS_NEWLINE, rc);
      goto error;
   }

   // enable sdbDataSource
   rc = ds.enable();
   if(SDB_OK != rc)
   {
      ossPrintf("Fail to enable sdbDataSource, error=%d"OSS_NEWLINE, rc);
      goto error;
   }

done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::disableDataSource()
{
   INT32 rc = SDB_OK;

   rc = ds.disable();
   if(SDB_OK != rc)
   {
      ossPrintf("Fail to disable sdbDataSource, error=%d"OSS_NEWLINE, rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;

}


INT32 sequoiaFS::closeDataSource()
{
   INT32 rc = SDB_OK;

   PD_LOG(PDDEBUG, "Called: closeDataSource()");

   // disable sdbDataSource
   rc = disableDataSource();
   if(SDB_OK != rc)
   {
      goto error;
   } // dose sdbDataSource
   ds.close();

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::getConnection(sdb **connection)
{

   INT32 rc = SDB_OK;

   rc = ds.getConnection(*connection);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to get a connection, error=%d, exit."OSS_NEWLINE, rc);
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
      PD_LOG(PDERROR, "Failed to get index, idx=%s, cl=%s, error=%d",
             idxName, clName.c_str(), rc);
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_IXM_NOTEXIST != rc && SDB_DMS_EOC != rc)
      {
          PD_LOG(PDERROR, "Error happened during do cursor current, idx=%s, "
                 "cl=%s, error=%d", idxName, clName.c_str(), rc);
          goto error;
      }

      rc = collection.createIndex(indexDef, idxName, isUnique, isEnforced);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to create index, idx=%s, cl=%s, error=%d",
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
   sdbCollection collection;
   sdbCollectionSpace cs;
   string csName;
   string clName;

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

   rc = getCollection(cs, clName.c_str(), collection, replSize());
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   return rc;
error:
   goto done;
}


INT32 sequoiaFS::getAndUpdateID(sdbCollection *cl, INT64 *sequenceId)
{
   INT32 rc = SDB_OK;
   BSONObj condition;
   BSONObj rule;
   sdbCursor cursor;
   BSONObj record;

   rule = BSON("$inc"<<BSON(SEQUOIAFS_SEQUENCEID_VALAUE<<1));
   condition = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_SEQUENCEID);
   rc = cl->queryAndUpdate(cursor, rule, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query and update sequenceid, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SEQUENCEID_VALAUE,
                       (void *)(sequenceId), NumberLong);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get value of sequoenceid, error=%d", rc);
      goto error;
   }

done:
   return rc;
error:
   goto done;

}

INT32 sequoiaFS::initMetaID(sdb *db)
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
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      rc = -ENOENT;
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_SEQUENCEID);
   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query collection, cl=%s, error=%d",
             SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_OK != rc)
   {
      if(SDB_DMS_EOC == rc)
      {
          obj = BSON(SEQUOIAFS_NAME<<SEQUOIAFS_SEQUENCEID<<\
              SEQUOIAFS_SEQUENCEID_VALAUE<<(INT64)(value + 1));
          ret = cl.insert(obj);
          if(SDB_OK != ret)
          {
           PD_LOG(PDERROR, "Fail to insert to collection, cl=%s, error=%d",
                  SEQUOIAFS_META_ID_CL_FULL.c_str(), ret);
           rc = ret;
           goto error;
          }
      }
      else
      {
          PD_LOG(PDERROR, "Error happened during cursor current, cl=%s, error=%d",
                 SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
          rc = -EIO;
          goto error;
      }
   }

   if(SDB_DMS_EOC != rc)
   {
      rc = getRecordField(record, (CHAR *)SEQUOIAFS_SEQUENCEID_VALAUE,
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

INT32 sequoiaFS::initRootPath()
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbCollection sysDirMetaCL;
   INT64 pid = 0;
   INT64 id = ROOT_ID;
   struct dirMetaNode dirNode;
   struct timeval tval;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   BSONObj options;
   INT32 pageSize = getpagesize();

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }
   
   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   gettimeofday(&tval, NULL);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   mtime = ctime;

   dirNode.name = "/";
   dirNode.mode = S_IFDIR | 0755;
   dirNode.uid = uid;
   dirNode.gid = gid;
   dirNode.nLink = 2;
   dirNode.pid= pid;
   dirNode.id = id;
   dirNode.size = pageSize;
   dirNode.ctime = ctime;
   dirNode.mtime = mtime;
   dirNode.atime= mtime;

   rc = doSetDirNodeAttr(sysDirMetaCL, dirNode);
   if(SDB_OK != rc)
   {
      if (SDB_IXM_DUP_KEY == rc)
      {
         rc = SDB_OK;
      }
      else
      {
         PD_LOG(PDERROR, "Failed to set attr, error=%d", rc);
         goto error;
      }
   }

done:
   releaseConnection(db);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::writeMapHistory(const CHAR *hosts)
{
   INT32 rc = SDB_OK;
   sdb *db;
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
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             SEQUOIAFS_META_MAP_AUDIT_CL_FULL.c_str(), rc);
      rc = -ENOENT;
      goto error;
   }

   ossGetCurrentTime(time);
   ossTimestampToString(time, timestampStr);
   builder.append(SEQUOIAFS_MOUNT_TIME, timestampStr);
   timeObj = builder.obj();

   rc = getLocalIPs(&localHosts);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get local ips, error=%d", rc);
      goto error;
   }

   obj = BSON(SEQUOIAFS_SRC_CLNAME<<_collection<<\
              SEQUOIAFS_DIR_META_CLNAME<<_sysDirMetaCLFullName<<\
              SEQUOIAFS_FILE_META_CLNAME<<_sysFileMetaCLFullName<<\
              SEQUOIAFS_ADDRESS<<localHosts<<\
              SEQUOIAFS_MOUNT_POINT<<_mountpoint<<\
              SEQUOIAFS_MOUNT_TIME<<timeObj);
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

INT32 sequoiaFS::init(INT32 argc, CHAR **argv, vector<string> *options4fuse)
{
   INT32 rc = SDB_OK;
   sdb *db;
   BSONObj idxDefObj;
   BSONObj emtpyObj;
   CHAR diaglogPath[OSS_MAX_PATHSIZE + 1] = {0};
   CHAR *tempDialogPath = NULL;
   CHAR verText[OSS_MAX_PATHSIZE + 1] = {0};
   const CHAR *nameIdx = "NameIndex";
   const CHAR *lobOidIdx = "LobOidIndex";
   sequoiafsOptionMgr *optionMgr = getOptionMgr();
   string configs;
   BSONObj options;
   INT32 capacity = 0;

   //1. init options
   rc = optionMgr->init(argc, argv, options4fuse);
   if(SDB_PMD_HELP_ONLY == rc || SDB_PMD_VERSION_ONLY == rc)
   {
      rc = SDB_OK;
      goto done;
   }

   else if(SDB_OK != rc)
   {
      ossPrintf("Failed to resolving arguments(error=%d), exit."OSS_NEWLINE, rc);
      goto error;
   }

   tempDialogPath = optionMgr->getDiaglogPath();
   setReplSize(optionMgr->replsize());

   //2. init and build diaglogPath
   rc = buildDialogPath(diaglogPath, tempDialogPath, OSS_MAX_PATHSIZE + 1);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to build dialog path(error=%d), eixt."OSS_NEWLINE, rc);
      goto error;
   }

   //./diaglog/sequoiafs.log
   rc = engine::utilCatPath(diaglogPath, OSS_MAX_PATHSIZE,
                            SDB_SEQUOIAFS_LOG_FILE_NAME);
   if(SDB_OK != rc)
   {
      ossPrintf("Failed to build dialog path(error=%d), exit."OSS_NEWLINE, rc);
      goto error;
   }

   sdbEnablePD(diaglogPath, optionMgr->getDiagMaxNUm());
   setPDLevel((PDLEVEL(optionMgr->getDiaglogLevel())));
   ossSprintVersion("Version", verText, OSS_MAX_PATHSIZE, FALSE);
   PD_LOG((getPDLevel()>PDEVENT)?PDEVENT:getPDLevel(),
           "Start sequoiafs[%s]...", verText);

   //print configuration in log file
   optionMgr->toString(configs);
   PD_LOG(PDEVENT, "ALL configs:\n%s", configs.c_str());

   //3. init datasource
   rc= initDataSource(optionMgr->getUserName(), \
              optionMgr->getPasswd(), \
              optionMgr->getConnNum());
   if(SDB_OK != rc)
   {
      closeDataSource();
      PD_LOG( PDERROR, "Failed to init connection pool, rc:%d", rc ) ;
      ossPrintf("Failed to init connection pool(error=%d), exit."OSS_NEWLINE, rc);
      goto done;
   }

   _collection = optionMgr->getCollection();

   //4. init data/meta cs and cl
   rc=getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG( PDERROR, "Failed to set the preferred instance for read request "
              "in the current session (PreferedInstance:M), rc:%d", rc ) ;
      ossPrintf("Failed to set the preferred instance for read request "
                "in the current session (PreferedInstance:M), error=%d"OSS_NEWLINE,
                rc);
      rc = -EIO;
      goto error;
   }

   rc = initDataCSCL(db, _collection);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init collection, cs.cl=%s, rc:%d", 
             _collection.c_str(), rc);
      ossPrintf("Failed to init collection, cs.cl=%s, error=%d, exit."OSS_NEWLINE,
                _collection.c_str(), rc);
      goto error;
   }

   _sysFileMetaCLFullName = optionMgr->getMetaFileCL();
   _sysDirMetaCLFullName = optionMgr->getMetaDirCL();

   rc = optionMgr->parseCollection(_sysDirMetaCLFullName, &_sysDirMetaCSName,
                                   &_sysDirMetaCLName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse dir meta collection, cs.cl=%s, rc:%d", 
             _sysDirMetaCLFullName.c_str(), rc);
      ossPrintf("Failed to parse dir meta collection, cs.cl=%s, error=%d, exit."OSS_NEWLINE,
                _sysDirMetaCLFullName.c_str(), rc);
      goto error;
   }

   rc = optionMgr->parseCollection(_sysFileMetaCLFullName,
                                   &_sysFileMetaCSName, &_sysFileMetaCLName);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to parse file meta collection, cs.cl=%s, rc:%d", 
             _sysFileMetaCLFullName.c_str(), rc);
      ossPrintf("Failed to parse file meta collection, cs.cl=%s, error=%d, exit."OSS_NEWLINE,
                _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }
   idxDefObj = BSON(SEQUOIAFS_NAME << 1 << SEQUOIAFS_PID << 1);
   rc = initMetaCSCL(db, _sysDirMetaCSName, _sysDirMetaCLName, nameIdx,
                     TRUE, idxDefObj, TRUE, TRUE);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init dir meta collection, cs.cl=%s.%s, rc:%d", 
             _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
      ossPrintf("Failed to init dir meta collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                _sysDirMetaCSName.c_str(), _sysDirMetaCLName.c_str(), rc);
      goto error;
   }

   rc = initRootPath();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init root path, rc:%d", rc);
      ossPrintf("Failed to init root path, error=%d, exit."OSS_NEWLINE, rc);
      goto error;
   }

   rc = initMetaCSCL(db, _sysFileMetaCSName, _sysFileMetaCLName, nameIdx,
                     TRUE, idxDefObj, TRUE, TRUE);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init file meta collection, cs.cl=%s.%s, rc:%d", 
             _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
      ossPrintf("Failed to init file meta collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
      goto error;
   }

   idxDefObj = BSON(SEQUOIAFS_LOBOID << 1);
   rc = initMetaCSCL(db, _sysFileMetaCSName, _sysFileMetaCLName, lobOidIdx,
                     TRUE, idxDefObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init file meta collection, cs.cl=%s.%s, rc:%d", 
             _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
      ossPrintf("Failed to init file meta collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                _sysFileMetaCSName.c_str(), _sysFileMetaCLName.c_str(), rc);
      goto error;
   }

   rc = initMetaCSCL(db, SEQUOIAFS_META_CS, SEQUOIAFS_META_MAP_AUDIT_CL,
                     "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create maphistory collection, cs.cl=%s.%s, rc:%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_MAP_AUDIT_CL.c_str(), rc);
      ossPrintf("Failed to create maphistory collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_MAP_AUDIT_CL.c_str(), rc);
      goto error;
   }

   rc = initMetaCSCL(db, SEQUOIAFS_META_CS, SEQUOIAFS_META_ID_CL, "", FALSE, emtpyObj);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create sequenceid collection, cs.cl=%s.%s, rc:%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to create sequenceid collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   rc = initMetaID(db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to init sequenceid collection, cs.cl=%s.%s, rc:%d", 
             SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      ossPrintf("Failed to init sequenceid collection, cs.cl=%s.%s, error=%d, exit."OSS_NEWLINE,
                SEQUOIAFS_META_CS.c_str(), SEQUOIAFS_META_ID_CL.c_str(), rc);
      goto error;
   }

   capacity = optionMgr->getCacheSize() * 1024 * 1024 / sizeof(struct dirMetaNode);
   //5. init lru cache
   InitLruCace(capacity);
   pthread_mutex_init(&mutex, NULL);
done:
   releaseConnection(db);
   return rc;

error:
   goto done;

}

INT32 sequoiaFS::isDir(sdbCollection *sysFileMetaCL,
                       sdbCollection *sysDirMetaCL,
                       CHAR *name, INT64 pid, BOOLEAN *is_dir)
{
   INT32 rc = SDB_OK;
   BSONObj rule;
   BSONObj condition;
   BSONObj record;
   BSONElement ele;
   sdbCursor cursor;

   *is_dir = TRUE;
   condition = BSON(SEQUOIAFS_NAME<<name<<SEQUOIAFS_PID<<(INT64)pid);
   rc = sysDirMetaCL->query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file or directory, name=%s, error=%d",
             name, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_DMS_EOC == rc )
   {
      rc = sysFileMetaCL->query(cursor, condition);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to query query file or directory, "
                 "name=%s, error=%d", name, rc);
          rc = -EIO;
          goto error;
      }

      rc = cursor.current(record);
      if(SDB_DMS_EOC == rc )
      {
          PD_LOG(PDINFO, "Such file or directory does not exist, name=%s, "
                 "error=%d", name, rc);
          rc = -ENOENT;
          goto error;
      }

      else if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, "
                 "error=%d", name, rc);
          rc = -EIO;
          goto error;
      }

      *is_dir = FALSE;

   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get such file or directory, name=%s, "
             "error=%d", name, rc);
      rc = -EIO;
      goto error;
   }
done:
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
          rc = -EIO;
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
          rc = -EIO;
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
          rc = -EIO;
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
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      rc = -EIO;
      goto error;
   }

done:
   return rc;
error:
   goto done;
}

INT32 sequoiaFS::doSetFileNodeAttr(sdbCollection &cl,
                                   struct fileMetaNode &fileNode)
{
   INT32 rc = SDB_OK;
   BSONObj obj;

   obj = BSON(SEQUOIAFS_NAME<<fileNode.name<<\
              SEQUOIAFS_MODE<<fileNode.mode<<\
              SEQUOIAFS_UID<<fileNode.uid<<\
              SEQUOIAFS_GID<<fileNode.gid<<\
              SEQUOIAFS_NLINK<<fileNode.nLink<<\
              SEQUOIAFS_PID<<fileNode.pid<<\
              SEQUOIAFS_LOBOID<<fileNode.lobOid<<\
              SEQUOIAFS_SIZE<<fileNode.size<<\
              SEQUOIAFS_CREATE_TIME<<fileNode.ctime<<\
              SEQUOIAFS_MODIFY_TIME<<fileNode.mtime<<\
              SEQUOIAFS_ACCESS_TIME<<fileNode.atime<<\
              SEQUOIAFS_SYMLINK<<fileNode.symLink);

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

INT32 sequoiaFS::doesFileExist(sdbCollection &cl,
                               const CHAR *lobName,
                               BSONObj &condition,
                               BOOLEAN *exist, OID *oid, BSONObj &record)
{
   INT32 rc = SDB_OK;
   BSONElement ele;
   INT32 count = 0;
   sdbCursor cursor;

   rc = cl.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", lobName, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   while(SDB_DMS_EOC != rc)
   {
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Error happened during do cursor next, error=%d", rc);
          rc = -EIO;
          goto error;
      }
      else
      {
          count++;
          ele = record.getField("LobOid");
          if(bson::String != ele.type())
          {
           PD_LOG(PDERROR, "Invalid type of oid");
           rc = -EIO;
           goto error;
          }
          if(!ele.String().empty())
           *oid = bson::OID(ele.String());
          rc = cursor.next(record);
      }
   }

   if(1 <= count)
   {
      *exist = TRUE;
   }
   rc = SDB_OK;
done:
   return rc;

error:
   goto done;

}


INT32 sequoiaFS::doSetDirNodeAttr(sdbCollection &cl, struct dirMetaNode &dirNode)
{
   INT32 rc = SDB_OK;
   BSONObj obj;

   obj = BSON(SEQUOIAFS_NAME<<dirNode.name<<\
          SEQUOIAFS_MODE<<dirNode.mode<<\
          SEQUOIAFS_UID<<dirNode.uid<<\
          SEQUOIAFS_GID<<dirNode.gid<<\
          SEQUOIAFS_PID<<dirNode.pid<<\
          SEQUOIAFS_ID<<dirNode.id<<\
          SEQUOIAFS_NLINK<<dirNode.nLink<<\
          SEQUOIAFS_SIZE<<dirNode.size<<\
          SEQUOIAFS_CREATE_TIME<<dirNode.ctime<<\
          SEQUOIAFS_MODIFY_TIME<<dirNode.mtime<<\
          SEQUOIAFS_ACCESS_TIME<<dirNode.atime<<\
          SEQUOIAFS_SYMLINK<<dirNode.symLink);

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

INT64 sequoiaFS::getDirIno(sdbCollection *sysDirMetaCL,
                           string dirname, INT64 pid)
{
   INT64 id = 0;
   INT64 rc = SDB_OK;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   BOOLEAN exist = TRUE;
   INT64 hash = 1;
   struct dirMetaNode dirNode;
   INIT_DIR_NODE(dirNode);

   condition = BSON(SEQUOIAFS_NAME<<dirname<<SEQUOIAFS_PID<<(INT64)pid);
   rc = sysDirMetaCL->query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query directory, dir=%s, error=%d",
             dirname.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_DMS_EOC == rc)
      exist = FALSE;

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, dir=%s, "
             "error=%d", dirname.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   if(!exist)
   {
      PD_LOG(PDERROR, "The directory does not exist, dir=%s, error=%d",
             dirname.c_str(), rc);
      rc = -ENOENT;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_ID, (void *)(&id), NumberLong);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get id of directory, dir=%s, error=%d",
             dirname.c_str(), rc);
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SIZE,
                       (void *)(&dirNode.size), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_CREATE_TIME,
                        (void *)(&dirNode.ctime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_ACCESS_TIME,
                        (void *)(&dirNode.atime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODIFY_TIME,
                        (void *)(&dirNode.mtime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODE,
                        (void *)(&dirNode.mode), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_UID,
                        (void *)(&dirNode.uid), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_GID,
                        (void *)(&dirNode.gid), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_SYMLINK,
                        (void *)(&dirNode.symLink), String);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get attr of directory, dir=%s, error=%d",
             dirname.c_str(), rc);
      goto error;
   }

   dirNode.name = dirname;
   dirNode.id = id;
   dirNode.pid = pid;

   hash = namePidHash(pid, dirname.c_str());
   lrucache->put(hash, &dirNode);

   rc = id;
done:
   return rc;
error:
   goto done;
}

INT64 sequoiaFS::getDirPIno(sdbCollection *sysDirMetaCL,
                            CHAR *pathStr, string *basePath, bool flag)
{
   CHAR *dir;
   INT64 pid = 0;
   const CHAR *delimiter = "/";
   CHAR *pTemp = NULL;
   vector<string> dirName;
   PD_LOG(PDDEBUG, "All nodes path:%s", pathStr);
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

   pid = ROOT_ID;

   for(vector<string>::size_type i = 0; i < dirName.size() - 1; i++)
   {
      size_t hash = namePidHash(pid, dirName[i].c_str());
      struct dirMetaNode* node= lrucache->get(hash);
      if(node)
      {
         pid = node->id;
         continue;
      }
      pid = getDirIno(sysDirMetaCL, dirName[i], pid);
      if(pid < 0)
      {
         goto error;
      }
   }

   *basePath = dirName[dirName.size() - 1];
done:
   return pid;
error:
   goto done;
}

INT32 sequoiaFS::getattr(const CHAR *path, struct stat *sbuf)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   string basePath;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   uid_t uid = getuid();
   gid_t gid = getgid();
   UINT32 mode = 0;
   BSONObj record;
   BSONElement ele;
   UINT32 nlink = 0;
   INT64 pid = 0;
   BOOLEAN is_dir =TRUE;
   BSONObj options;
   INT32 blocks;
   INT32 pageSize = getpagesize();

   PD_LOG(PDDEBUG, "Called: getattr(). Path:%s", path);
   ossMemset(sbuf, 0, sizeof(struct stat));

   if(!enableDataSource)
   {
      rc = ds.enable();
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Fail to enable sdbDataSource, error=%d, exit", rc);
          rc = -EIO;
          goto error;
      }
      enableDataSource = TRUE;
   }

   sbuf->st_uid = uid;
   sbuf->st_gid = gid;
   pathStr = ossStrdup(path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<basePath<<SEQUOIAFS_PID<<pid);
   //If didn't get the record from sys file meta cl, then search the sys dir meta cl.
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, (CHAR *)basePath.c_str(),
              pid, &is_dir);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = (is_dir?sysDirMetaCL:sysFileMetaCL).query(*cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query file or dirctory, name=%s, error=%d",
             basePath.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor->current(record);
   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "No such file or directory, name=%s, error=%d",
             basePath.c_str(), rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, error=%d",
             basePath.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   nlink = 0;
   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SIZE,
                       (void *)(&sbuf->st_size), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_CREATE_TIME,
                        (void *)(&sbuf->st_ctime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_ACCESS_TIME,
                        (void *)(&sbuf->st_atime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODIFY_TIME,
                        (void *)(&sbuf->st_mtime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODE,
                        (void *)(&mode), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_NLINK,
                        (void *)(&nlink), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_UID,
                        (void *)(&sbuf->st_uid), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_GID,
                        (void *)(&sbuf->st_gid), NumberInt);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get attr, name=%s, error=%d", basePath.c_str(), rc);
      goto error;
   }

   sbuf->st_mode = mode;
   sbuf->st_nlink = nlink;
   sbuf->st_ctime /= 1000;
   sbuf->st_mtime /= 1000;
   sbuf->st_atime /= 1000;
   blocks = sbuf->st_size / pageSize;
   blocks = (sbuf->st_size % pageSize == 0) ? blocks : blocks + 1;
   sbuf->st_blocks = blocks * 8;
   rc = SDB_OK;

done:
   delete cursor;
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::readlink(const CHAR *path, CHAR * link, size_t size)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *name = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   BSONElement ele;
   string linkName;
   INT64 pid = 1;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: readlink(), path:%s", path);

   pathStr = ossStrdup(path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Fail to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<basePath<<SEQUOIAFS_PID<<(INT64)pid);
   name = (CHAR *)basePath.c_str();

   rc = sysFileMetaCL.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query file, name=%s, error=%d", name, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);

   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "File does not exist, name=%s, error=%d", name, rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, "
             "error=%d", name, rc);
      rc = -EIO;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SYMLINK, (void *)(&linkName), String);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get linkname, name=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      goto error;
   }

   ossMemcpy(link, linkName.c_str(), ossStrlen(linkName.c_str()));
   rc = SDB_OK;

done:
   releaseConnection(db);
   SDB_OSS_FREE(pathStr);
   return rc;

error:
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

INT32 sequoiaFS::mkdir(const CHAR *path, mode_t mode)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbLob lob;
   sdbCollection cl;
   sdbCollection sequenceCl;
   sdbCollection sysDirMetaCL;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj record;
   BSONObj rule;
   OID oid;
   BSONObj obj;
   BOOLEAN exist = TRUE;
   struct dirMetaNode dirNode;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct timeval tval;
   INT64 id = 0;
   INT64 pid = 0;
   string basePath;
   vector<string> dirName;
   CHAR *pathStr = NULL;
   INIT_DIR_NODE(dirNode);
   size_t hash;
   BSONObj options;

   pathStr = ossStrdup(path);

   PD_LOG(PDDEBUG, "Called: mkdir(), path:%s, mode:%u", path, mode);

   rc = getConnection(&db);
   if(SDB_OK != rc)
      goto error;

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(SEQUOIAFS_META_ID_CL_FULL.c_str(), sequenceCl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             SEQUOIAFS_META_ID_CL_FULL.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = getAndUpdateID(&sequenceCl, &id);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get and update id in sequence collecion, "
             "error=%d", rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Fail to get pid of directory, name=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<basePath<<SEQUOIAFS_PID<<(INT64)pid);
   rc = sysDirMetaCL.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to query directory, name=%s, error=%d",
             basePath.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_DMS_EOC == rc)
   {
      exist = FALSE;
   }
   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, "
             "error=%d", basePath.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   if(exist)
   {
      PD_LOG(PDERROR, "The directory has alread existed, name=%s",
             basePath.c_str());
      rc = -EEXIST;
      goto error;
   }

   gettimeofday(&tval, NULL);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   mtime = ctime;

   dirNode.name = basePath;
   dirNode.mode = S_IFDIR | mode;
   dirNode.uid = uid;
   dirNode.gid = gid;
   dirNode.nLink = 2;
   dirNode.pid= pid;
   dirNode.id = id;
   dirNode.size = 4096;
   dirNode.ctime = ctime;
   dirNode.mtime = mtime;
   dirNode.atime= mtime;

   rc = doSetDirNodeAttr(sysDirMetaCL, dirNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set attr, error=%d", rc);
      goto error;
   }

   /*add subdir no on parent dir*/
   condition = BSON(SEQUOIAFS_ID<<(INT64)pid);
   rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<1));
   rc = doUpdateAttr(&sysDirMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
      goto error;
   }

   hash = namePidHash(dirNode.pid, dirNode.name.c_str());
   lrucache->put(hash, &dirNode);

done:
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::unlink(const CHAR *path)
{
   INT32 rc = SDB_OK;
   sdb *db;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollection cl;
   BOOLEAN exist = FALSE;
   OID oid;
   CHAR *pathStr = NULL;
   BSONObj condition;
   BSONObj record;
   BSONObj rule;
   UINT32 mode = 0;
   UINT32 newmode = 0;
   UINT32 nlink = 0;
   string lobOid;
   string basePath;
   INT64 pid = 0;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: unlink(), path:%s", path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get connection, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _collection.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pathStr = ossStrdup(path);
   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();

   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)pid);
   rc = doesFileExist(sysFileMetaCL, fileName, condition, &exist, &oid, record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection for file, name=%s, cl=%s, "
             "error=%d", fileName, _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   if(!exist)
   {
      PD_LOG(PDERROR, "File does not exist, name=%s", fileName);
      rc = -ENOENT;
      goto error;
   }

   rc =  sysFileMetaCL.del(condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to delete file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_MODE,
                       (void *)(&mode), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_NLINK,
                        (void *)(&nlink), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_LOBOID,
                        (void *)(&lobOid), String);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get attr, error=%d", rc);
      goto error;
   }

   //is link file
   if(S_ISLNK(mode))
   {
      rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<-1));
      newmode = S_IFREG | 0644;
      condition = BSON("$and"<<BSON_ARRAY(BSON(SEQUOIAFS_LOBOID<<oid)<<\
                       BSON(SEQUOIAFS_MODE<<newmode)));
      rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
      if(SDB_OK != rc)
      {
          goto error;
      }
   }

   //is regular file
   if(S_ISREG(mode))
   {
      if(nlink > 1)
      {
          condition = BSON(SEQUOIAFS_LOBOID<<lobOid);
          rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<-1));
          rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
          if(SDB_OK != rc)
          {
           PD_LOG(PDERROR, "Failed to update attr, error=%d", rc);
           rc = -EIO;
           goto error;
          }
          goto done;
      }

      rc = cl.removeLob(oid);
      //if lob didnot exist, ignore the error
      if(SDB_FNE == rc)
      {
          PD_LOG(PDWARNING, "Lob for file has alread removed, filename=%s, "
                 "oid:%s, error=%d", fileName, oid.toString().c_str(), rc);
          rc = SDB_OK;
      }
      else if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to remove lob for file, filename=%s, oid:%s, "
                 "error=%d", fileName, oid.toString().c_str(), rc);
          rc = -EIO;
          goto error;
      }
   }

   PD_LOG(PDDEBUG, "Finish to unlink file:%s", path);
done:
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;

}
INT32 sequoiaFS::rmdir(const CHAR *path)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   sdbLob lob;
   sdbCollection cl;
   sdbCollection sysDirMetaCL;
   sdbCollection sysFileMetaCL;
   BSONObj condition;
   BSONObj record;
   OID oid;
   BSONObj obj;
   INT64 pid = 0;
   string basePath;
   vector<string> dirName;
   CHAR *pathStr = NULL;
   BSONObj options;
   INT64 id;
   sdbCursor *tempCursor;
   sdbCursor *cursor[2];
   INT32 i = 0;

   cursor[0] = new sdbCursor();
   cursor[1] = new sdbCursor();
   pathStr = ossStrdup(path);

   PD_LOG(PDDEBUG, "Called: rmdir(), path:%s", path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
      goto error;

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }


   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }

   if(basePath == "/")
   {
      id = 1;
   }
   else
   {
      //get the ino of the dir
      id = getDirIno(&sysDirMetaCL, basePath, pid);
   }

   //look for files in the dir based on the ino of the dir
   condition = BSON(SEQUOIAFS_PID<<id);
   rc = sysDirMetaCL.query(*(cursor[0]), condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = sysFileMetaCL.query(*(cursor[1]), condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   for(i = 0; i < NUM_OF_META_CL; i++)
   {
      tempCursor = cursor[i];
      if(SDB_OK == tempCursor->next(record))
      {
          PD_LOG(PDERROR, "Failed to remove dir:%s, directory not empty",
                 basePath.c_str());
          rc = -ENOTEMPTY;
          goto error;
      }
   }

   condition = BSON(SEQUOIAFS_NAME<<basePath<<SEQUOIAFS_PID<<(INT64)pid);
   rc =  sysDirMetaCL.del(condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to delete dir:%s, error=%d",
             basePath.c_str(), rc);
      rc = -EIO;
      goto error;
   }
done:
   delete cursor[0];
   delete cursor[1];
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;

}

INT32 sequoiaFS::symlink(const CHAR *path, const CHAR *link)
{
   INT32 rc = SDB_OK;
   sdb *db;
   CHAR *linkName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   BOOLEAN exist = FALSE;
   OID oid;
   CHAR *linkStr = NULL;
   BSONObj condition;
   struct fileMetaNode fileNode;
   BSONObj record;
   BSONObj rule;
   BSONObj obj;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct timeval tval;
   INT64 pid = 1;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: symlink(), path:%s, link:%s", path, link);
   //INIT_NODE(lobNode);
   INIT_FILE_NODE(fileNode);

   linkStr = ossStrdup(link);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get connection, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, linkStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to pid of directory, dir=%s, error=%d",
             linkStr, pid);
      rc = pid;
      goto error;
   }
   linkName = (CHAR *)basePath.c_str();
   condition = BSON(SEQUOIAFS_NAME<<linkName<<SEQUOIAFS_PID<<pid);

   rc = doesFileExist(sysFileMetaCL, linkName, condition, &exist, &oid, record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, file=%s, error=%d", linkName, rc);
      rc = -EIO;
      goto error;
   }

   if(exist)
   {
      PD_LOG(PDERROR, "Failed to create symlink:%s, it does exist", linkName);
      rc = -EEXIST;
      goto error;

   }

   ossGetTimeOfDay(&tval);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   mtime = ctime;

   fileNode.name = linkName;
   fileNode.mode = S_IFLNK | 0777;//41471  S_IFLNK|06444-41380
   fileNode.nLink= 1;
   fileNode.pid = pid;
   fileNode.lobOid = "";
   fileNode.ctime= ctime;
   fileNode.mtime= mtime;
   fileNode.atime= mtime;
   fileNode.uid = uid;
   fileNode.gid = gid;
   fileNode.size = ossStrlen(path);
   fileNode.symLink = path;

   rc = doSetFileNodeAttr(sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to insert symlink, error=%d", rc);
      rc = -EIO;
      goto error;
   }

done:
   SDB_OSS_FREE(linkStr);
   releaseConnection(db);
   return rc;

error:
   goto done;

}


INT32 sequoiaFS::rename(const CHAR *path, const CHAR *newpath)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   CHAR *fileNewName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   CHAR *pathStr = NULL;
   CHAR *newPathStr = NULL;
   BSONObj rule;
   BSONObj condition;
   BSONObj cond;
   string basePath;
   string newbasePath;
   INT64 pid =0;
   INT64 newPino = 0;
   BSONObj record;
   BSONElement ele;
   sdbCursor cursor;
   BOOLEAN is_dir = true;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: rename(), path:%s, newpath:%s", path, newpath);
   pathStr = ossStrdup(path);
   newPathStr = ossStrdup(newpath);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }
   fileNewName = basename(newPathStr);
   newPino = getDirPIno(&sysDirMetaCL,newPathStr, &newbasePath);
   if(newPino < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of new directory, dir=%s, error=%d",
             newPathStr, newPino);
      rc = newPino;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of dorectory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   rule = BSON("$set"<<BSON(SEQUOIAFS_NAME<<fileNewName<<SEQUOIAFS_PID<<newPino));
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)pid);
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, fileName, pid, &is_dir);
   if(SDB_OK != rc)
      goto error;

   //if it is a file, check the newfile exists or not, if exist, should delete it first
   if(!is_dir)
   {
      cond = BSON(SEQUOIAFS_NAME<<fileNewName<<SEQUOIAFS_PID<<(INT64)newPino);
      rc = sysFileMetaCL.query(cursor, cond);
      if(SDB_OK != rc)
      {
         PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", fileName, rc);
         rc = -EIO;
         goto error;
      }

      rc = cursor.current(record);
      if(SDB_OK == rc)
       {
         rc = unlink(newpath);
         if(SDB_OK != rc)
          {
            PD_LOG(PDERROR, "Failed to remove file, name=%s, error=%d", newpath, rc);
            goto error;
         }
      }
   }

   rc = doUpdateAttr(is_dir ? &sysDirMetaCL : &sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   SDB_OSS_FREE(newPathStr);
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;
}


INT32 sequoiaFS::link(const CHAR *path, const CHAR *newpath)
{
   INT32 rc = SDB_OK;
   sdb *db;
   CHAR *linkName = NULL;
   CHAR *fileName = NULL;
   CHAR *pathStr = NULL;
   sdbCursor cursor;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   OID oid;
   CHAR *linkStr = NULL;
   BSONObj condition;
   struct fileMetaNode fileNode;
   BSONObj record;
   BSONObj rule;
   BSONObj obj;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   struct timeval tval;
   INT64 pid = 1;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: link(), path:%s, link:%s", path, newpath);
   INIT_FILE_NODE(fileNode);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }


   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pathStr = ossStrdup(path);
   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);

   rc = sysFileMetaCL.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);
   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "File does not exist, name=%s, error=%d", fileName, rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, error=%d",
             fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SIZE,
                       (void *)(&fileNode.size), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_CREATE_TIME,
                       (void *)(&fileNode.ctime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODIFY_TIME,
                        (void *)(&fileNode.mtime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_ACCESS_TIME,
                        (void *)(&fileNode.atime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODE,
                        (void *)(&fileNode.mode), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_NLINK,
                        (void *)(&fileNode.nLink), NumberInt);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_LOBOID,
                        (void *)(&fileNode.lobOid), String);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get attr on file:%s, error=%d", fileName, rc);
      goto error;
   }

   linkStr = ossStrdup(newpath);
   linkName = basename(linkStr);

   gettimeofday(&tval, NULL);
   ctime = tval.tv_sec * 1000 + tval.tv_usec/1000;
   mtime = ctime;

   fileNode.name = linkName;
   fileNode.mode = S_IFREG | 0755;//33188
   fileNode.ctime= ctime;
   fileNode.mtime= mtime;
   fileNode.atime= mtime;
   fileNode.uid = uid;
   fileNode.gid = gid;
   fileNode.pid = pid;

   rc = doSetFileNodeAttr(sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set attr on file:%s, error=%d", pathStr, rc);
      goto error;
   }

   condition = BSON(SEQUOIAFS_LOBOID<<fileNode.lobOid);
   rule = BSON("$inc"<<BSON(SEQUOIAFS_NLINK<<1));
   rc = doUpdateAttr(&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }


done:
   SDB_OSS_FREE(pathStr);
   SDB_OSS_FREE(linkStr);
   releaseConnection(db);
   return rc;

error:
   goto done;


}

INT32 sequoiaFS::chmod(const CHAR *path, mode_t mode)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   BSONObj rule;
   INT64 pid = 0;
   BOOLEAN is_dir = TRUE;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: chmod(), path:%s, mode:%u", path, mode);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, fileName, pid, &is_dir);
   if(SDB_OK != rc)
   {
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);
   rule = BSON("$set"<<BSON(SEQUOIAFS_MODE<<mode));
   rc = doUpdateAttr(is_dir?&sysDirMetaCL:&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = SDB_OK;

done:
   SDB_OSS_FREE(pathStr);
   delete cursor;
   releaseConnection(db);
   return rc;

error:
   goto done;

}

INT32 sequoiaFS::chown(const CHAR *path, uid_t uid, gid_t gid)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   BSONObj rule;
   INT64 pid = 0;
   BOOLEAN is_dir = TRUE;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: chown(), path:%s, uid:%d, gid:%d", path, uid, gid);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, fileName, pid, &is_dir);
   if(SDB_OK != rc)
   {
      goto error;
   }
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);
   rule = BSON("$set"<<BSON(SEQUOIAFS_UID<<uid<<SEQUOIAFS_GID<<gid));
   rc = doUpdateAttr(is_dir?&sysDirMetaCL:&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = SDB_OK;

done:
   SDB_OSS_FREE(pathStr);
   delete cursor;
   releaseConnection(db);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::truncate(const CHAR *path, off_t newsize)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor cursor;
   BSONObj rule;
   INT64 pid = 0;
   string basePath;
   BSONObj record;
   BSONElement ele;
   sdbLob lob;
   sdbCollection cl;
   BSONObj options;
   OID oid;
   lobHandle *lh = new lobHandle;
   struct fuse_file_info *fi = new fuse_file_info;

   INIT_LOBHANDLE(lh);

   PD_LOG(PDDEBUG, "Called: truncate(), path:%s", path);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);
   rc = sysFileMetaCL.query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);

   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "Failed to get file, name=%s, it does not exist, "
             "error=%d", fileName, rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during cursor current, name=%s, "
             "error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }
   ele = record.getField(SEQUOIAFS_LOBOID);
   if(bson::String != ele.type())
   {
      PD_LOG(PDERROR, "Invalid type of oid, the type is not string");
      rc = -EIO;
      goto error;
   }

   if(ele.String().empty())
   {
      PD_LOG(PDERROR, "The oid is null");
      rc = -EIO;
      goto error;
   }

   oid = bson::OID(ele.String());
   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _collection.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   //open lob according to the mode of open
   rc = cl.openLob(lob, oid, SDB_LOB_WRITE);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob for file, name=%s, error=%d",
             fileName, rc);
      rc = -EIO;
      goto error;
   }

   lh->hLob = &lob;
   lh->hSdb = db;
   lh->hSysFileMetaCL= &sysFileMetaCL;
   fi->fh = (intptr_t)(uint64_t)((void *)lh);
   //pthread_mutex_init(&lh->lock, NULL);

   ftruncate(path, newsize, fi);

done:
   releaseConnection(db);
   SDB_OSS_FREE(pathStr);
   delete fi;
   return rc;

error:
   goto done;
}


/*change the access and modification times of lob*/
INT32 sequoiaFS::utime(const CHAR *path, struct utimbuf * ubuf)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   BSONObj rule;
   INT64 pid = 0;
   BOOLEAN is_dir = TRUE;
   string basePath;
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: utime(), path:%s, actime:%d, modtime:%d",
          path, ubuf->actime, ubuf->modtime);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, fileName, pid, &is_dir);
   if(SDB_OK != rc)
   {
      goto error;
   }

   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);
   rule = BSON("$set"<<BSON(SEQUOIAFS_MODIFY_TIME<<(INT64)ubuf->modtime<<\
               SEQUOIAFS_CREATE_TIME<<(INT64)ubuf->actime<<\
               SEQUOIAFS_ACCESS_TIME<<(INT64)ubuf->actime));
   rc = doUpdateAttr(is_dir?&sysDirMetaCL:&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = SDB_OK;

done:
   delete cursor;
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::open(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   BSONObj record;
   sdbCursor cursor;
   BSONElement ele;
   sdbLob *lob= new sdbLob();
   sdbCollection cl;
   sdbCollectionSpace cs;
   sdbCollection *sysFileMetaCL = new sdbCollection();
   sdbCollection *sysDirMetaCL = new sdbCollection();
   BSONObj condition;
   OID oid;
   INT64 pid = 0;
   BSONObj obj;
   string basePath;
   SDB_LOB_OPEN_MODE mode = SDB_LOB_READ;
   CHAR *pathStr = NULL;
   lobHandle *lh = new lobHandle;
   BSONObj options;

   INIT_LOBHANDLE(lh);
   PD_LOG(PDDEBUG, "Called: open(), path:%s, flags:%d", path, fi->flags);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   if(fi->flags & O_WRONLY)
   {
      mode = SDB_LOB_WRITE;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), *sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), *sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)pid);
   rc = sysFileMetaCL->query(cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor.current(record);

   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "Failed to get file, name=%s, it does not exist, error=%d",
             fileName, rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, error=%d",
             fileName, rc);
      rc = -EIO;
      goto error;
   }
   ele = record.getField(SEQUOIAFS_LOBOID);
   if(bson::String != ele.type())
   {
      PD_LOG(PDERROR, "Invalid type of oid, the type of oid is not string");
      rc = -EIO;
      goto error;
   }

   if(ele.String().empty())
   {
      PD_LOG(PDERROR, "The oid is null");
      rc = -EIO;
      goto error;
   }

   oid = bson::OID(ele.String());
   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _collection.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   //open lob according to the mode of open
   rc = cl.openLob(*lob, oid, mode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob for file, name=%s, error=%d",
             fileName, rc);
      rc = -EIO;
      goto error;
   }
   lh->hLob = lob;
   lh->hSdb = db;
   lh->oid = oid;
   lh->hSysFileMetaCL= sysFileMetaCL;
   fi->fh = (intptr_t)(uint64_t)((void *)lh);
   pthread_mutex_init(&lh->lock, NULL);
   if(fi->flags & O_RDWR)
   {
      pthread_mutex_lock(&mutex);
      _mapOpMode.insert(std::pair<UINT64, INT8>(fi->fh, 1));
      pthread_mutex_unlock(&mutex);
   }

done:
   SDB_OSS_FREE(pathStr);
   delete sysDirMetaCL;
   return rc;

error:
   delete lh;
   delete lob;
   delete sysFileMetaCL;
   releaseConnection(db);
   goto done;
}

INT32 sequoiaFS::read(const CHAR *path, CHAR *buf, size_t size, off_t offset ,
                      struct fuse_file_info *fi)
{
   sdb *db = NULL;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   UINT32 readlen = 0;
   sdbLob *lob = NULL;
   lobHandle *lh = NULL;
   std::map<UINT64, INT8>::iterator it;
   bson::BSONObj sessionAttr;

   PD_LOG(PDDEBUG, "Called: read(), path:%s, offset:%d, size:%d",
          path, offset, size);

   lh = (lobHandle *)fi->fh;
   lob = (sdbLob *)lh->hLob;
   db = lh->hSdb;

   pthread_mutex_lock(&mutex);
   it = _mapOpMode.find(fi->fh);
   if( it != _mapOpMode.end())
   {
      if(it->second != 1)
      {
         rc = lob->close();
         if(rc != SDB_OK)
         {
            PD_LOG(PDERROR, "Failed to close lob, error=%d", rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }

         rc = db->getCollection(_collection.c_str(), cl);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
                   _collection.c_str(), rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }

         rc = cl.openLob(*lob, lh->oid, SDB_LOB_READ);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to open lob for file, name=%s, error=%d",
                   path, rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }
         lh->hLob = lob;
         _mapOpMode.erase(it);
         _mapOpMode.insert(std::pair<UINT64, INT8>(fi->fh, 1));
      }
   }
   pthread_mutex_unlock(&mutex);

   pthread_mutex_lock(&lh->lock);
   rc = lob->seek(offset, SDB_LOB_SEEK_SET);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to seek file, name=%s, error=%d", path, rc);
      rc = -EIO;
      goto error;
   }
   rc = lob->read(size, buf, &readlen);
   if(SDB_EOF == rc)
   {
      PD_LOG(PDDEBUG, "Reach the end of the file, name=%s, readlen=%d",
             path, readlen);
      rc = SDB_OK;
      goto done;
   }
   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to read file, name=%s, error=%d", path, rc);
      rc = -EIO;
      goto error;
   }
   PD_LOG(PDDEBUG, "Succeed to read size=%d", readlen);
   rc = (INT32)readlen;

done:
   pthread_mutex_unlock(&lh->lock);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::write(const CHAR *path,
                       const CHAR *buf,
                       size_t size,
                       off_t offset,
                       struct fuse_file_info *fi)
{
   sdb *db = NULL;
   sdbCollection cl;
   INT32 rc = SDB_OK;
   SINT64 lobSize = 0;
   sdbLob *lob = NULL;
   lobHandle *lh = NULL;
   sdbCollection *sysFileMetaCL = NULL;
   BSONObj rule;
   OID oid;
   BSONObj condition;
   UINT64 mtime = 0;
   std::map<UINT64, INT8>::iterator it;

   PD_LOG(PDDEBUG, "Called: write(), path:%s, offset:%d, size:%d",
          path, offset, size);

   lh = (lobHandle *)fi->fh;
   lob = (sdbLob *)lh->hLob;
   db = lh->hSdb;
   sysFileMetaCL = (sdbCollection *)lh->hSysFileMetaCL;

   pthread_mutex_lock(&mutex);
   it = _mapOpMode.find(fi->fh);
   if( it != _mapOpMode.end())
   {
      if(it->second != 2)
      {
         rc = lob->close();
         if(rc != SDB_OK)
         {
            PD_LOG(PDERROR, "Failed to close lob, error=%d", rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }

         rc = db->getCollection(_collection.c_str(), cl);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
                   _collection.c_str(), rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }

         rc = cl.openLob(*lob, lh->oid, SDB_LOB_WRITE);
         if(SDB_OK != rc)
         {
            PD_LOG(PDERROR, "Failed to open lob for file, name=%s, error=%d",
                   path, rc);
            rc = -EIO;
            pthread_mutex_unlock(&mutex);
            goto error;
         }
         lh->hLob = lob;
         _mapOpMode.erase(it);
         _mapOpMode.insert(std::pair<UINT64, INT8>(fi->fh, 2));
      }
   }
   pthread_mutex_unlock(&mutex);

   //pthread_mutex_lock(&lh->lock);
   rc = lob->lockAndSeek(offset, size);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to lockAndseek lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }
   rc = lob->write(buf, (UINT32)size);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to write lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }
   //pthread_mutex_unlock(&lh->lock);
   rc = lob->getSize(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get size, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   mtime = lob->getModificationTime();
   if(-1 == (SINT64)mtime)
   {
      PD_LOG(PDERROR, "Failed to get modification time, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = lob->getOid(oid);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get lob oid, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<lobSize<<SEQUOIAFS_MODIFY_TIME<<\
               (SINT64)mtime<<SEQUOIAFS_ACCESS_TIME<<(SINT64)mtime));
   condition = BSON(SEQUOIAFS_LOBOID<<oid.toString());
   rc = doUpdateAttr(sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

   rc = size;
done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::release(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   sdbLob *lob = NULL;
   sdb *db = NULL;
   lobHandle *lh = NULL;
   std::map<UINT64, INT8>::iterator it;

   PD_LOG(PDDEBUG, "Called: release(), path:%s", path);

   lh = (lobHandle *)fi->fh;
   lob = (sdbLob *)lh->hLob;
   db = (sdb *)lh->hSdb;

   rc = lob->close();
   if(rc != SDB_OK)
   {
      PD_LOG(PDERROR, "Failed to close lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   pthread_mutex_lock(&mutex);
   it = _mapOpMode.find(fi->fh);
   if( it != _mapOpMode.end())
   {
      _mapOpMode.erase(it);
   }
   pthread_mutex_unlock(&mutex);
   PD_LOG(PDDEBUG, "succeed to release, path:%s", path);

done:
   delete lob;
   delete lh->hSysFileMetaCL;
   delete lh;
   releaseConnection(db);
   pthread_mutex_destroy(&lh->lock);
   return rc;

error:
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
   sdb *db = NULL;
   INT32 rc = SDB_OK;
   sdbLob *lob = NULL;
   lobHandle *lh = NULL;

   PD_LOG(PDDEBUG, "Called: flush(), path:%s", path);

   lh = (lobHandle *)fi->fh;
   db = (sdb *)lh->hSdb;
   lob = (sdbLob *)lh->hLob;

   lh = lh;
   db = db;
   lob = lob;
   goto error;
done:
   return rc;
error:
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
   sdb *db = NULL;
   BSONObj condition;
   sdbCollectionSpace cs;
   sdbCollection *sysDirMetaCL = new sdbCollection;
   sdbCollection *sysFileMetaCL = new sdbCollection;
   sdbCursor *cursorDir = new sdbCursor();
   sdbCursor *cursorFile = new sdbCursor();
   lobHandle *lh = new lobHandle;
   CHAR *pathStr = NULL;
   string lobName;
   INT64 pid;
   INT64 id;
   BSONObj options;

   pathStr = ossStrdup(path);
   INIT_LOBHANDLE(lh);
   PD_LOG(PDDEBUG, "Called: opendir(), path:%s", path);

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get connection, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }


   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), *sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), *sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(sysDirMetaCL, pathStr, &lobName);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   if(lobName == "/")
   {
      id = 1;
   }
   else
   {
      //get the ino of the dir
      id = getDirIno(sysDirMetaCL, lobName, pid);
   }
   //look for files in the dir based on the ino of the dir
   condition = BSON(SEQUOIAFS_PID<<id);
   rc = sysDirMetaCL->query(*cursorDir, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = sysFileMetaCL->query(*cursorFile, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   lh->hCursor[CURSOR_OF_META_DIR_CL] = cursorDir;
   lh->hCursor[CURSOR_OF_META_FILE_CL] = cursorFile;
   lh->hSdb = db;
   lh->hSysDirMetaCL= sysDirMetaCL;

   fi->fh = (intptr_t)(uint64_t)((void *)lh);

done:
   SDB_OSS_FREE(pathStr);
   delete sysFileMetaCL;
   return rc;

error:
   delete sysDirMetaCL;
   delete cursorDir;
   delete cursorFile;
   delete lh;
   releaseConnection(db);
   goto done;

}

INT32 sequoiaFS::readdir(const CHAR *path,
                         void *buf,
                         fuse_fill_dir_t filler,
                         off_t offset,
                         struct fuse_file_info *fi)
{
   sdbCursor *cursor = NULL;
   lobHandle *lh = NULL;
   INT32 count = 0;
   INT32 rc = SDB_OK;
   BSONObj record;
   BSONElement ele;
   INT32 i = 0;

   PD_LOG(PDDEBUG, "Called: readdir(), path:%s", path);

   rc = filler(buf, ".", NULL, 0);
   if(0 != rc)
   {
      rc = -ENOMEM;
      goto error;
   }

   rc = filler(buf, "..", NULL, 0);
   if(0 != rc)
   {
      rc = -ENOMEM;
      goto error;
   }

   lh = (lobHandle*)fi->fh;
   for(i = 0; i < NUM_OF_META_CL; i++)
   {
      cursor = lh->hCursor[i];
      while(SDB_OK == cursor->next(record))
      {
          count++;
          ele = record.getField("Name");
          rc = filler(buf, ele.String().c_str(), NULL, 0);
          if(SDB_OK != rc)
          {
           rc = -ENOMEM;
           goto error;
          }
      }
   }
done:
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::releasedir(const CHAR *path, struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   lobHandle *lh = NULL;
   sdb *db = NULL;

   PD_LOG(PDDEBUG, "Called: releasedir(), path:%s", path);

   lh = (lobHandle*)fi->fh;
   db = lh->hSdb;

   releaseConnection(db);
   delete lh->hSysDirMetaCL;
   if(lh->hCursor[CURSOR_OF_META_DIR_CL])
   {
      delete lh->hCursor[CURSOR_OF_META_DIR_CL];
   }
   if(lh->hCursor[CURSOR_OF_META_FILE_CL])
   {
      delete lh->hCursor[CURSOR_OF_META_FILE_CL];
   }
   delete lh;

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

INT32 sequoiaFS::do_access(struct stat *stbuf)
{
   INT32 rc = SDB_OK;

   if(S_ISREG(stbuf->st_mode) && !(stbuf->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
   {
      rc = -EACCES;
      PD_LOG(PDERROR, "Failed to access file, error=%d", rc);
      goto error;
   }

   if(S_ISDIR(stbuf->st_mode) && !(stbuf->st_mode & (S_IXUSR|S_IXGRP|S_IXOTH)))
   {
      rc = -EACCES;
      PD_LOG(PDERROR, "Failed to access dir, error=%d", rc);
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
          PD_LOG(PDERROR, "Failed to getattr, error=%d", rc);
          goto error;
      }

      rc = do_access(&stbuf);
   }

done:
   return rc;

error:
   goto done;

}

//when I call creat(filename, 0740), It seams it will call open to create
//->getattr():/testlob3 ->
INT32 sequoiaFS::create(const CHAR *path,
                        mode_t mode,
                        struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbLob *lob= new sdbLob();
   sdbCollection cl;
   sdbCollection *sysFileMetaCL = new sdbCollection();
   sdbCollection *sysDirMetaCL = new sdbCollection();
   BSONObj condition;
   BSONObj record;
   OID oid;
   BSONObj obj;
   string basePath;
   CHAR *pathStr = NULL;
   lobHandle *lh = new lobHandle;
   BOOLEAN exist = FALSE;
   struct fileMetaNode fileNode;
   UINT64 ctime = 0;
   UINT64 mtime = 0;
   INT64 pid = 0;
   uid_t uid = getuid();
   gid_t gid = getgid();
   BSONObj options;

   INIT_LOBHANDLE(lh);
   INIT_FILE_NODE(fileNode);

   PD_LOG(PDDEBUG, "Called: create(), path:%s, mode:%u, flags:%d", path, mode, fi->flags);

   pathStr = ossStrdup(path);
   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      goto error;
   }
   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), *sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), *sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, rc);
      rc = pid;
      goto error;
   }
   fileName = (CHAR *)basePath.c_str();
   PD_LOG(PDDEBUG, "Name:%s, Pid:%d", basePath.c_str(), pid);
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)pid);
   rc = doesFileExist(*sysFileMetaCL, fileName, condition, &exist, &oid, record);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   if(exist)
   {
      PD_LOG(PDERROR, "Failed to create file, name=%s, it does exist", fileName);
      rc = -EEXIST;
      goto error;
   }

   rc = db->getCollection(_collection.c_str(), cl);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _collection.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = cl.createLob(*lob);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to create lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = lob->getOid(oid);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get lob oid, error=%d", rc);
      rc = -EIO;
      goto error;
   }
   //close the lob to update the lob meta in sdb
   rc = lob->close();
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to close lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   //open lob according to the mode of open
   rc = cl.openLob(*lob, oid, SDB_LOB_WRITE);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to open lob, error=%d", rc);
      rc = -EIO;
      goto error;
   }


   rc = lob->getCreateTime(&ctime);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get lob create time, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   mtime = lob->getModificationTime();
   if(-1 == (SINT64)mtime)
   {
      PD_LOG(PDERROR, "Failed to get modification time, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   fileNode.name= fileName;
   fileNode.uid = uid;
   fileNode.gid = gid;
   fileNode.pid= pid;
   fileNode.nLink= 1;
   fileNode.lobOid = oid.toString();
   fileNode.ctime= ctime;
   fileNode.atime= mtime;
   fileNode.mtime = mtime;
   fileNode.mode= S_IFREG | mode;
   //here to remove the Name uniq index, cause there may be some same name
   //of files in different dirs
   rc = doSetFileNodeAttr(*sysFileMetaCL, fileNode);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set attr, error=%d", rc);
      goto error;
   }

   PD_LOG(PDDEBUG, "Called: create() successfully, Name:%s, Pid:%d", fileName, pid);
   lh->hLob = lob;
   lh->hSdb = db;
   lh->oid = oid;
   lh->hSysFileMetaCL= sysFileMetaCL;
   lh->hSysDirMetaCL= sysDirMetaCL;
   fi->fh = (intptr_t)(uint64_t)((void *)lh);
   pthread_mutex_init(&lh->lock, NULL);
   if(fi->flags & O_RDWR)
   {
      pthread_mutex_lock(&mutex);
      _mapOpMode.insert(std::pair<UINT64, INT8>(fi->fh, 2));
      pthread_mutex_unlock(&mutex);
   }

done:
   SDB_OSS_FREE(pathStr);
   return rc;

error:
   releaseConnection(db);
   delete sysDirMetaCL;
   delete sysFileMetaCL;
   delete lh;
   goto done;

}

INT32 sequoiaFS::ftruncate(const CHAR *path,
                           off_t offset,
                           struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   SINT64 lobSize = 0;
   sdbLob *lob = NULL;
   lobHandle *lh = NULL;
   sdbCollection *sysFileMetaCL = NULL;
   sdbCollection cl;
   BSONObj rule;
   BSONObj condition;
   CHAR *pathStr = NULL;
   UINT64 mtime = 0;
   UINT32 size = 0;
   CHAR *buf = NULL;
   bson::OID oid;
   struct timeval tval;
   BSONObj record;

   PD_LOG(PDDEBUG, "Called: ftruncate(), path:%s, offset:%d", path, offset);

   lh = (lobHandle *)fi->fh;
   db = (sdb *)lh->hSdb;
   lob = (sdbLob *)lh->hLob;
   pathStr = ossStrdup(path);
   sysFileMetaCL = (sdbCollection *)lh->hSysFileMetaCL;
   //first: get the size of lob, then compare the size with offset
   rc = lob->getSize(&lobSize);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get size, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = lob->getOid(oid);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get lob oid, error=%d", rc);
      rc = -EIO;
      goto error;
   }

   if(lobSize < offset)
   {
      rc = lob->seek(0, SDB_LOB_SEEK_END);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to seek lob, error=%d", rc);
          rc = -EIO;
          goto error;
      }
      size = offset - lobSize;

      buf = (CHAR *)SDB_OSS_MALLOC(size);
      ossMemset(buf, '\0', size);

      rc = lob->write(buf, (UINT32)size);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to write lob, error=%d", rc);
          rc = -EIO;
          goto error;
      }
      SDB_OSS_FREE(buf);
      lobSize = offset;
   }

   else if(lobSize > offset)
   {
      rc = db->getCollection(_collection.c_str(), cl);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
                 _collection.c_str(), rc);
          rc = -EIO;
          goto error;
      }

      rc = lob->close();
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to close lob, error=%d", rc);
          rc = -EIO;
          goto error;
      }

      rc = cl.truncateLob(oid, offset);
      if(SDB_OK != rc)
      {
          PD_LOG(PDERROR, "Failed to truncate lob, error=%d", rc);
          rc = -EIO;
          goto error;
      }
      lobSize = offset;
   }

   else
   {
      PD_LOG(PDDEBUG, "Lobsize is equal to offset, nothing to do");
      goto done;
   }

   gettimeofday(&tval, NULL);
   mtime = tval.tv_sec * 1000 + tval.tv_usec/1000;

   rule = BSON("$set"<<BSON(SEQUOIAFS_SIZE<<lobSize<<\
               SEQUOIAFS_MODIFY_TIME<<(SINT64)mtime));
   condition = BSON(SEQUOIAFS_LOBOID<<oid.toString());
   rc = doUpdateAttr(sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   SDB_OSS_FREE(pathStr);
   return rc;

error:
   goto done;
}

INT32 sequoiaFS::fgetattr(const CHAR *path, struct stat *buf,
                          struct fuse_file_info *fi)
{
   INT32 rc = SDB_OK;
   CHAR *fileName = NULL;
   sdb *db = NULL;
   BSONObj options;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   uid_t uid = getuid();
   gid_t gid = getgid();
   BSONObj record;
   BSONElement ele;
   lobHandle *lh = NULL;
   sdbCollection *sysFileMetaCL = NULL;
   sdbCollection sysDirMetaCL;
   string basePath;
   INT64 pid = 1;

   lh = (lobHandle *)fi->fh;
   sysFileMetaCL = (sdbCollection *)lh->hSysFileMetaCL;

   PD_LOG(PDDEBUG, "Called: fgetattr(), path:%s", path);

   ossMemset(buf, 0, sizeof(struct stat));

   buf->st_uid = uid;
   buf->st_gid = gid;
   pathStr = ossStrdup(path);

   if(ossStrcmp(path, "/") == 0)
   {
      buf->st_mode = S_IFDIR | 0755;
      buf->st_nlink = 2;
      goto done;
   }

   buf->st_mode = S_IFREG | 0644;
   buf->st_nlink = 1;

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Fail to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   PD_LOG(PDDEBUG, "The path:%s, pid:%d", pathStr, pid);
   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      PD_LOG(PDERROR, "Failed to get pid of directory, dir=%s, error=%d",
             pathStr, pid);
      rc = pid;
      goto error;
   }
   PD_LOG(PDDEBUG, "The base path:%s, pid:%d", basePath.c_str(), pid);
   fileName = (CHAR *)basePath.c_str();
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<(INT64)pid);
   rc = sysFileMetaCL->query(*cursor, condition);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to query file, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = cursor->current(record);
   if(SDB_DMS_EOC == rc )
   {
      PD_LOG(PDERROR, "Failed to get file, name=%s, it does not exist, error=%d",
             fileName, rc);
      rc = -ENOENT;
      goto error;
   }

   else if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Error happened during do cursor current, name=%s, error=%d",
             fileName, rc);
      rc = -EIO;
      goto error;
   }

   rc = getRecordField(record, (CHAR *)SEQUOIAFS_SIZE,
                       (void *)(&buf->st_size), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_CREATE_TIME,
                        (void *)(&buf->st_ctime), NumberLong);
   rc += getRecordField(record, (CHAR *)SEQUOIAFS_MODIFY_TIME,
                        (void *)(&buf->st_mtime), NumberLong);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get file attr, name=%s, error=%d", fileName, rc);
      rc = -EIO;
      goto error;
   }

   buf->st_ctime /= 1000;
   buf->st_mtime /= 1000;
   rc = SDB_OK;

done:
   SDB_OSS_FREE(pathStr);
   releaseConnection(db);
   return rc;
error:
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

INT32 sequoiaFS::utimens(const CHAR *path, const struct timespec ts[2])
{
   INT32 rc = SDB_OK;
   sdb *db = NULL;
   CHAR *fileName = NULL;
   sdbCollection sysFileMetaCL;
   sdbCollection sysDirMetaCL;
   sdbCollectionSpace cs;
   CHAR *pathStr = NULL;
   BSONObj condition;
   sdbCursor *cursor = new sdbCursor;
   BSONObj rule;
   INT64 pid = 0;
   BOOLEAN is_dir = TRUE;
   string basePath;
   INT64 atime = (INT64)(ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000);
   INT64 mtime = (INT64)(ts[1].tv_sec * 1000 + ts[1].tv_nsec/1000000);
   BSONObj options;

   PD_LOG(PDDEBUG, "Called: utimens(), path:%s, actime:%d, modtime:%d",
          path, (ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000),
          (ts[0].tv_sec * 1000 + ts[0].tv_nsec/1000000));

   pathStr = ossStrdup(path);
   if(ossStrcmp(path, "/") == 0)
   {
      goto done;
   }

   rc = getConnection(&db);
   if(SDB_OK != rc)
   {
      rc = -EIO;
      goto error;
   }

   options = BSON("PreferedInstance" << "M");
   rc = db->setSessionAttr(options);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to set the preferred instance for read request "
             "in the current session (PreferedInstance:M), error=%d", rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysFileMetaCLFullName.c_str(), sysFileMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysFileMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   rc = db->getCollection(_sysDirMetaCLFullName.c_str(), sysDirMetaCL);
   if(SDB_OK != rc)
   {
      PD_LOG(PDERROR, "Failed to get collection, cl=%s, error=%d",
             _sysDirMetaCLFullName.c_str(), rc);
      rc = -EIO;
      goto error;
   }

   pid = getDirPIno(&sysDirMetaCL, pathStr, &basePath);
   if(pid < 0)
   {
      rc = pid;
      goto error;
   }

   fileName = (CHAR *)basePath.c_str();
   rc = isDir(&sysFileMetaCL, &sysDirMetaCL, fileName, pid, &is_dir);
   if(SDB_OK != rc)
   {
      goto error;
   }
   condition = BSON(SEQUOIAFS_NAME<<fileName<<SEQUOIAFS_PID<<pid);
   //ts[0]:atime; ts[1]:mtime
   rule = BSON("$set"<<BSON(SEQUOIAFS_MODIFY_TIME<<mtime<<\
               SEQUOIAFS_CREATE_TIME<<mtime<<\
               SEQUOIAFS_ACCESS_TIME<<atime));
   rc = doUpdateAttr(is_dir?&sysDirMetaCL:&sysFileMetaCL, rule, condition);
   if(SDB_OK != rc)
   {
      goto error;
   }

done:
   SDB_OSS_FREE(pathStr);
   delete cursor;
   releaseConnection(db);
   return rc;

error:
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
   case TCGETS:
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



