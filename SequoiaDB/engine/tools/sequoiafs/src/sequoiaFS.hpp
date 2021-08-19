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

   Source File Name = sequoiaFS.hpp

   Descriptive Name = sequoiafs fuse file operation api.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
       03/05/2018  YWX  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFS_HPP__
#define __SEQUOIAFS_HPP__

#define FUSE_USE_VERSION 26

#define _XOPEN_SOURCE 500

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<libgen.h>
#include "core.h"
#include<fuse.h>
#include<time.h>
//#include<linux/stat.h>
#include "fuse_lowlevel.h"
#include "ossUtil.h"
#include "ossProc.hpp"
#include "ossUtil.hpp"
#include "utilStr.hpp"
#include "utilParam.hpp"
#include "ossIO.hpp"
#include "pd.hpp"
#include "pmdDef.hpp"
#include "sequoiaFSOptionMgr.hpp"
#include "sdbDataSourceComm.hpp"
#include "sdbDataSource.hpp"

#include<arpa/inet.h>
#include<sys/socket.h>
#include<netdb.h>
#include<ifaddrs.h>
#include<net/if.h>

using std::string;
using namespace sdbclient;
using namespace bson;

#define MAX_NODE_NAME_LEN 20
#define NUM_OF_NODE_NAME {sizeof(lobLbfsNodeName)/sizeof(lobLbfsNodeName[0])}

namespace sequoiafs
{
   struct dirMetaNode
   {
      string name;
      UINT32 mode; //umode_t(u_shourt)
      UINT32 uid;
      UINT32 gid;
      UINT32 nLink;
      INT64 pid;
      INT64 id;
      INT64 size;  //off_t long long
      INT64 ctime;//(struct timespec lobCTime)
      INT64 mtime;
      INT64 atime;
      string symLink;
   };

   struct fileMetaNode
   {
      string name;
      UINT32 mode; //umode_t(u_shourt)
      UINT32 uid;
      UINT32 gid;
      UINT32 nLink;
      string lobOid;
      INT64 pid;
      INT64 size;  //off_t long long
      INT64 ctime;//(struct timespec lobCTime)
      INT64 mtime;
      INT64 atime;
      string symLink;
   };

   class sequoiaFS: public SDBObject
   {
   public:
      INT32 init(INT32 argc, CHAR **argv, vector<string> *options4fuse);
      void destroy(void *userdata);
      INT32 getattr(const CHAR *path, struct stat *statbuf);
      INT32 readlink(const CHAR *path, CHAR * link, size_t size);
      INT32 getdir(const CHAR *path, fuse_dirh_t dirh,
                   fuse_dirfil_t dirfill);
      INT32 mknod(const CHAR *path, mode_t mode, dev_t dev);
      INT32 mkdir(const CHAR *path, mode_t mode);
      INT32 unlink(const CHAR *path);
      INT32 rmdir(const CHAR *path);
      INT32 symlink(const CHAR *path, const CHAR *link);
      INT32 rename(const CHAR *path, const CHAR *newpath);
      INT32 link(const CHAR *path, const CHAR *newpath);
      INT32 chmod(const CHAR *path, mode_t mode);
      INT32 chown(const CHAR *path, uid_t uid, gid_t gid);
      INT32 truncate(const CHAR *path, off_t newsize);
      INT32 utime(const CHAR *path, struct utimbuf *);
      INT32 open(const CHAR *path, struct fuse_file_info *fi);
      INT32 read(const CHAR *path, CHAR *buf, size_t size, off_t offset ,
                 struct fuse_file_info *fi);
      INT32 write(const CHAR *path, const CHAR *buf, size_t size,
                  off_t offset, struct fuse_file_info *fi);
      INT32 statfs(const CHAR *path, struct statvfs *statfs);
      INT32 flush(const CHAR *path, struct fuse_file_info *fi);
      INT32 release(const CHAR *path, struct fuse_file_info *fi);
      INT32 fsync(const CHAR *path, INT32 datasync,
                  struct fuse_file_info *fi);
      INT32 setxattr(const CHAR *path, const CHAR *name,
                     const CHAR *value, size_t size, INT32 flags);
      INT32 getxattr(const CHAR *path, const CHAR *name,
                     CHAR *value, size_t size);
      INT32 listxattr(const CHAR *path, CHAR *list, size_t size);
      INT32 removexattr(const CHAR *path, const CHAR *name);
      INT32 opendir(const CHAR *path, struct fuse_file_info *fi);
      INT32 readdir(const CHAR *path, void *buf,
                    fuse_fill_dir_t filler, off_t offset,
                    struct fuse_file_info *fi);
      INT32 releasedir(const CHAR *path, struct fuse_file_info *fi);
      INT32 fsyncdir(const CHAR *path, INT32 datasync,
                     struct fuse_file_info *fi);
      INT32 access(const CHAR *path, INT32 mask);
      INT32 create(const CHAR *path, mode_t mode,
                   struct fuse_file_info *fi);
      INT32 ftruncate(const CHAR *path, off_t offset,
                      struct fuse_file_info *fi);
      INT32 fgetattr(const CHAR *path, struct stat *buf,
                     struct fuse_file_info *fi);
      INT32 lock(const CHAR *path, struct fuse_file_info *fi,
                 INT32 cmd, struct flock *lock);
      INT32 utimens(const CHAR *path, const struct timespec tv[2]);
      INT32 bmap(const CHAR *path, size_t blocksize, uint64_t *idx);
      INT32 ioctl(const CHAR *path, INT32 cmd, void *arg,
                  struct fuse_file_info *fi, UINT32 flags, void *data);
      INT32 poll(const CHAR *path, struct fuse_file_info *fi,
                 struct fuse_pollhandle *ph, unsigned *reventsp);
      INT32 write_buf(const CHAR *path, struct fuse_bufvec *buf, off_t off,
                      struct fuse_file_info *fi);
      INT32 read_buf(const CHAR *path, struct fuse_bufvec **bufp,
                     size_t size, off_t off, struct fuse_file_info *fi);
      INT32 flock(const CHAR *path, struct fuse_file_info *fi, INT32 op);
      INT32 fallocate(const CHAR *path, INT32 mode,
                      off_t offset, off_t value, struct fuse_file_info *fi);

      sequoiafsOptionMgr * getOptionMgr();
      sequoiaFS()
      {
        _collection = "";
        _sysFileMetaCLFullName = "";
        _sysDirMetaCLFullName = "";
        _mountpoint = "";
        _replsize = SDB_SEQUOIAFS_REPLSIZE_DEFAULT_VALUE;
      }

      ~sequoiaFS(){}

      const CHAR *getHosts()const{return _optionMgr.getHosts();}
      INT32 getRecordField(BSONObj &record, CHAR *fieldName,
                           void *value, BSONType type);
      INT32 getRecord();
      INT32 doUpdateAttr(sdbCollection *cl, const BSONObj &rule,
                         const BSONObj &condition = _sdbStaticObject,
                         const BSONObj &hint = _sdbStaticObject);
      INT32 doesFileExist(sdbCollection &cl, const CHAR *lobName,
                          BSONObj &condition, BOOLEAN *exist,
                          OID *oid, BSONObj &record);
      void setDataSourceConf(const CHAR * userName,
                             const CHAR *passwd, const INT32 connNum);
      int initDataSource(const CHAR * userName,
                         const CHAR *passwd,
                         const INT32 connNum);
      INT32 disableDataSource();
      INT32 closeDataSource();
      INT32 getConnection(sdb **connection);
      void releaseConnection(sdb *connection)
      {
         ds.releaseConnection(connection);
      }
      INT64 getDirIno(sdbCollection *sysMetaCl,
                      string dirname, INT64 pino);
      INT64 getDirPIno(sdbCollection *sysMetaCl,
                       CHAR *pathStr, string *basePath, bool flag = FALSE);
      void getCoordHost();
      INT32 writeMapHistory(const CHAR *hosts);
      INT32 initMetaCSCL(sdb *db, const string csName,
                         const string clName,
                         const CHAR *idxName,
                         BOOLEAN createIndex,
                         const bson::BSONObj &indexDef,
                         BOOLEAN isUnique = FALSE,
                         BOOLEAN isEnforced = FALSE);
      INT32 initDataCSCL(sdb *db, const string collection);
      void setReplSize(INT32 replsize)
      {
        _replsize = replsize;
      };
      INT32 replSize() const
      {
        return _replsize;
      }
   private:
      INT32 doSetDirNodeAttr(sdbCollection &cl,
                             struct dirMetaNode &dirNode);
      INT32 doSetFileNodeAttr(sdbCollection &cl,
                             struct fileMetaNode &fileNode);
      INT32 getAndUpdateID(sdbCollection *cl, INT64 *sequenceId);
      INT32 initMetaID(sdb *db);
      INT32 initRootPath();
      INT32 do_access(struct stat *stbuf);
      INT32 isDir(sdbCollection *sysFileMetaCL,
                  sdbCollection *sysDirMetaCL,
                  CHAR *name, INT64 pid, BOOLEAN *is_dir);
   private:
      sdbDataSourceConf conf;
      sdbDataSource ds;
      string _csName;
      string _clName;
      string _sysDirMetaCSName;
      string _sysDirMetaCLName;
      string _sysFileMetaCSName;
      string _sysFileMetaCLName;
      string _sysFileMetaCLFullName;
      string _sysDirMetaCLFullName;
      vector<string> _coordHostPort;
      sequoiafsOptionMgr _optionMgr;
      std::map<UINT64, INT8> _mapOpMode;
      INT32 _replsize;

   public:
      string _collection;
      string _mountpoint;
   };
}
#endif

