
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

#ifndef __SEQUOIAFSSESSION_HPP__
#define __SEQUOIAFSSESSION_HPP__

#include "pmdAsyncSession.hpp"

using namespace engine;

namespace sequoiafs
{
   class _fsMsgSession : public _pmdAsyncSession
   {
      DECLARE_OBJ_MSG_MAP()
      public:
         _fsMsgSession(UINT64 sessionID);
         virtual ~_fsMsgSession();

         virtual SDB_SESSION_TYPE sessionType() const;
         virtual const CHAR*      className() const {return "fs-msg";}

         virtual EDU_TYPES eduType () const ;
         virtual BOOLEAN canAttachMeta() const {return FALSE;}

         // called by net io thread
         virtual BOOLEAN timeout (UINT32 interval);
         virtual void    onRecieve (const NET_HANDLE netHandle,
                                    MsgHeader * msg);
         // called by self thread
         virtual void    onTimer (UINT64 timerID, UINT32 interval);
         virtual void   _onAttach ();
         virtual void   _onDetach ();

      private:
         _dpsLogWrapper                *_logger;

         MsgRouteID                    _syncSr;
         MsgRouteID                    _lastSyncNode;
         BOOLEAN                       _quit;

         BOOLEAN                       _isFirstToSync;
         UINT32                        _timeout;
         UINT64                        _requestID;
         UINT32                        _syncFailedNum;

         UINT32                        _fullSyncIgnoreTimes ;
   } ;

}

#endif
