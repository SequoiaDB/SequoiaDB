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

   Source File Name = clsTimerHandler.hpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#ifndef CLS_TIMER_HANDLER_HPP_
#define CLS_TIMER_HANDLER_HPP_

#include "pmdAsyncHandler.hpp"

namespace engine
{
   class _pmdAsycSessionMgr ;

   /*
      _clsReplTimerHandler define
   */
   class _clsReplTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _clsReplTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_clsReplTimerHandler () ;

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;

   } ;
   typedef _clsReplTimerHandler clsReplTimerHandler ;

   /*
      _clsShardTimerHandler define
   */
   class _clsShardTimerHandler : public _pmdAsyncTimerHandler
   {
      public:
         _clsShardTimerHandler ( _pmdAsycSessionMgr * pSessionMgr ) ;
         virtual ~_clsShardTimerHandler () ;

      protected:
         virtual UINT64  _makeTimerID( UINT32 timerID ) ;
   } ;
   typedef _clsShardTimerHandler clsShardTimerHandler ;

}

#endif //CLS_TIMER_HANDLER_HPP_

