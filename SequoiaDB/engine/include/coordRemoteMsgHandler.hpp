/*******************************************************************************


   Copyright (C) 2011-2021 SequoiaDB Ltd.

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

   Source File Name = coordRemoteMsgHandler.hpp

   Descriptive Name = Remote message handler on coordinator.

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains Runtime code for insert
   request.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          08/08/2020  YSD Initial Draft

   Last Changed =

*******************************************************************************/
#ifndef COORD_REMOTEMSGHANDLER_HPP__
#define COORD_REMOTEMSGHANDLER_HPP__

#include "pmdRemoteMsgEventHandler.hpp"

namespace engine
{
   class _coordDataSourceMgr ;

   /**
    * _coordDataSourceMsgHandler define
    * Data source message handler for coordinator, used to handle network events
    * with data source. It's binded to the net route agent for data source when
    * data source manager is initialized.
   */
   class _coordDataSourceMsgHandler : public _pmdRemoteMsgHandler
   {
   public:
      _coordDataSourceMsgHandler( _pmdRemoteSessionMgr *pRSManager,
                                  _coordDataSourceMgr *pDSMgr ) ;
      virtual ~_coordDataSourceMsgHandler() ;

      virtual INT32 handleMsg( const NET_HANDLE &handle,
                               const _MsgHeader *header,
                               const CHAR *msg ) ;

      virtual void  handleClose( const NET_HANDLE &handle, _MsgRouteID id ) ;

      virtual INT32 handleConnect( const NET_HANDLE &handle,
                                   _MsgRouteID id,
                                   BOOLEAN isPositive ) ;

   private:
      _coordDataSourceMgr     *_pDSMgr ;

   } ;
   typedef _coordDataSourceMsgHandler coordDataSourceMsgHandler ;

}

#endif /* COORD_REMOTEMSGHANDLER_HPP__ */

