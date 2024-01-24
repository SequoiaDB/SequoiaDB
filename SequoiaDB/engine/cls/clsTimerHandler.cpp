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

   Source File Name = clsTimerHandler.cpp

   Descriptive Name = Data Management Service Header

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          1/12/2012  Xu Jianhui  Initial Draft

   Last Changed =

*******************************************************************************/

#include "core.hpp"
#include "clsBase.hpp"
#include "clsTimerHandler.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   /*
      _clsReplTimerHandler implement
   */
   _clsReplTimerHandler::_clsReplTimerHandler ( _pmdAsycSessionMgr * pSessionMgr )
      :_pmdAsyncTimerHandler ( pSessionMgr )
   {
   }

   _clsReplTimerHandler::~_clsReplTimerHandler ()
   {
   }

   UINT64 _clsReplTimerHandler::_makeTimerID( UINT32 timerID )
   {
      return ossPack32To64( CLS_REPL, timerID ) ;
   }

   /*
      _clsShardTimerHandler implement
   */
   _clsShardTimerHandler::_clsShardTimerHandler ( _pmdAsycSessionMgr *pSessionMgr )
      :_pmdAsyncTimerHandler ( pSessionMgr )
   {
   }

   _clsShardTimerHandler::~_clsShardTimerHandler ()
   {
   }

   UINT64 _clsShardTimerHandler::_makeTimerID( UINT32 timerID )
   {
      return ossPack32To64( CLS_SHARD, timerID ) ;
   }

}

