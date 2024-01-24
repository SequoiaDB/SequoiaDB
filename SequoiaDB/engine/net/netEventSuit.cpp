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
      _stopped = FALSE ;
      _noAttachTimeout = 0 ;
   }

   _netEventSuit::~_netEventSuit()
   {
      SDB_ASSERT( _active == FALSE, "Active must be FALSE" ) ;
   }

   netEvSuitPtr _netEventSuit::createShared( _netFrame *frame )
   {
      netEvSuitPtr suitPtr ;

      netEvSuitPtr tmpPtr = netEvSuitPtr::allocRaw( __FILE__, __LINE__,
                                                    ALLOC_POOL ) ;
      if ( NULL != tmpPtr.get() &&
           NULL != new( tmpPtr.get() ) netEventSuit( frame ) )
      {
         suitPtr.swap( tmpPtr ) ;
      }

      return suitPtr ;
   }

   io_service& _netEventSuit::getIOService()
   {
      return _ioservice ;
   }

   INT32 _netEventSuit::addHandle( const NET_HANDLE &handle )
   {
      INT32 rc = SDB_OK ;

      try
      {
         ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;
         _setHandle.insert( handle ) ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Failed to insert handle set, occur exception: %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
      }

      return rc ;
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

   INT32 _netEventSuit::getHandles( SET_HANDLE &setHandle )
   {
      INT32 rc = SDB_OK ;

      try
      {
         ossScopedRWLock lock( &_rwMutex, SHARED ) ;
         setHandle = _setHandle ;
      }
      catch ( exception &e )
      {
         PD_LOG( PDERROR, "Failed to copy handle set, occur exception %s",
                 e.what() ) ;
         rc = ossException2RC( &e ) ;
      }

      return rc ;
   }

   NET_HANDLE _netEventSuit::getNextHandle( NET_HANDLE curHandle )
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      SET_HANDLE_IT iter = _setHandle.upper_bound( curHandle ) ;
      if ( iter == _setHandle.end() )
      {
         return NET_INVALID_HANDLE ;
      }
      return *iter ;
   }

   void _netEventSuit::removeAllEH()
   {
      ossScopedRWLock lock( &_rwMutex, EXCLUSIVE ) ;
      _setHandle.clear() ;
   }

   UINT32 _netEventSuit::getHandleNum()
   {
      ossScopedRWLock lock( &_rwMutex, SHARED ) ;
      return (UINT32)_setHandle.size() ;
   }

   INT32 _netEventSuit::run()
   {
      INT32 rc = SDB_OK ;

      try
      {
         _active = TRUE ;
         _pFrame->onRunSuitStart( _getShared() ) ;
         _asyncWait() ;
         _ioservice.run() ;
         _timer.cancel() ;
      }
      catch( std::exception &e )
      {
         if ( _active )
         {
            PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
            rc = SDB_SYS ;
         }
      }

      _pFrame->onRunSuitStop( _getShared() ) ;
      _active = FALSE ;
      return rc ;
   }

   void _netEventSuit::stop()
   {
      try
      {
         _active = FALSE ;
         _ioservice.stop() ;
      }
      catch ( std::exception &e )
      {
         PD_LOG( PDWARNING, "Stop occur exception: %s", e.what() ) ;
      }
   }

   void _netEventSuit::_asyncWait()
   {
      _timer.expires_from_now( std::chrono::milliseconds( NET_EV_INNER_TIMER_INTERVAL ) ) ;
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
               _pFrame->onSuitTimer( _getShared() ) ;
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

}


