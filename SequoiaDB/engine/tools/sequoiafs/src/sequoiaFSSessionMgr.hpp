
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

   Source File Name = sequoiaFSMsgHandler.hpp

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

#ifndef __SEQUOIAFSSESSIONMGR_HPP__
#define __SEQUOIAFSSESSIONMGR_HPP__

#include "pmdAsyncSession.hpp"

using namespace engine;

#define MAX_CATCH_SIZE          (1000)

namespace sequoiafs
{
   class _fsSessionMgr : public _pmdAsycSessionMgr
   {
      public:
         _fsSessionMgr();
         virtual ~_fsSessionMgr();

         virtual INT32        handleSessionTimeout(UINT32 timerID,
                                                   UINT32 interval);
         
         virtual UINT64       makeSessionID(const NET_HANDLE &handle,
                                            const MsgHeader *header);

         virtual INT32        onErrorHanding(INT32 rc,
                                             const MsgHeader *pReq,
                                             const NET_HANDLE &handle,
                                             UINT64 sessionID,
                                             pmdAsyncSession *pSession);
         
      protected:
         virtual SDB_SESSION_TYPE   _prepareCreate(UINT64 sessionID,
                                                   INT32 startType,
                                                   INT32 opCode );

         virtual BOOLEAN      _canReuse(SDB_SESSION_TYPE sessionType);
         virtual UINT32       _maxCacheSize() const ;
         
         virtual pmdAsyncSession*  _createSession( SDB_SESSION_TYPE sessionType,
                                                   INT32 startType,
                                                   UINT64 sessionID,
                                                   void *data = NULL);

      protected:
         UINT32               _unShardSessionTimer ;
   } ;

}

#endif
