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

#ifndef __SEQUOIAFSMETACACHE_HPP__
#define __SEQUOIAFSMETACACHE_HPP__

#include "sequoiaFSRegister.hpp"
#include "sequoiaFSDirMetaCache.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSMsgHandler.hpp"
#include "sequoiaFSDao.hpp"
#include<fuse.h>

using namespace sdbclient;

namespace sequoiafs
{
   class fsMetaCache : public SDBObject
   {
      public:
         fsMetaCache();
         ~fsMetaCache();
         INT32 init(sdbConnectionPool* ds, 
                    string dirCLName, 
                    string fileCLName, 
                    string collection, 
                    string mountpoint,
                    INT32 capacity,
                    BOOLEAN standalone);
         void fini();

         BOOLEAN isCacheMeta(){return _isCacheMeta;}
         void setCacheMeta(){_isCacheMeta = TRUE;}
         void cancleCacheMeta(){_isCacheMeta = FALSE;}

         INT32 getDirInfo(INT64 parentId, const CHAR* name, dirMeta &meta);
         INT32 getFileInfo(INT64 parentId, const CHAR* name, fileMeta &meta);
         INT32 getParentIdName(const CHAR* path, string *basePath, INT64 &parentId);
         
         INT32 isDir(INT64 parentId, const CHAR* path, BOOLEAN *isDir);
         INT32 isDirEmpty(INT64 id, BOOLEAN *isEmpty); 
         INT32 getAttr(const CHAR* path, metaNode *meta);
         INT32 readDir(INT64 parentId, void *buf, fuse_fill_dir_t filler);

         INT32 delDir(fsConnectionDao* connection, INT64 parentId, const CHAR* name);
         INT32 incDirLink(fsConnectionDao* connection, INT64 id);
         INT32 decDirLink(fsConnectionDao* connection, INT64 id);
         INT32 renameEntity(fsConnectionDao* connection,  
                            INT64 oldParentId, CHAR* oldName, 
                            INT64 newParentId, CHAR* newName);
         INT32 modMetaMode(INT64 parentId, 
                           CHAR* name, INT32 newMode);
         INT32 modMetaOwn(INT64 parentId, CHAR* name, 
                          UINT32 newUid, UINT32 newGid);
         INT32 modMetaUtime(INT64 parentId, 
                            CHAR* name, INT64 newMtime, INT64 newAtime);
         INT32 modMetaSize(CHAR* lobId, INT64 newSize, INT64 newMtime, INT64 newAtime);
         //INT32 updateConnection();

         fsRegister* getRegister(){return &_fsReg;}
         fsMsghandler* getMsgHandler(){return &_msgHandler;}
         dirMetaCache* getDirMetaCache(){return &_dirCache;}

      private:
         dirMetaCache     _dirCache;
         fsRegister       _fsReg;
         fsMsghandler     _msgHandler;
         BOOLEAN          _isCacheMeta;
         sdbConnectionPool*   _ds;
         fsConnectionDao* _dbDao;
         string           _fileCLName;
         string           _dirCLName;

         boost::thread*  _thFSReg;
   };
}

#endif
