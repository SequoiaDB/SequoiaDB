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

   Source File Name = sequoiaFSMsgHandler.cpp

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

#include "sequoiaFSMCSSessionMgr.hpp"
#include "sequoiaFSMCSSession.hpp"

#include "sequoiaFSMCS.hpp"

using namespace engine;

namespace sequoiafs
{
   _mcsSessionMgr::_mcsSessionMgr()
   {
   }
   
   _mcsSessionMgr::~_mcsSessionMgr()
   {
   }
         
   UINT64 _mcsSessionMgr::makeSessionID(const NET_HANDLE &handle,
                                        const MsgHeader *header)
   {
      UINT64 sessionID = ossPack32To64(header->routeID.columns.nodeID,
                                       header->TID);
      if(header->routeID.columns.nodeID < DATA_NODE_ID_BEGIN ||
         header->routeID.columns.groupID < DATA_GROUP_ID_BEGIN)
      {
         sessionID = ossPack32To64( PMD_BASE_HANDLE_ID + handle, header->TID ) ;
      }

      return sessionID;
   }

   SDB_SESSION_TYPE _mcsSessionMgr::_prepareCreate(UINT64 sessionID,
                                                   INT32 startType,
                                                   INT32 opCode)
   {
      SDB_SESSION_TYPE sessionType = SDB_SESSION_SHARD;
      
      return sessionType ;
   }

   BOOLEAN _mcsSessionMgr::_canReuse(SDB_SESSION_TYPE sessionType)
   {
      if (SDB_SESSION_SHARD == sessionType)
      {
         return TRUE ;
      }
      return FALSE ;
   }

   UINT32 _mcsSessionMgr::_maxCacheSize() const
   {
      return MAX_CATCH_SIZE;
   }

   pmdAsyncSession* _mcsSessionMgr::_createSession(
                                    SDB_SESSION_TYPE sessionType,
                                    INT32 startType,
                                    UINT64 sessionID,
                                    void *data )
   {
      pmdAsyncSession *pSession = SDB_OSS_NEW _mcsMsgSession(sessionID);
      return pSession ;
   }

   INT32 _mcsSessionMgr::handleSessionTimeout(UINT32 timerID,
                                              UINT32 interval)
   {
      INT32 rc = SDB_OK;
      
      rc = _pmdAsycSessionMgr::handleSessionTimeout(timerID, interval);
      
      return rc;
   }

   INT32 _mcsSessionMgr::onErrorHanding(INT32 rc,
                                        const MsgHeader *pReq,
                                        const NET_HANDLE &handle,
                                        UINT64 sessionID,
                                        pmdAsyncSession *pSession )
   {
      INT32 ret = SDB_OK ;

      UINT32 nodeID = 0 ;
      UINT32 tid = 0 ;
      ossUnpack32From64( sessionID, nodeID, tid ) ;

      ret = rc ;
      
      return ret ;
   }
  
}

