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

   Source File Name = clsVSAnnounce.cpp

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

#include "clsVSAnnounce.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   _clsVSAnnounce::_clsVSAnnounce( _clsGroupInfo *info, _netRouteAgent *agent )
   :_clsVoteStatus( info, agent, CLS_ELECTION_STATUS_ANNOUNCE )
   {

   }

   _clsVSAnnounce::~_clsVSAnnounce()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_HDINPUT, "_clsVSAnnounce::handleInput" )
   INT32 _clsVSAnnounce::handleInput( const MsgHeader *header,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "header should not be NULL" ) ;

      /// do not accept any ballot
      if ( MSG_CLS_BALLOT == header->opCode )
      {
         next = id() ;
      }
      else if ( MSG_CLS_BALLOT_RES == header->opCode )
      {
         const _MsgClsElectionRes *msg = ( const _MsgClsElectionRes * ) header ;
         if ( CLS_ELECTION_ROUND_STAGE_ONE == msg->round )
         {
            next = id() ;
         }
         else
         {
            if ( SDB_OK == msg->header.res )
            {
               const _clsSharingStatus &status = _info()->info.find( msg->identity.value )->second ;

               // If this accepted msg is from critical node, record it
               if ( status.isInCriticalMode() )
               {
                  ++_criticalAccepted() ;
               }

               // If this accepted msg is from maintenance node, ignore it
               if ( ! status.isInMaintenanceMode() )
               {
                  ++_accepted() ;
               }

               // Node in normal mode, not critical mode
               if ( _info()->groupSize() <= ( _accepted() + 1 ) )
               {
                  next =  CLS_ELECTION_STATUS_PRIMARY;
                  PD_LOG( PDEVENT, "%s Vote: change to primary by all accept", getScopeName() ) ;
               }
               // Node in critical mode which is only effective in group election( not in location election )
               else if ( ! isLocation() && CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode )
               {
                  if ( _info()->criticalSize() <= ( _criticalAccepted() + 1 ) )
                  {
                     // Node is in enforced mode, need to check if all critical nodes agree
                     if ( _info()->enforcedGrpMode )
                     {
                        next = CLS_ELECTION_STATUS_PRIMARY ;
                        PD_LOG( PDEVENT, "%s Vote: change to primary by all critical nodes accept "
                                "in critical mode", getScopeName() ) ;
                     }
                     // Node is not in enforced mode, need to check if all alive nodes and
                     // all critical nodes agree
                     else if ( _info()->aliveSize() <= ( _accepted() + 1 ) )
                     {
                        next = CLS_ELECTION_STATUS_PRIMARY ;
                        PD_LOG( PDEVENT, "%s Vote: change to primary by all alive nodes accept "
                                "in critical mode", getScopeName() ) ;
                     }
                     else
                     {
                        next = id() ;
                     }
                  }
                  else
                  {
                     next = id() ;
                  }
               }
               else
               {
                  next = id() ;
               }
            }
            // If is in enforced critical mode, ignore the reject result, wait until timeout
            else if ( ! isLocation() && _info()->enforcedGrpMode &&
                      CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode )
            {
               next = id() ;
            }
            else
            {
               next = CLS_ELECTION_STATUS_SILENCE ;
            }
         }
      }
      else
      {
         /// error msg
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_HDINPUT ) ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_HDTMOUT, "_clsVSAnnounce::handleTimeout" )
   void _clsVSAnnounce::handleTimeout( const UINT32 &millisec,
                                       INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_HDTMOUT ) ;
      _timeout() += millisec ;
      if ( CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( _isAccepted() )
         {
            next = CLS_ELECTION_STATUS_PRIMARY ;

            if ( CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode && ! isLocation() )
            {
               if ( _info()->enforcedGrpMode )
               {
                  PD_LOG( PDEVENT, "%s Vote: change to primary by timeout "
                          "in enforced critical mode", getScopeName() ) ;
               }
               else
               {
                  PD_LOG( PDEVENT, "%s Vote: change to primary by timeout "
                          "in critical mode", getScopeName() ) ;
               }
            }
            else
            {
               PD_LOG( PDEVENT, "%s Vote: change to primary by timeout", getScopeName() ) ;
            }
         }
         else
         {
            next = CLS_ELECTION_STATUS_SILENCE ;
         }
      }
      else
      {
         next = id() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSANN_ACTIVE, "_clsVSAnnounce::active" )
   void _clsVSAnnounce::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSANN_ACTIVE ) ;
      _timeout() = 0 ;
      _accepted() = 0 ;
      _criticalAccepted() = 0 ;

      if ( _info()->groupSize() == 1 )
      {
         next = CLS_ELECTION_STATUS_PRIMARY ;
      }
      else
      {
         next = id() ;
         _announce() ;
      }
      PD_TRACE_EXIT ( SDB__CLSVSANN_ACTIVE ) ;
      return ;
   }
}
