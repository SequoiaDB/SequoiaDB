/******************************************************************************

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
#include "ossMem.hpp"
#include "pmdEnv.hpp"
#include "pdTrace.hpp"
#include "msgConvertorImpl.hpp"
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
     _iops( 0 ),
     _peerVersion( SDB_PROTOCOL_VER_INVALID ),
     _inMsgConvertor( NULL ),
     _outMsgConvertor( NULL )
   {
      _id.value = MSG_INVALID_ROUTEID ;
   }

   _netEventHandlerBase::~_netEventHandlerBase()
   {
      if ( _inMsgConvertor )
      {
         SDB_OSS_DEL _inMsgConvertor ;
         _inMsgConvertor = NULL ;
      }
      if ( _outMsgConvertor )
      {
         SDB_OSS_DEL _outMsgConvertor ;
         _outMsgConvertor = NULL ;
      }
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

   INT32 _netEventHandlerBase::_enableMsgConvertor()
   {
      INT32 rc = SDB_OK ;

      if ( SDB_PROTOCOL_VER_1 != _peerVersion )
      {
         SDB_ASSERT( FALSE, "Version is not 1" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_inMsgConvertor )
      {
         _inMsgConvertor = SDB_OSS_NEW msgConvertorImpl ;
      }

      if ( !_outMsgConvertor )
      {
         _outMsgConvertor = SDB_OSS_NEW msgConvertorImpl ;
      }

      if ( !(_inMsgConvertor && _outMsgConvertor) )
      {
         rc = SDB_OOM ;
         goto error ;
      }

   done:
      return rc ;
   error:
      SAFE_OSS_DELETE( _inMsgConvertor ) ;
      SAFE_OSS_DELETE( _outMsgConvertor ) ;
      goto done ;
   }

   IMsgConvertor *_netEventHandlerBase::getInMsgConvertor()
   {
      return _inMsgConvertor ;
   }

   IMsgConvertor *_netEventHandlerBase::getOutMsgConvertor()
   {
      return _outMsgConvertor ;
   }
}
