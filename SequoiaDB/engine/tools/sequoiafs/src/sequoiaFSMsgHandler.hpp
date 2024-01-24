
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

#ifndef __SEQUOIAFSHANDLER_HPP__
#define __SEQUOIAFSHANDLER_HPP__

#include "pmdAsyncHandler.hpp"
#include "netMsgHandler.hpp"
#include "sequoiaFSCommon.hpp"

using namespace engine;

#define FSTimer 1

namespace sequoiafs
{
   class fsMetaCache;
   class fsMsghandler : public _netMsgHandler
   {
      public:
         fsMsghandler(fsMetaCache* metaCache);
         virtual ~fsMsghandler();
         virtual INT32 handleMsg( const NET_HANDLE &handle,
                       const MsgHeader *header,
                       const CHAR *msg );
         virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id );
         INT32 handleRegRsp(_mcsRegRsp* response);
         INT32 handleNotify(_mcsNotifyReq* request);

      private:
         fsMetaCache* _metaCache;
   };

   class _fsTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _fsTimerHandler(_pmdAsycSessionMgr * pSessionMgr);
         virtual ~_fsTimerHandler();

      protected:
         virtual UINT64  _makeTimerID(UINT32 timerID);
   };
}

#endif
