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

   Source File Name = clsReelection.cpp

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/01/2015  YW  Initial Draft

   Last Changed =

*******************************************************************************/

#include "clsReelection.hpp"
#include "pd.hpp"
#include "clsTrace.hpp"
#include "pdTrace.hpp"
#include "clsSyncManager.hpp"
#include "clsVoteMachine.hpp"
#include "pmd.hpp"
#include "dpsLogWrapper.hpp"

namespace engine
{
   _clsReelection::_clsReelection( _clsVoteMachine *vote,
                                   _clsSyncManager *syncMgr )
   :_vote( vote ),
    _syncMgr( syncMgr ),
    _level( CLS_REELECTION_LEVEL_NONE )
   {
      SDB_ASSERT( NULL != _vote &&
                  NULL != _syncMgr, "can not be null" ) ;
      _event.signalAll() ;
   }

   _clsReelection::~_clsReelection()
   {

   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION_WAIT, "_clsReelection::wait" )
   INT32 _clsReelection::wait( pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_WAIT ) ;
      UINT32 timePassed = 0 ;
      UINT32 timeout = 600 ;

      if ( CLS_REELECTION_LEVEL_NONE != _level )
      {
         rc = _wait( timePassed, timeout, cb, TRUE ) ;
         if ( SDB_OK != rc )
         {
            goto error ;
         }
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION_WAIT, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   void _clsReelection::signal()
   {
      _event.signalAll() ;
      ossAtomicExchange32( &_level, CLS_REELECTION_LEVEL_NONE ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION_RUN, "_clsReelection::run" )
   INT32 _clsReelection::run( CLS_REELECTION_LEVEL lvl,
                              UINT32 seconds,
                              pmdEDUCB *cb,
                              UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION_RUN ) ;
      UINT32 timePassed = 0 ;
      BOOLEAN resetEvent = FALSE ;

      if ( CLS_REELECTION_LEVEL_1 != lvl &&
           CLS_REELECTION_LEVEL_3 != lvl ) 
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "invalid reelection level:%d", lvl ) ;
         goto error ;
      }

      if ( seconds < 10 )
      {
         rc = SDB_INVALIDARG ;
         PD_LOG( PDERROR, "seconds of reelection should over 10" ) ;
         goto error ;
      }

      if ( !_vote->primaryIsMe() )
      {
         rc = SDB_CLS_NOT_PRIMARY ;
         PD_LOG( PDERROR, "only primary node can reelect" ) ;
         goto error ;
      }
      /// is self
      else if ( 0 != destID && destID == pmdGetNodeID().columns.nodeID )
      {
         // restore
         _vote->setShadowWeight( CLS_ELECTION_WEIGHT_USR_MIN ) ;
         goto done ;
      }

      if ( !ossCompareAndSwap32( &_level, CLS_REELECTION_LEVEL_NONE, lvl ) )
      {
         PD_LOG( PDERROR, "can not do reelection when last"
                 " reelection is not done" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      _event.reset() ;
      resetEvent = TRUE ;

      rc = _wait4AllWriteDone( timePassed, seconds, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "reelection is out of time" ) ;
         rc = SDB_TIMEOUT ;
         goto error ;
      }

      /// we need at least one replication done.
      /// otherwise this node will still be the primary.
      /// WARNING: do not compare with _level.
      if ( CLS_REELECTION_LEVEL_1 < lvl )
      {
         rc = _wait4Replica( timePassed, seconds, cb, destID ) ;
         if ( SDB_OK != rc )
         {
            PD_LOG( PDERROR, "reelection is out of time" ) ;
            rc = SDB_TIMEOUT ;
            goto error ;   
         }   
      }

      rc = _stepDown( timePassed, seconds, cb ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to step down:%d", rc ) ;
         goto error ;
      }

   done:
      if ( resetEvent )
      {
         signal() ;
      }
      PD_TRACE_EXITRC( SDB__CLSREELECTION_RUN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4ALLWRITEDONE, "_clsReelection::_wait4AllWriteDone" )
   INT32 _clsReelection::_wait4AllWriteDone( UINT32 &timePassed,
                                             UINT32 timeout,
                                             pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4ALLWRITEDONE ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      UINT32 writingCount = 0 ;
      UINT32 transCount = 0 ;

      while ( timePassed < timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         writingCount = eduMgr->getWritingEDUCount( -1, 0, EDU_BLOCK_REELECT,
                                                    dpsTransLockId(),
                                                    &transCount ) ;
         if ( 0 == writingCount || writingCount == transCount )
         {
            rc = SDB_OK ;
            break ;
         }

         ossSleepsecs( 1 ) ;
         ++timePassed ;
         rc = SDB_TIMEOUT ;
      }

   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4ALLWRITEDONE, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__WAIT4REPLICA, "_clsReelection::_wait4Replica" )
   INT32 _clsReelection::_wait4Replica( UINT32 &timePassed,
                                        UINT32 timeout,
                                        pmdEDUCB *cb,
                                        UINT16 destID )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__WAIT4REPLICA ) ;
      DPS_LSN lsn = pmdGetKRCB()->getDPSCB()->getCurrentLsn() ;
      while ( timePassed < timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         if ( _syncMgr->atLeastOne( lsn.offset, destID ) )
         {
            break ;
         }

         ossSleepsecs( 1 ) ;
         ++timePassed ;
      }

      if ( timeout <= timePassed )
      {
         rc = SDB_TIMEOUT ;
         goto error ;
      } 
   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__WAIT4REPLICA, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION (SDB__CLSREELECTION__STEPDOWN, "_clsReelection::_stepDown" )
   INT32 _clsReelection::_stepDown( UINT32 &timePassed,
                                    UINT32 timeout,
                                    pmdEDUCB *cb )
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY( SDB__CLSREELECTION__STEPDOWN ) ;
      pmdEDUMgr *eduMgr = pmdGetKRCB()->getEDUMgr() ;
      EDUID eduID = eduMgr->getSystemEDU( EDU_TYPE_CLUSTER ) ;
      rc = eduMgr->postEDUPost( eduID, PMD_EDU_EVENT_STEP_DOWN ) ;
      if ( SDB_OK != rc )
      {
         PD_LOG( PDERROR, "failed to post event to repl cb:%d", rc ) ;
         goto error ;
      }

      rc = _wait( timePassed, timeout, cb, FALSE ) ;
      if ( SDB_OK != rc )
      {
         goto error ;
      }
   done:
      PD_TRACE_EXITRC( SDB__CLSREELECTION__STEPDOWN, rc ) ;
      return rc ;
   error:
      goto done ;
   }

   INT32 _clsReelection::_wait( UINT32 &timePassed,
                                UINT32 timeout,
                                pmdEDUCB *cb,
                                BOOLEAN canSetBlock )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN hasBlock = FALSE ;

      while ( timePassed <= timeout )
      {
         if ( cb->isInterrupted() )
         {
            rc = SDB_APP_INTERRUPT ;
            goto error ;
         }

         rc = _event.wait( OSS_ONE_SEC ) ;
         if ( SDB_OK == rc )
         {
            break ;
         }
         else if ( SDB_TIMEOUT == rc )
         {
            if ( !hasBlock && canSetBlock )
            {
               cb->setBlock( EDU_BLOCK_REELECT, "Waiting for reelect" ) ;
               hasBlock = TRUE ;
            }
            rc = SDB_OK ;
            ++timePassed ;
            continue ;
         }
         else
         {
            PD_LOG( PDERROR, "failed to wait:%d", rc ) ;
            goto error ;
         }
      }

      if ( timeout < timePassed )
      {
         rc = SDB_TIMEOUT ;
         goto error ;
      }

   done:
      if ( hasBlock )
      {
         cb->unsetBlock() ;
      }
      return rc ;
   error:
      goto done ;
   }
}

