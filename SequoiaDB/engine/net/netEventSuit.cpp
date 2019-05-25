/*******************************************************************************


   Copyright (C) 2011-2014 SequoiaDB Ltd.

   This program is free software: you can redistribute it and/or modify
   it under the term of the GNU Affero General Public License, version 3,
   as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warrenty of
   MARCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   GNU Affero General Public License for more details.

   You should have received a copy of the GNU Affero General Public License
   along with this program. If not, see <http://www.gnu.org/license/>.

   Source File Name = netEventSuit.cpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          12/07/2017  XJH Initial Draft

   Last Changed =

*******************************************************************************/

#include "netEventSuit.hpp"
#include "netFrame.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"

namespace engine
{

   #define NET_EV_INNER_TIMER_INTERVAL          ( 60 * OSS_ONE_SEC )
   #define NET_EV_RUN_TIMEOUT                   ( 600 * OSS_ONE_SEC )

   /*
      _netEventSuit implement
   */
   _netEventSuit::_netEventSuit( _netFrame *pFrame )
   :_timer( _ioservice )
   {
      _pFrame = pFrame ;
      _active = FALSE ;
      _noAttachTimeout = 0 ;
   }

   _netEventSuit::~_netEventSuit()
   {
      SDB_ASSERT( _active == FALSE, "Active must be FALSE" ) ;
   }

   io_service& _netEventSuit::getIOService()
   {
      return _ioservice ;
   }

   void _netEventSuit::addHandle( const NET_HANDLE &handle )
   {
      ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;
      _setHandle.insert( handle ) ;
   }

   void _netEventSuit::delHandle( const NET_HANDLE &handle )
   {
      ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;
      _setHandle.erase( handle ) ;
   }

   BOOLEAN _netEventSuit::exist( const NET_HANDLE &handle )
   {
      SET_HANDLE_IT it ;
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      it = _setHandle.find( handle ) ;
      if ( it != _setHandle.end() )
      {
         return TRUE ;
      }
      return FALSE ;
   }

   _netEventSuit::SET_HANDLE _netEventSuit::getHandles()
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      return _setHandle ;
   }

   UINT32 _netEventSuit::getHandleNum()
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      return (UINT32)_setHandle.size() ;
   }

   INT32 _netEventSuit::run()
   {
      _active = TRUE ;
      _attachEvent.signal() ;
      _pFrame->onRunSuitStart( shared_from_this() ) ;
      _asyncWait() ;
      _ioservice.run() ;
      _timer.cancel() ;
      _pFrame->onRunSuitStop( shared_from_this() ) ;
      _active = FALSE ;

      return SDB_OK ;
   }

   void _netEventSuit::stop()
   {
      if ( _active )
      {
         _ioservice.stop() ;
         _active = FALSE ;
      }
   }

   void _netEventSuit::_asyncWait()
   {
      _timer.expires_from_now( boost::chrono::milliseconds( NET_EV_INNER_TIMER_INTERVAL ) ) ;
      _timer.async_wait( boost::bind( &_netEventSuit::_timeoutCallback,
                                      this,
                                      boost::asio::placeholders::error ) ) ;
   }

   void _netEventSuit::_timeoutCallback( const boost::system::error_code &error )
   {
      if ( !error )
      {
         if ( 0 == getHandleNum() )
         {
            _noAttachTimeout += NET_EV_INNER_TIMER_INTERVAL ;

            if ( _noAttachTimeout >= NET_EV_RUN_TIMEOUT )
            {
               _pFrame->onSuitTimer( shared_from_this() ) ;
            }
         }
         else
         {
            _noAttachTimeout = 0 ;
         }
      }
      if ( _active )
      {
         _asyncWait() ;
      }
   }

   INT32 _netEventSuit::waitAttach( INT64 millsec )
   {
      return _attachEvent.wait( millsec ) ;
   }

}


