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

   Source File Name = sequoiaFSRegister.hpp

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

#ifndef __SEQUOIAFSREG_HPP__
#define __SEQUOIAFSREG_HPP__
 
#include "netRouteAgent.hpp"
#include "sequoiaFSDao.hpp"

#include "sequoiaFSMsgHandler.hpp"
#include "sequoiaFSSessionMgr.hpp"

using namespace engine; 

#define FS_REGISTER_MCS_SECOND 30 

namespace sequoiafs
{
   class fsMetaCache;
   class fsRegister : public SDBObject
   {
      public:
         fsRegister(fsMetaCache* metaCache);
         ~fsRegister();
         void fini();
         INT32 start(sdbConnectionPool* ds, CHAR* mountCL, CHAR* mountPath);
         void hold();
         INT32 registerToMCS();
         BOOLEAN getStatus(){return _isRegistered;}
         void setRegister(BOOLEAN isRegistered);  
         
         INT32 sendRegReq(CHAR* mountPath, INT32 mountId, CHAR* mountIp);
         INT32 activeEDU();
         INT32 sendNotify(INT64 parentId, const CHAR* dirName);

      private:
         INT32 _queryMcsInfo(const CHAR *pCLFullName, string* hostName, string* port);
         INT32 _queryMountId(CHAR* mountcl, CHAR* mountpoint, INT64 *mountId);
         
      private:
         fsMetaCache*     _metaCache;
         netRouteAgent    _agent;
         BOOLEAN          _isRegistered; 
         BOOLEAN          _running;
         fsConnectionDao* _dbDao;
         CHAR*            _mountPath;
         CHAR*            _mountCL;
         CHAR*            _mountIP;
         INT64            _mountCLID;

         MsgRouteID       _routeID;
         
         _fsTimerHandler* _fsTimer;
         _fsSessionMgr    _fsSesMgr;
         NET_HANDLE       _pHandle;
   };
}

#endif

