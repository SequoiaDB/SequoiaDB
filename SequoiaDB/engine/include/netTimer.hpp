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

   Source File Name = netTimer.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef NETTIMER_HPP_
#define NETTIMER_HPP_

#include "core.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include "oss.hpp"
#include "netDef.hpp"
#include "utilPooledAutoPtr.hpp"

namespace engine
{
   /*
      _netTimeoutHandler define
   */
   class _netTimeoutHandler : public SDBObject
   {
      public:
         virtual ~_netTimeoutHandler(){}
      public:
         virtual void handleTimeout( const UINT32 &millisec,
                                     const UINT32 &id ) = 0;
   } ;
   typedef _netTimeoutHandler netTimeoutHandler ;

   /*
      _netTimer define
   */
   class _netTimer ;
   typedef _netTimer netTimer ;
   typedef utilSharePtr<netTimer> NET_TH ;

   class _netTimer : public SDBObject
   {
      public:
         _netTimer( UINT32 millisec,
                    UINT32 id,
                    boost::asio::io_service &io,
                    _netTimeoutHandler *handler )
         :_timer( io ),
          _handler(handler),
          _id(id),
          _millisec(millisec),
          _actived( TRUE )
         {
         }

         ~_netTimer()
         {
         }

         static OSS_INLINE NET_TH createShared( UINT32 millisec,
                                                UINT32 id,
                                                boost::asio::io_service &io,
                                                _netTimeoutHandler *handler )
         {
            NET_TH th ;

            NET_TH tmpTH = NET_TH::allocRaw( __FILE__, __LINE__, ALLOC_POOL ) ;
            if ( NULL != tmpTH.get() &&
                 NULL != new( tmpTH.get() ) netTimer( millisec,
                                                      id,
                                                      io,
                                                      handler ) )
            {
               th.swap( tmpTH ) ;
            }

            return th ;
         }

      public:
         OSS_INLINE void timeoutCallback( const boost::system::error_code &error )
         {
            if ( !error )
            {
               _handler->handleTimeout( _millisec, _id ) ;
            }
            asyncWait() ;
         }

         OSS_INLINE UINT32 id()
         {
            return _id ;
         }

         OSS_INLINE UINT32 timeout()
         {
            return _millisec ;
         }

         OSS_INLINE void asyncWait()
         {
            if ( _actived )
            {
               _timer.expires_from_now( std::chrono::milliseconds(_millisec) ) ;
               _timer.async_wait( boost::bind( &_netTimer::timeoutCallback,
                                               _getShared(),
                                               boost::asio::placeholders::error ) ) ;
            }
         }

         OSS_INLINE void cancel()
         {
            _actived = FALSE ;
            _timer.cancel() ;
         }

      protected:
         OSS_INLINE NET_TH _getShared()
         {
            return NET_TH::makeRaw( this, ALLOC_POOL ) ;
         }

      private:
         boost::asio::steady_timer  _timer;
         _netTimeoutHandler         *_handler ;
         UINT32                     _id ;
         UINT32                     _millisec ;
         BOOLEAN                    _actived ;
   } ;

}

#endif // NETTIMER_HPP_

