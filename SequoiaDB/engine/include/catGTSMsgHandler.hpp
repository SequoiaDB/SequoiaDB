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

   Source File Name = catGTSMsgHandler.hpp

   Descriptive Name = GTS message handler

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          07/17/2018  David Li  Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef CAT_GTS_MSG_HANDLER_HPP_
#define CAT_GTS_MSG_HANDLER_HPP_

#include "oss.hpp"
#include "ossUtil.hpp"
#include "ossQueue.hpp"
#include "ossLatch.hpp"
#include "netDef.hpp"
#include "msg.h"
#include "rtnContextBuff.hpp"

namespace engine
{
   struct _catGTSMsg ;
   class sdbCatalogueCB ;
   class _catGTSManager ;
   class _pmdEDUCB ;

   class _catGTSMsgHandler: public SDBObject
   {
   public:
      _catGTSMsgHandler() ;
      ~_catGTSMsgHandler() ;

      INT32 init() ;
      INT32 fini() ;

      INT32 active() ;
      INT32 deactive() ;

      void checkLoad() ;
      void jobExit( BOOLEAN isController ) ;

      INT32 postMsg( const NET_HANDLE& handle, const MsgHeader* msg ) ;

      BOOLEAN popMsg( INT64 timeout, _catGTSMsg*& gtsMsg ) ;

      INT32 processMsg( const _catGTSMsg* gtsMsg ) ;

      INT32 primaryCheck() ;

      INT32 sendReply ( const NET_HANDLE& handle,
                        MsgOpReply* reply,
                        void* replyData = NULL,
                        UINT32 replyDataLen = 0 ) ;

   private:
      INT32 _ensureMsgJobController() ;
      INT32 _processSequenceAcquireMsg( MsgHeader* msg, rtnContextBuf& buf, _pmdEDUCB* eduCB ) ;
      INT32 _processSequenceCreateMsg( MsgHeader* msg, _pmdEDUCB* eduCB ) ;
      INT32 _processSequenceDropMsg( MsgHeader* msg, _pmdEDUCB* eduCB ) ;
      INT32 _processSequenceAlterMsg( MsgHeader* msg, _pmdEDUCB* eduCB ) ;

   private:
      _catGTSManager*         _gtsMgr ;
      ossQueue<_catGTSMsg*>   _msgQueue ;
      sdbCatalogueCB*         _catCB ;

      ossSpinXLatch           _jobLatch ;
      INT32                   _activeJobNum ;
      INT32                   _maxJobNum ;
      BOOLEAN                 _isControllerStarted ;
   } ;
   typedef _catGTSMsgHandler catGTSMsgHandler ;
}

#endif /* CAT_GTS_MSG_HANDLER_HPP_ */
