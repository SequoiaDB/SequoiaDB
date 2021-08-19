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

   Source File Name = clsVoteMachine.hpp

   Descriptive Name =

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/28/2012  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsVSSilence.hpp"
#include "pd.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "pmd.hpp"

namespace engine
{

   _clsVSSilence::_clsVSSilence( _clsGroupInfo *info,
                                 _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_SILENCE )
   {

   }

   _clsVSSilence::~_clsVSSilence()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_HDINPUT, "_clsVSSilence::handleInput" )
   INT32 _clsVSSilence::handleInput( const MsgHeader *header,
                                     INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_HDINPUT ) ;
      PD_LOG( PDDEBUG, "vote: silence status, ignore msg" ) ;
      next = id() ;
      PD_TRACE_EXIT ( SDB__CLSVSSL_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_HDTMOUT, "_clsVSSilence::handleTimeout" )
   void _clsVSSilence::handleTimeout( const UINT32 &millisec,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_HDTMOUT ) ;
      _timeout() += millisec ;

      const static UINT32 maxSliceTime = 30 * OSS_ONE_SEC ;
      /// silence time must be higher than brk time.
      if ( pmdGetOptionCB()->sharingBreakTime() + 1000 <= _timeout() ||
           maxSliceTime <= _timeout() )
      {
         next = CLS_ELECTION_STATUS_SEC ;
         goto done ;
      }
      next = id() ;
   done:
      PD_TRACE_EXIT ( SDB__CLSVSSL_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_ACTIVE, "_clsVSSilence::active" )
   void _clsVSSilence::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_ACTIVE ) ;
      _timeout() = 0 ;

      if ( _info()->groupSize() == 1 )
      {
         next = CLS_ELECTION_STATUS_SEC ;
      }
      else
      {
         next = id() ;
      }

      PD_TRACE_EXIT ( SDB__CLSVSSL_ACTIVE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSL_FORCEACTIVE, "_clsVSSilence::forceActive" )
   void _clsVSSilence::forceActive ( INT32 & next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSL_FORCEACTIVE ) ;
      _timeout() = 0 ;
      next = id() ;
      PD_TRACE_EXIT ( SDB__CLSVSSL_FORCEACTIVE ) ;
      return ;
   }

}
