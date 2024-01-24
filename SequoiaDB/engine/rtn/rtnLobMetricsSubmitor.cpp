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

   Source File Name = rtnLobMetricsSubmitor.hpp

   Descriptive Name =

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/29/2022  TZB  Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnLobMetricsSubmitor.hpp"
#include "pmdEDU.hpp"

namespace engine
{

   _rtnLobMetricsSubmitor::_rtnLobMetricsSubmitor( pmdEDUCB *cb,
                                                   IMonSubmitEvent *pEvent )
   : _eduCB( cb ),
     _pEvent( pEvent ),
     _hasSubmit( FALSE ),
     _hasDiscard( FALSE )
   {
      if ( cb )
      {
         _monAppBegin = *cb->getMonAppCB() ;
      }
   }

   _rtnLobMetricsSubmitor::~_rtnLobMetricsSubmitor()
   {
      submit() ;
   }

   void _rtnLobMetricsSubmitor::submit()
   {
      if ( _eduCB && !_hasDiscard && !_hasSubmit )
      {
         if ( _pEvent )
         {
            monAppCB monDelta = *_eduCB->getMonAppCB() - _monAppBegin ;
            _pEvent->onSubmit( monDelta ) ;
         }
         _hasSubmit = TRUE ;
      }
   }

   void _rtnLobMetricsSubmitor::discard()
   {
      _hasDiscard = TRUE ;
   }

}

