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

   Source File Name = pmdDummySession.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date     Who     Description
   ====== ======== ======= ==============================================
          2020/8/8 Ting YU Initial Draft

   Last Changed =

*******************************************************************************/

#include "pmdDummySession.hpp"
#include "pmdEDU.hpp"

namespace engine
{
   MsgRouteID _pmdDummySession::identifyNID()
   {
      MsgRouteID route ;
      route.value = MSG_INVALID_ROUTEID ;
      return route ;
   }

   void _pmdDummySession::attachCB ( _pmdEDUCB *cb )
   {
      _pEDUCB = cb ;
      _eduID  = cb->getID() ;
      _pEDUCB->clearProcessInfo() ;
      _pEDUCB->attachSession( this ) ;
      _client.attachCB( _pEDUCB ) ;
   }

   void _pmdDummySession::detachCB ()
   {
      _client.detachCB() ;
      _pEDUCB->clearProcessInfo() ;
      _pEDUCB->detachSession() ;
      _pEDUCB = NULL ;
   }
}
