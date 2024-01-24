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

   Source File Name = clsVSPrimary.cpp

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

#include "clsVSPrimary.hpp"
#include "pmd.hpp"
#include "pmdCB.hpp"
#include "pdTrace.hpp"
#include "clsTrace.hpp"

namespace engine
{
   #define CLS_PRIMARY_UP_NOTIFY_TIMES          ( 60 )

   _clsVSPrimary::_clsVSPrimary( _clsGroupInfo *info, _netRouteAgent *agent )
   : _clsVoteStatus( info, agent, CLS_ELECTION_STATUS_PRIMARY)
   {

   }

   _clsVSPrimary::~_clsVSPrimary()
   {

   }

   INT32 _clsVSPrimary::handleInput( const MsgHeader *header,
                                     INT32 &next )
   {
      /// primary do not accept any request
      next = id() ;
      return SDB_OK ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSPMY_HDTMOUT, "_clsVSPrimary::handleTimeout" )
   void _clsVSPrimary::handleTimeout( const UINT32 &millisec,
                                      INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSPMY_HDTMOUT ) ;
      _timeout() += millisec ;
      if ( CLS_VOTE_CS_TIME <= _timeout() )
      {
         if ( CLS_IS_MAJORITY( _info()->aliveSize(), _info()->groupSize() ) ||
              ( ! isLocation() &&
                CLS_GROUP_MODE_CRITICAL == _info()->localGrpMode &&
                CLS_IS_MAJORITY( _info()->criticalAliveSize(), _info()->criticalSize() ) ) )
         {
            _timeout() = 0 ;
            next = id() ;
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
      PD_TRACE_EXIT ( SDB__CLSVSPMY_HDTMOUT ) ;
      return ;
   }

   void _clsVSPrimary::deactive ()
   {
      _MsgCatPrimaryChange msg ;
      UINT32 opKey = MAKE_REPLY_TYPE( msg.header.opCode ) ;
      const _clsCataCallerMeta* pMeta = sdbGetReplCB()->getCataCallerMeta( opKey ) ;

      // Merge Replica Group and Location primary change info together in one msg
      if ( NULL != pMeta && 0 != pMeta->sendTimes )
      {
         _MsgCatPrimaryChange* pMsg = ( _MsgCatPrimaryChange* ) pMeta->header ;
         msg.newPrimary = pMsg->newPrimary ;
         msg.oldPrimary = pMsg->oldPrimary ;
         msg.newLocationPrimary = pMsg->newLocationPrimary ;
         msg.oldLocationPrimary = pMsg->oldLocationPrimary ;
      }

      if ( ! isLocation() )
      {
         // primary change before
         sdbGetClsCB()->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_BEFORE ) ;

         _info()->mtx.lock_w() ;
         if ( _info()->local.value == _info()->primary.value )
         {
            _info()->primary.value = MSG_INVALID_ROUTEID ;
         }
         pmdSetPrimary( FALSE ) ; // set global primary
         msg.newPrimary = _info()->primary ;
         msg.oldPrimary = _info()->local ;
         _info()->mtx.release_w() ;

         // primary change after
         sdbGetClsCB()->ntyPrimaryChange( FALSE, SDB_EVT_OCCUR_AFTER ) ;
      }
      else
      {
          _info()->mtx.lock_w() ;
         if ( _info()->local.value == _info()->primary.value )
         {
            _info()->primary.value = MSG_INVALID_ROUTEID ;
         }
         pmdSetLocationPrimary( FALSE ) ; // Set location primary in pmdSysInfo
         msg.newLocationPrimary = _info()->primary ;
         msg.oldLocationPrimary = _info()->local ;
         msg.locationID = _info()->localLocationID ;
         _info()->mtx.release_w() ;
      }

      sdbGetReplCB()->callCatalog( (MsgHeader *)&msg ) ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__CLSVSPMY_ACTIVE, "_clsVSPrimary::active" )
   void _clsVSPrimary::active( INT32 &next )
   {
      PD_TRACE_ENTRY ( SDB__CLSVSPMY_ACTIVE ) ;
      _timeout() = 0 ;
      next = id() ;
      _MsgCatPrimaryChange msg ;
      UINT32 opKey = MAKE_REPLY_TYPE(msg.header.opCode) ;
      const _clsCataCallerMeta* pMeta = sdbGetReplCB()->getCataCallerMeta( opKey ) ;

      // Merge Replica Group and Location primary change info together in one msg
      if ( NULL != pMeta && 0 != pMeta->sendTimes )
      {
         _MsgCatPrimaryChange* pMsg = ( _MsgCatPrimaryChange* ) pMeta->header ;
         msg.newPrimary = pMsg->newPrimary ;
         msg.oldPrimary = pMsg->oldPrimary ;
         msg.newLocationPrimary = pMsg->newLocationPrimary ;
         msg.oldLocationPrimary = pMsg->oldLocationPrimary ;
      }

      if ( ! isLocation() )
      {
         // before primary
         sdbGetClsCB()->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_BEFORE ) ;

         _info()->mtx.lock_w() ;
         msg.newPrimary = _info()->local ;
         msg.oldPrimary = _info()->primary ;
         _info()->primary = _info()->local ;
         pmdSetPrimary( TRUE ) ; // set global primary
         _info()->mtx.release_w() ;

         sdbGetReplCB()->reelectionDone() ;

         PD_LOG ( PDEVENT, "%s Vote: change to primary", getScopeName() ) ;

         // after primary
         sdbGetClsCB()->ntyPrimaryChange( TRUE, SDB_EVT_OCCUR_AFTER ) ;
      }
      else
      {
         _info()->mtx.lock_w() ;
         msg.newLocationPrimary = _info()->local ;
         msg.oldLocationPrimary = _info()->primary ;
         msg.locationID = _info()->localLocationID ;
         _info()->primary = _info()->local ;
         pmdSetLocationPrimary( TRUE ) ; // Set location primary in pmdSysInfo
         _info()->mtx.release_w() ;

         sdbGetReplCB()->locationReelectionDone() ;

         PD_LOG ( PDEVENT, "%s Vote: node change to primary", getScopeName() ) ;
      }

      sdbGetReplCB()->callCatalog( (MsgHeader *)&msg,
                                   CLS_PRIMARY_UP_NOTIFY_TIMES ) ;

      PD_TRACE_EXIT ( SDB__CLSVSPMY_ACTIVE ) ;
      return ;
   }

}
