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

   Source File Name = sequoiaFSMCS.hpp

   Descriptive Name = sequoiafs meta cache service.

   When/how to use: This program is used on sequoiafs.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date       Who Description
   ====== =========== === ==============================================
        01/07/2021  zyj  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef __SEQUOIAFSMCS_HPP__
#define __SEQUOIAFSMCS_HPP__

#include "sequoiaFSMCSOptionMgr.hpp"
//#include "sdbConnectionPoolcomm.hpp"
#include "sdbConnectionPool.hpp"

#include "netRouteAgent.hpp"

#include "sequoiaFSMCSMsgHandler.hpp"
#include "sequoiaFSMCSRegister.hpp"
#include "sequoiaFSMCSSessionMgr.hpp"
#include "sequoiaFSMCSOptionMgr.hpp"


using namespace engine;
using namespace bson; 
using namespace sdbclient;  

namespace sequoiafs
{
   struct mountNode
   {
      CHAR mountIp[OSS_MAX_IP_ADDR + 1];
      CHAR mountPath[OSS_MAX_PATHSIZE + 1];
      NET_HANDLE _handle;  
      mountNode* pre;
      mountNode* next;

      mountNode(CHAR *ip, CHAR* path, NET_HANDLE handle)
      {
         ossStrncpy(mountIp, ip, OSS_MAX_IP_ADDR);
         ossStrncpy(mountPath, path, OSS_MAX_PATHSIZE);
         _handle = handle;
         pre = NULL;
         next = NULL;
      }
   };
   
   class sequoiaFSMCS : public SDBObject
   {
      public:
         sequoiaFSMCS();
         ~sequoiaFSMCS();
         INT32 mcsThreadMain(INT32 argc, CHAR** argv);
         
         mountNode* getFsList(INT32 mountId);
         INT32 addMountNode(INT32 mountId, CHAR * mountIp, CHAR * mountPath, NET_HANDLE handle);
         void delMountNode(UINT32 handle);
         void cleanAllFS();
         
         netRouteAgent* getAgent(){return _agent;}
         mcsRegService* getRegService(){return &_regService;}

      private:
         INT32 _mcsInitDataSource(sequoiafsMcsOptionMgr *optionMgr);
         INT32 _activeEDU();
         INT32 _openPdLog();

      private:
         sdbConnectionPool   _ds;
         boost::thread*  _thRegisterDB;
         _netRouteAgent* _agent;
         
         ossPoolMap<UINT32, INT32> _handleMap;  
         ossPoolMap<INT32, mountNode*> _fsIdMap;
         ossSpinXLatch _mapMutex;

         CHAR _hostName[OSS_MAX_HOSTNAME+1];
         CHAR *_port;

         _mcsTimerHandler *_mcsTimer;
         _mcsSessionMgr    _mcsSesMgr;
         mcsMsghandler     _msgHandler;
         mcsRegService   _regService;  // reg in db and hold the lock
         sequoiafsMcsOptionMgr _optionMgr;
   };
}

#endif
