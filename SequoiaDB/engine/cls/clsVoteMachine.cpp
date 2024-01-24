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

   Source File Name = clsVoteMachine.cpp

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

#include "clsVoteMachine.hpp"
#include "netRouteAgent.hpp"
#include "clsVSSilence.hpp"
#include "clsVSSecondary.hpp"
#include "clsVSVote.hpp"
#include "clsVSAnnounce.hpp"
#include "clsVSPrimary.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"
#include "clsGroupModeJob.hpp"
#include "pmd.hpp"

namespace engine
{
   #define CLS_VOTE_REGISGER_STATUS( status, rc ) \
           do {\
              _clsVoteStatus *s = SDB_OSS_NEW status( _groupInfo,\
                                                      _agent ) ;\
              if ( NULL == s ) \
              {\
                 clear() ;\
                 PD_LOG( PDERROR, "allocate mem failed") ;\
                 rc = SDB_OOM ;\
                 goto error ;\
              }\
              _status.push_back( s ) ;\
           } while( 0 )

   #define CLS_VOTE_FIRST_STATUS             ( 0 )

   #define CLS_VOTE_ACTIVE_STATUS( next, force ) \
           do { \
              SDB_ASSERT( next < (INT32)_status.size() &&\
                          0 <= next,\
                          "valid: 0 <= status < status.size()" ) ;\
              ossScopedLock lock( &_latch ) ; \
              INT32 now = CLS_INVALID_VOTE_ID ;\
              INT32 nextStatus = next ;\
              _clsVoteStatus *prevVS = _current ; \
              if ( NULL != _current )\
              {\
                 now = _current->id() ;\
              }\
              _current = _status.at( next ) ;\
              while ( now != nextStatus )\
              {\
                 if ( prevVS ) { prevVS->deactive() ; } \
                 prevVS = _current ; \
                 PD_LOG( PDINFO, "%s: vote change to %s", \
                         _current->getScopeName(), \
                         _current->name() ) ;\
                 now = _current->id() ;\
                 if ( force )\
                 {\
                    _current->forceActive( nextStatus ) ;\
                 }\
                 else\
                 {\
                    _current->active( nextStatus ) ;\
                 }\
                 SDB_ASSERT( nextStatus < (INT32)_status.size() &&\
                     0 <= nextStatus,\
                     "valid: 0 <= status < status.size()" ) ;\
                 _current = _status.at( nextStatus ) ;\
              }\
              if ( 0 != _forceMillis ) \
              {\
                 _forceMillis = 0 ;\
              }\
           } while ( 0 )

   _clsVoteMachine::_clsVoteMachine( _clsGroupInfo *info,
                                     _netRouteAgent *agent )
   :_agent( agent ),
    _current( NULL ),
    _groupInfo( info ),
    _shadowTimeout( 0 ),
    _shadowForReelect( TRUE ),
    _forceMillis( 0 ),
    _electionWeight( CLS_ELECTION_WEIGHT_DFT ),
    _shadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ),
    _grpModeShadowTime( 0 )
   {
   }

   _clsVoteMachine::~_clsVoteMachine()
   {
      clear() ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_INIT, "_clsVoteMachine::init" )
   INT32 _clsVoteMachine::init()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__CLSVTMH_INIT ) ;
      /// according to cls_ELECTION_STATUS
      CLS_VOTE_REGISGER_STATUS( _clsVSSilence, rc ) ;
      CLS_VOTE_REGISGER_STATUS( _clsVSSecondary, rc ) ;
      CLS_VOTE_REGISGER_STATUS( _clsVSVote, rc ) ;
      CLS_VOTE_REGISGER_STATUS( _clsVSAnnounce, rc ) ;
      CLS_VOTE_REGISGER_STATUS( _clsVSPrimary, rc ) ;

      CLS_VOTE_ACTIVE_STATUS( CLS_VOTE_FIRST_STATUS, FALSE ) ;
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTMH_INIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_CLEAR, "_clsVoteMachine::clear" )
   void _clsVoteMachine::clear()
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_CLEAR ) ;
      vector<_clsVoteStatus *>::iterator itr = _status.begin() ;
      for ( ; itr != _status.end(); itr++ )
      {
         SDB_OSS_DEL *itr ;
         *itr = NULL ;
      }
      _status.clear() ;
      PD_TRACE_EXIT ( SDB__CLSVTMH_CLEAR ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_HDINPUT, "_clsVoteMachine::handleInput" )
   INT32 _clsVoteMachine::handleInput( const MsgHeader *header )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_HDINPUT ) ;
      SDB_ASSERT( NULL != header, "msg should not be NULL" ) ;

      INT32 rc = SDB_OK ;
      INT32 next = CLS_INVALID_VOTE_ID ;

      if ( !_current )
      {
         rc = SDB_INVALIDARG ;
         goto done ;
      }
      if ( CLS_GROUP_MODE_MAINTENANCE != _groupInfo->localGrpMode )
      {
         rc = _current->handleInput( header, next ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
         CLS_VOTE_ACTIVE_STATUS( next, FALSE ) ;
      }
   done:
      PD_TRACE_EXITRC ( SDB__CLSVTMH_HDINPUT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_HDTMOUT, "_clsVoteMachine::handleTimeout" )
   void _clsVoteMachine::handleTimeout( const UINT32 &millisec )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_HDTMOUT ) ;

      if ( _shadowTimeout > millisec )
      {
         _shadowTimeout -= millisec ;
      }
      else
      {
         _shadowTimeout = 0 ;
         if ( !_shadowForReelect )
         {
            // if the shadow wight is not set for reelect,
            // it should be timeout to restore
            _shadowWeight = CLS_ELECTION_WEIGHT_USR_MIN ;
            _shadowForReelect = TRUE ;
         }
      }

      // Handle group mode shadow time
      if ( _grpModeShadowTime > 0 )
      {
         _grpModeShadowTime -= millisec ;

         // Remove group mode
         if ( _grpModeShadowTime <= 0 )
         {
            ossScopedRWLock lock( &_groupInfo->mtx, EXCLUSIVE ) ;
            if ( CLS_GROUP_MODE_CRITICAL == _groupInfo->grpMode.mode )
            {
               // Remove reelect targetNode flag, if this flag is use for critical mode instead of reelectGroup
               if ( 0 == _shadowTimeout )
               {
                  resetElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
               }

               // Remove critical mode info
               resetElectionWeight( CLS_ELECTION_WEIGHT_CRITICAL_NODE ) ;
               _groupInfo->localGrpMode = CLS_GROUP_MODE_NONE ;
               _groupInfo->grpMode.reset() ;
               _groupInfo->enforcedGrpMode = FALSE ;
            }
            _grpModeShadowTime = 0 ;
         }
      }

      if ( !_current )
      {
         goto done ;
      }
      else if ( 0 < _forceMillis )
      {
         if ( _forceMillis <= millisec )
         {
            _forceMillis = 0 ;
         }
         else
         {
            _forceMillis -= millisec ;
         }
      }
      else if ( CLS_GROUP_MODE_MAINTENANCE != _groupInfo->localGrpMode )
      {
         INT32 next = CLS_INVALID_VOTE_ID ;
         _current->handleTimeout( millisec, next ) ;
         CLS_VOTE_ACTIVE_STATUS( next, FALSE ) ;
         goto done ;
      }
   done :
      PD_TRACE_EXIT ( SDB__CLSVTMH_HDTMOUT ) ;
      return ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_FORCE, "_clsVoteMachine::force" )
   void _clsVoteMachine::force( const INT32 &id, UINT32 millis )
   {
      PD_TRACE_ENTRY ( SDB__CLSVTMH_FORCE ) ;
      CLS_VOTE_ACTIVE_STATUS( id, TRUE ) ;
      if ( 0 < millis )
      {
         _forceMillis = millis ;
      }
      PD_TRACE_EXIT ( SDB__CLSVTMH_FORCE ) ;
      return ;
   }

   BOOLEAN _clsVoteMachine::isStatus( const INT32 &id ) const
   {
      if ( _current )
      {
         return id == _current->id() ? TRUE : FALSE ;
      }
      return FALSE ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVTMH_SETGRPMODE, "_clsVoteMachine::setGrpMode" )
   INT32 _clsVoteMachine::setGrpMode( const clsGroupMode &grpMode,
                                      const INT32 &shadowTime,
                                      const BOOLEAN &isLocalMode,
                                      const BOOLEAN &enforced )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSVTMH_SETGRPMODE ) ;

      _grpModeShadowTime = shadowTime ;

      {
         ossScopedRWLock lock( &_groupInfo->mtx, EXCLUSIVE ) ;

         _groupInfo->enforcedGrpMode = enforced ;

         // Remove local and global group mode
         if ( CLS_GROUP_MODE_NONE == grpMode.mode )
         {
            if ( grpMode.mode != _groupInfo->grpMode.mode )
            {
               resetElectionWeight( CLS_ELECTION_WEIGHT_CRITICAL_NODE ) ;
               _groupInfo->grpMode.reset() ;
               _groupInfo->localGrpMode = CLS_GROUP_MODE_NONE ;
            }
         }
         else
         {
            // Set local group mode to CLS_GROUP_MODE_CRITICAL/CLS_GROUP_MODE_MAINTENANCE
            if ( isLocalMode )
            {
               if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
               {
                  // Set critical mode flag
                  setElectionWeight( CLS_ELECTION_WEIGHT_CRITICAL_NODE ) ;
                  _groupInfo->localGrpMode = CLS_GROUP_MODE_CRITICAL ;

                  // shadowTime < 0 means keep this mode forever, we need to remove targetNode flag
                  if ( shadowTime < 0 )
                  {
                     resetElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
                  }
                  // shadowTime > 0 means keep this mode temporary, we need to add targetNode flag
                  else
                  {
                     setElectionWeight( CLS_ELECTION_WEIGHT_REELECT_TARGET_NODE ) ;
                  }
               }
               else if ( CLS_GROUP_MODE_MAINTENANCE == grpMode.mode )
               {
                  _groupInfo->localGrpMode = CLS_GROUP_MODE_MAINTENANCE ;
               }
            }
            // Reset local group mode to CLS_GROUP_MODE_NONE
            else
            {
               if ( CLS_GROUP_MODE_CRITICAL == grpMode.mode )
               {
                  // Reset critical mode flag
                  resetElectionWeight( CLS_ELECTION_WEIGHT_CRITICAL_NODE ) ;
               }
               _groupInfo->localGrpMode = CLS_GROUP_MODE_NONE ;
            }

            // Set global group mode
            clsGroupMode *pGrpMode = SDB_OSS_NEW clsGroupMode( grpMode ) ;
            if ( NULL == pGrpMode )
            {
               PD_LOG( PDWARNING, "Failed to allocate memory for clsGroupMode" ) ;
            }

            pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
            rc = eduMgr->postEDUPost( eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ),
                                      PMD_EDU_EVENT_UPDATE_GRPMODE,
                                      PMD_EDU_MEM_NONE, pGrpMode ) ;
            if ( SDB_OK != rc )
            {
               if ( NULL != pGrpMode )
               {
                  SDB_OSS_DEL pGrpMode ;
                  pGrpMode = NULL ;
               }
               PD_LOG( PDERROR, "Failed to post update group mode event to edu" ) ;
               goto error ;
            }
         }

         // Save global group mode to _clsSharingStatus
         VEC_GRPMODE_ITEM::const_iterator grpModeItr ;
         CLS_NODE_STATUS_MAP::iterator nodeItr = _groupInfo->info.begin() ;
         UINT8 remoteLocationNodeSize = 0 ;

         while ( _groupInfo->info.end() != nodeItr )
         {
            _clsSharingStatus &status = nodeItr->second ;
            grpModeItr = grpMode.grpModeInfo.begin() ;

            while ( grpMode.grpModeInfo.end() != grpModeItr )
            {
               if ( ( INVALID_NODEID != grpModeItr->nodeID &&
                      status.beat.identity.columns.nodeID == grpModeItr->nodeID ) ||
                    ( MSG_INVALID_LOCATIONID != status.beat.locationID &&
                      status.beat.locationID == grpModeItr->locationID ) )
               {
                  status.grpMode = grpMode.mode ;
                  break ;
               }
               ++grpModeItr ;
            }
            if ( grpMode.grpModeInfo.end() == grpModeItr )
            {
               status.grpMode = CLS_GROUP_MODE_NONE ;
            }

            if ( !status.isAffinitiveLocation &&
                 !status.isInMaintenanceMode() )
            {
               remoteLocationNodeSize ++ ;
            }

            ++nodeItr ;
         }
         if ( CLS_GROUP_MODE_MAINTENANCE == _groupInfo->grpMode.mode )
         {
            _groupInfo->remoteLocationNodeSize = remoteLocationNodeSize ;
         }
      }

   done:
      PD_TRACE_EXITRC ( SDB__CLSVTMH_SETGRPMODE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   INT32 _clsVoteMachine::startCriticalModeMonitor()
   {
      INT32 rc = SDB_OK ;
      ossScopedRWLock lock( &_groupInfo->mtx, SHARED ) ;

      SDB_ASSERT( 1 == _groupInfo->grpMode.grpModeInfo.size(),
                  "grpModeInfo's item number should be 1" ) ;

      rc = clsStartCriticalModeMonitor( _groupInfo ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to start critical mode monitor, rc: %d", rc ) ;
      }

      return rc ;
   }

   INT32 _clsVoteMachine::startMaintenanceModeMonitor()
   {
      INT32 rc = SDB_OK ;
      ossScopedRWLock lock( &_groupInfo->mtx, SHARED ) ;

      rc = clsStartMaintenanceModeMonitor( _groupInfo ) ; 
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "Failed to start maintenance mode monitor, rc: %d", rc ) ;
      }

      return rc ;
   }
}
