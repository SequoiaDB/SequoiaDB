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

   Source File Name = sequoiaFSDao.hpp

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

#ifndef __SEQUOIAFSDAO_HPP__
#define __SEQUOIAFSDAO_HPP__

#include "ossUtil.hpp"
#include "ossTypes.hpp"

#include "sdbConnectionPool.hpp"
#include "sequoiaFSCommon.hpp"

using namespace sdbclient;
using namespace bson;

namespace sequoiafs
{
   class fsConnectionDao : public SDBObject
   {
      public:
         fsConnectionDao(sdbConnectionPool* dataSource);
         ~fsConnectionDao();
         INT32 getFSConn(sdb** db);
         
         INT32 writeLob(const CHAR *buf,
                        const CHAR *clFullName,
                        const OID &lobId, 
                        INT64 offset,
                        INT32 len,
                        INT64 *lobSizeNew);
         INT32 writeNewLob(const CHAR *buf,
                           const CHAR *clFullName,
                           OID &lobId, 
                           INT64 offset,
                           INT32 len,
                           INT64 &lobSizeNew);               
         INT32 readLob(const CHAR *clFullName, 
                       const OID &lobId, 
                       INT64 offset, 
                       INT32 size, 
                       CHAR  *buf, 
                       INT32 *len);
         
         INT32 truncateLob(const CHAR *clFullName,
                           const OID &lobId, 
                           INT64 offset);
         INT32 removeLob(const CHAR *clFullName,
                         const OID &lobId);
         INT32 getLobSize(const CHAR *clFullName, 
                          const OID &lobId, 
                          SINT64 *len);

         INT32 transBegin(BOOLEAN readonly = FALSE);
         INT32 transCommit();
         INT32 transRollback();

         BOOLEAN isValid();

         INT32 releaseSLock(sdbCollection &cl, BSONObj &condition);
         
         INT32 getCL(const CHAR *pCollectionFullName,
                     sdbCollection &collection);
         INT32 getCS(const CHAR *pCollectionSpacelName,
                     sdbCollectionSpace &cs);
         
         INT32 createCollectionSpace(const CHAR *pCollectionSpaceName,
                                     const bson::BSONObj &options,
                                     sdbCollectionSpace &cs);
         
         INT32 createCollection(sdbCollectionSpace &cs,
                                const CHAR *pCollectionName,
                                sdbCollection &collection);
         
         INT32 queryForUpdate(const CHAR *pCLFullName, 
                              sdbCursor &cursor, 
                              const bson::BSONObj &condition);

         INT32 insertMountId(const CHAR *pCLFullName,
                             CHAR* mountpoint, 
                             INT32 mountId);

         INT32 queryMeta(sdbCollection &cl,
                         BSONObj &condition, 
                         BSONObj &record,
                         BOOLEAN lockS = FALSE);

         INT32 findOne(const CHAR* clName, 
                      INT64 parentid, 
                      BSONObj &record);

         INT32 insertMeta(const CHAR *pCLFullName,
                          BSONObj &obj);

         INT32 updateMeta(const CHAR *pCLFullName,
                          BSONObj &condition, 
                          BSONObj &rule,
                          BSONObj &hint);

         INT32 delMeta(const CHAR *pCLFullName,
                       const CHAR* name, 
                       INT64 parentid,
                       BSONObj &hint);

      private:
         sdbCollection* getDirCL(const CHAR *pCLFullName);
         sdbCollection* getFileCL(const CHAR *pCLFullName);
         
      private:
         sdbConnectionPool* _ds;
         sdb* _db;
         sdbCollection* _dirCL;
         sdbCollection* _fileCL;
   };
}
         
#endif
