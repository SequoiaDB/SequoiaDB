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

   Source File Name = netInnerTimer.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft
          09/01/2019  HGM Moved from netFrame.cpp

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "netInnerTimer.hpp"
#include "netFrame.hpp"

namespace engine
{

   #define NET_RESTART_TIMER_INTERVAL        ( 2000 )
   #define NET_DUMMY_TIMER_INTERVAL          ( 2147483647 )
   #define NET_UDP_CLEAR_EH_TIMER_INTERVAL   ( 60000 )

   /*
      _netRestartTimer implement
    */
   _netRestartTimer::_netRestartTimer( netFrame *frame )
   : _frame( frame ),
     _timerID( NET_INVALID_TIMER_ID ),
     _dummyTimerID( NET_INVALID_TIMER_ID )
   {
   }

   _netRestartTimer::~_netRestartTimer ()
   {
   }

   void _netRestartTimer::handleTimeout( const UINT32 &millisec,
                                         const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timerID == id )
      {
         rc = _frame->_listenTCP( _hostName.c_str(), _svcName.c_str() ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _frame->removeTimer( _timerID ) ;
            PD_LOG( PDEVENT, "Restart listening TCP on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netRestartTimer::setInfo( const CHAR *hostName,
                                   const CHAR *serviceName )
   {
      if ( _hostName.empty() )
      {
         _hostName = hostName ;
      }
      if ( _svcName.empty() )
      {
         _svcName = serviceName ;
      }
   }

   void _netRestartTimer::startTimer()
   {
      INT32 rc = _frame->addTimer( NET_RESTART_TIMER_INTERVAL, this,
                                   _timerID ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDSEVERE, "Restore listen error when open files up to "
                 "limit, stop network, rc: %d", rc ) ;
         _frame->stop() ;
      }
   }

   INT32 _netRestartTimer::startDummyTimer()
   {
      return _frame->addTimer( NET_DUMMY_TIMER_INTERVAL, this, _dummyTimerID ) ;
   }

   /*
      _netUDPRestartTimer implement
    */
   _netUDPRestartTimer::_netUDPRestartTimer( netFrame *frame )
   : _netRestartTimer( frame ),
     _handler( NULL ),
     _bufferSize( NET_UDP_DEFAULT_BUFFER_SIZE )
   {
   }

   _netUDPRestartTimer::~_netUDPRestartTimer()
   {
   }

   void _netUDPRestartTimer::handleTimeout( const UINT32 &millisec,
                                            const UINT32 &id )
   {
      INT32 rc = SDB_OK ;

      if ( _timerID == id )
      {
         rc = _frame->_listenUDP( _hostName.c_str(),
                                  _svcName.c_str(),
                                  _handler,
                                  _bufferSize ) ;
         if ( SDB_OK == rc || SDB_NET_ALREADY_LISTENED == rc )
         {
            _frame->removeTimer( _timerID ) ;
            PD_LOG( PDEVENT, "Restart listening UDP on %s:%s succeed",
                    _hostName.c_str(), _svcName.c_str() ) ;
         }
      }
   }

   void _netUDPRestartTimer::setInfo( const CHAR *hostName,
                                      const CHAR *serviceName,
                                      INetUDPMsgHandler *handler,
                                      UINT32 bufferSize )
   {
      _netRestartTimer::setInfo( hostName, serviceName ) ;
      _handler = handler ;
      _bufferSize = bufferSize ;
   }

}
