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

   Source File Name = netEventSuit.hpp

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

#ifndef NET_EVENT_SUIT_HPP__
#define NET_EVENT_SUIT_HPP__

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "ossLatch.hpp"
#include "netTimer.hpp"
#include "ossAtomic.hpp"
#include "ossRWMutex.hpp"
#include "ossEvent.hpp"
#include "sdbInterface.hpp"
#include "utilPooledObject.hpp"
#include <boost/asio/steady_timer.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>

#include <set>

using namespace boost::asio ;
using namespace std ;
namespace engine
{

   class _netFrame ;

   /*
      _netEventSuit define
   */
   class _netEventSuit : public _IIOService, public utilPooledObject
   {
      public:
         typedef set<NET_HANDLE>                SET_HANDLE ;
         typedef SET_HANDLE::iterator           SET_HANDLE_IT ;

      public:
         _netEventSuit( _netFrame *pFrame ) ;
         virtual ~_netEventSuit() ;

         static netEvSuitPtr createShared( _netFrame *frame ) ;

         io_service&    getIOService() ;
         _netFrame*     getFrame() { return _pFrame ; }

         void           addHandle( const NET_HANDLE &handle ) ;
         void           delHandle( const NET_HANDLE &handle ) ;
         BOOLEAN        exist( const NET_HANDLE &handle ) ;
         SET_HANDLE     getHandles() ; 
         void           removeAllEH() ;
         UINT32         getHandleNum() ;

         INT32          waitAttach( INT64 millsec ) ;

      public:
         virtual INT32     run() ;
         virtual void      stop() ;
         virtual void      resetMon() {}

      protected:
         void           _asyncWait() ;
         void           _timeoutCallback( const boost::system::error_code &error ) ;

         OSS_INLINE netEvSuitPtr _getShared()
         {
            return netEvSuitPtr::makeRaw( this, ALLOC_POOL ) ;
         }

      private:

         io_service                       _ioservice ;
         _netFrame                        *_pFrame ;

         SET_HANDLE                       _setHandle ;
         ossRWMutex                       _rwMutex ;
         ossEvent                         _attachEvent ;

         BOOLEAN                          _active ;
         UINT32                           _noAttachTimeout ;

         /// timer
         boost::asio::steady_timer        _timer;

   } ;
   typedef _netEventSuit netEventSuit ;

}

#endif // NET_EVENT_SUIT_HPP__

