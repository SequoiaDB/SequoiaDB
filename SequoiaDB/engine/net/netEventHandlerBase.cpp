/******************************************************************************

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

   Source File Name = netEventHandlerBase.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/01/2019  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "netEventHandlerBase.hpp"
#include "netFrame.hpp"
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "msgDef.h"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "netTrace.hpp"
#include "msgMessageFormat.hpp"
#include "netEventSuit.hpp"
#include <boost/bind.hpp>
#if defined (_WINDOWS)
#include <mstcpip.h>
#endif

using namespace boost::asio::ip ;
using namespace std ;

namespace engine
{

   #define NET_STAT_CLEAR_INTERVAL ( 120 * OSS_ONE_SEC )

   /*
      _netEventHandlerBase implement
    */
   _netEventHandlerBase::_netEventHandlerBase( const NET_HANDLE &handle )
   : _handle( handle ),
     _isConnected( FALSE ),
     _isNew( TRUE ),
     _msgid( 0 ),
     _lastSendTick( pmdGetDBTick() ),
     _lastRecvTick( _lastSendTick ),
     _lastBeatTick( _lastSendTick ),
     _lastStatTick( _lastSendTick ),
     _totalIOTimes( 0 ),
     _iops( 0 )
   {
      _id.value = MSG_INVALID_ROUTEID ;
   }

   _netEventHandlerBase::~_netEventHandlerBase()
   {
   }

   void _netEventHandlerBase::syncLastBeatTick()
   {
      _lastBeatTick = pmdGetDBTick() ;
   }

   void _netEventHandlerBase::makeStat( UINT64 curTick )
   {
      UINT64 spanTime = pmdDBTickSpan2Time( curTick - _lastStatTick ) ;
      if ( spanTime > 0 )
      {
         _iops = _totalIOTimes / spanTime ;

         if ( spanTime >= NET_STAT_CLEAR_INTERVAL )
         {
            _lastStatTick = curTick ;
            _totalIOTimes = 0 ;
         }
      }
   }

}
