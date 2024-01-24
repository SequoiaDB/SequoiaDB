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

   Source File Name = sequoiaFSDirMetaCache.cpp

   Descriptive Name = sequoiafs file meta cache manager.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          01/07/2021  zyj Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSDIRMETACACHE_HPP__
#define __SEQUOIAFSDIRMETACACHE_HPP__

#include "sequoiaFSMetaHashBucket.hpp"

#include "sequoiaFSLRUList.hpp"
#include "sequoiaFSCommon.hpp"
#include "sequoiaFSDao.hpp"

#include "ossRWMutex.hpp"

using namespace sdbclient;
using namespace engine;

namespace sequoiafs
{
   class fsMetaCache;
   class dirMetaCache : public SDBObject
   {
      public:
         dirMetaCache(fsMetaCache* mCache);
         ~dirMetaCache();
         INT32 init(sdbConnectionPool* ds, string dirCLName, INT32 capacity, BOOLEAN standalone);
         
         INT32 getDirInfo(INT64 parentId, const CHAR* dirName, dirMeta &meta);
         
         void releaseAllLocalCache(BOOLEAN isReleseLock);
         INT32 releaseLocalDirCache(INT64 parentId, const CHAR* dirName);
         sequoiaFSMetaHashBucket* getMetaHashBucket(){return &_dirHashBucket;} 
         LRUList* getLRUList(){return &_dirList;}

      private:
         INT32 _addDirInfo(dirMeta &meta);
         INT32 _delDirInfo(dirMetaNode* meta);
         INT32 _updateConnection();
         void _releaseLock(INT64 parentId, const CHAR* dirName);
         
      private:      
         sdbConnectionPool*         _ds;
         string _dirCLName;
         BOOLEAN _standalone;

         fsMetaCache*           _mCache;
         LRUList                _dirList;
         sequoiaFSMetaHashBucket _dirHashBucket;
         ossSpinXLatch          _listMutex;
         fsConnectionDao*       _dirDB;
         sdbCollection          _dMetaCL;
         
         ossSpinXLatch _dirDBMutex;
   };
}

#endif
