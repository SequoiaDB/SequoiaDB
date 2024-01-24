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

   Source File Name = netInnerTimer.hpp

   Descriptive Name =

   When/how to use: this program may be used on binary and text-motionatted
   versions of PD component. This file contains declare of PD functions.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          09/14/2012  YW  Initial Draft
          09/01/2019  HGM Moved from netFrame.hpp

   Last Changed =

*******************************************************************************/

#ifndef NET_INNER_TIMER_HPP_
#define NET_INNER_TIMER_HPP_

#include "core.hpp"
#include "oss.hpp"
#include "netDef.hpp"
#include "netTimer.hpp"
#include "netMsgHandler.hpp"
#include "ossMemPool.hpp"

namespace engine
{

   /*
      _netRestartTimer define
    */
   // _netRestartTimer restarts TCP listen
   class _netRestartTimer : public _netTimeoutHandler
   {
   public:
      _netRestartTimer( netFrame *frame ) ;
      virtual ~_netRestartTimer() ;

   public:
      // implement netTimeoutHandler virtual function
      virtual void handleTimeout( const UINT32 &millisec, const UINT32 &id ) ;

   public:
      void  setInfo( const CHAR *hostName, const CHAR *serviceName ) ;
      void  startTimer() ;
      INT32 startDummyTimer() ;
      void  stopDummyTimer() ;

   protected:
      netFrame *     _frame ;
      UINT32         _timerID ;
      // dummy timer is used to test io_service
      UINT32         _dummyTimerID ;
      ossPoolString  _hostName ;
      ossPoolString  _svcName ;
   } ;

   typedef class _netRestartTimer netRestartTimer ;

   /*
      _netUDPRestartTimer define
    */
   // _netRestartTimer restarts UDP listen
   class _netUDPRestartTimer : public _netRestartTimer
   {
   public:
      _netUDPRestartTimer( netFrame *frame ) ;
      virtual ~_netUDPRestartTimer() ;

   public:
      // implement netTimeoutHandler virtual function
      virtual void handleTimeout( const UINT32 &millisec, const UINT32 &id ) ;

   public:
      void setInfo( const CHAR *hostName,
                    const CHAR *serviceName,
                    UINT32 bufferSize ) ;

   protected:
      UINT32               _bufferSize ;
   } ;

   typedef class _netUDPRestartTimer netUDPRestartTimer ;

}

#endif // NET_INNER_TIMER_HPP_
