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

   Source File Name = clsVSSecondary.cpp

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

#include "clsVSSecondary.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{

   INT32 g_startShiftTime = 0 ;

   _clsVSSecondary::_clsVSSecondary( _clsGroupInfo *info,
                                     _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_SEC )
   {
      _hasPrint = FALSE ;
   }

   _clsVSSecondary::~_clsVSSecondary()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_HDINPUT, "_clsVSSecondary::handleInput" ) 
   INT32 _clsVSSecondary::handleInput( const MsgHeader *header,
                                       INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;
      if ( MSG_CLS_BALLOT_RES == header->opCode )
      {
         next = id() ;
         goto done ;
      }
      else if ( MSG_CLS_BALLOT == header->opCode )
      {
         g_startShiftTime = -1 ; // some node begin vote

         const _MsgClsElectionBallot *msg = ( const _MsgClsElectionBallot * ) header ;
         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round )
         {
            _promise( msg ) ;
            next = id() ;
         }
         else
         {
            if ( SDB_OK == _accept( msg ) )
            {
               next = CLS_ELECTION_STATUS_SILENCE ;
            }
            else
            {
               next = id() ;
            }
         }
      }
      else
      {
         /// error msg
         next = id() ;
      }
   done:
      PD_TRACE_EXIT ( SDB__CLSVSSD_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_HDTMOUT, "_clsVSSecondary::handleTimeout" )
   void _clsVSSecondary::handleTimeout( const UINT32 &millisec,
                                        INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_HDTMOUT ) ;
      _timeout() += millisec ;

      if ( g_startShiftTime > 0 && _info()->isAllNodeBeat() )
      {
         g_startShiftTime = -1 ; // recieve all node sharing-beat
      }

      if ( ( g_startShiftTime < 0 || g_startShiftTime <= (INT32)_timeout() ) &&
           CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( _hasPrint )
         {
            PD_LOG( PDEVENT, "%s Vote: begin to vote...", getScopeName() ) ;
         }
         g_startShiftTime = -1 ;
         next = CLS_ELECTION_STATUS_VOTE ;
      }
      else
      {
         if ( !_hasPrint && CLS_VOTE_CS_TIME <= _timeout() )
         {
            _hasPrint = TRUE ;
            PD_LOG( PDEVENT, "%s Vote: with waiting %u seconds or when all nodes beat "
                    "here, then begin to vote", getScopeName(),
                    ( g_startShiftTime - (INT32)_timeout() ) / 1000 ) ;
         }
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSSD_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_ACTIVE, "_clsVSSecondary::active" )
   void _clsVSSecondary::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_ACTIVE ) ;
      _timeout() = 0 ;
      _hasPrint = FALSE ;

      if ( _info()->groupSize() == 1 )
      {
         g_startShiftTime = -1 ;
         next = CLS_ELECTION_STATUS_VOTE ;
      }
      else
      {
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSSD_ACTIVE ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSSD_FORCEACTIVE, "_clsVSSecondary::forceActive" )
   void _clsVSSecondary::forceActive ( INT32 & next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSSD_FORCEACTIVE ) ;
      _timeout() = 0 ;
      _hasPrint = FALSE ;
      next = id() ;
      PD_TRACE_EXIT ( SDB__CLSVSSD_FORCEACTIVE ) ;
      return ;
   }

}
