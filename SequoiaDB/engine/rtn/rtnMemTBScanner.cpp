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

   Source File Name = rtnMemTBScanner.cpp

   Descriptive Name = RunTime Table Scanner Header

   When/how to use:

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/20/2023  HGM Initial Draft

   Last Changed =

*******************************************************************************/

#include "rtnMemTBScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnMemTBScanner define
    */
   _rtnMemTBScanner::_rtnMemTBScanner( dmsStorageUnit *su,
                                       dmsMBContext *mbContext,
                                       const dmsRecordID &startRID,
                                       BOOLEAN isAfterStartRID,
                                       INT32 direction,
                                       pmdEDUCB *cb )
   : _rtnTBScanner( su, mbContext, startRID, isAfterStartRID, direction, FALSE, cb ),
     _transCB( pmdGetKRCB()->getTransCB() )
   {
   }

   _rtnMemTBScanner::~_rtnMemTBScanner()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_ADVANCE, "_rtnMemTBScanner::advance" )
   INT32 _rtnMemTBScanner::advance( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_ADVANCE ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      if ( !_init )
      {
         rc = _firstInit() ;
         if ( SDB_DMS_EOC == rc )
         {
            _init = TRUE ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

         _init = TRUE ;
      }
      else if ( !_savedRID.isValid() )
      {
         _curRID = _oldVerUnit->getNextDeletingRID( _curRID ) ;
         if ( !_curRID.isValid() )
         {
            rc = SDB_DMS_EOC ;
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
      }
      else
      {
         _savedRID.reset() ;
         _relocatedRID.reset() ;
      }

      rc = getCurrentRID( rid ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get currnent record ID, rc: %d", rc ) ;

   done:
      if ( SDB_DMS_EOC == rc && !_isEOF )
      {
         _isEOF = TRUE ;
      }
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_RESUMESCAN, "_rtnMemTBScanner::resumeScan" )
   INT32 _rtnMemTBScanner::resumeScan( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_RESUMESCAN ) ;

      isCursorSame = TRUE ;

      rc = _initOldVerUnit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init old version unit, rc: %d", rc ) ;

      if ( !_oldVerUnit )
      {
         _isEOF = TRUE ;
         goto done ;
      }

      if ( !_init || !_savedRID.isValid() )
      {
         goto done ;
      }

      rc = _relocateRID( _savedRID, isCursorSame ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to relocate record, rc: %d", rc ) ;

      PD_LOG( PDDEBUG, "Relocate with rid(%u,%u), found(%d)",
              _savedRID._extent, _savedRID._offset, isCursorSame ) ;

      if ( isCursorSame && _relocatedRID != _savedRID )
      {
         _savedRID.reset() ;
      }
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_RESUMESCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_PAUSESCAN, "_rtnMemTBScanner::pauseScan" )
   INT32 _rtnMemTBScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_PAUSESCAN ) ;

      if ( !_init || _isEOF )
      {
         goto done ;
      }

      _savedRID = _curRID ;
      PD_LOG( PDDEBUG, "Pause in recordID [extent: %u, offset: %u]",
              _savedRID._extent, _savedRID._offset ) ;
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_PAUSESCAN, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_CHECKSNAPSHOTID, "_rtnMemTBScanner::checkSnapshotID" )
   INT32 _rtnMemTBScanner::checkSnapshotID( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_CHECKSNAPSHOTID ) ;

      isCursorSame = TRUE ;

      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_CHECKSNAPSHOTID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_GETCURRID, "_rtnMemTBScanner::getCurrentRID" )
   INT32 _rtnMemTBScanner::getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_GETCURRID ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      nextRID = _curRID ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_GETCURRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_GETCURREC, "_rtnMemTBScanner::getCurrentRecord" )
   INT32 _rtnMemTBScanner::getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_GETCURREC ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      recordData.reset() ;

      DMS_MON_OP_COUNT_INC( _cb->getMonAppCB(), MON_DATA_READ, 1 ) ;
      DMS_MON_OP_COUNT_INC( _cb->getMonAppCB(), MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN__FIRSTINIT, "_rtnMemTBScanner::_firstInit" )
   INT32 _rtnMemTBScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN__FIRSTINIT ) ;

      _curRID.reset() ;

      rc = _initOldVerUnit() ;
      PD_RC_CHECK( rc, PDERROR, "Failed to init old version unit, rc: %d", rc ) ;

      if ( !_oldVerUnit )
      {
         _isEOF = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( _startRID.isValid() )
      {
         if ( _isAfterStartRID )
         {
            if ( _startRID.isMax() )
            {
               _isEOF = TRUE ;
               rc = SDB_DMS_EOC ;
               goto error ;
            }
            _curRID.fromUINT64( _startRID.toUINT64() + 1 ) ;
         }
         else
         {
            if ( !_startRID.isMin() )
            {
               _curRID = _startRID ;
            }
         }
      }
      _curRID = _oldVerUnit->getNextDeletingRID( _curRID ) ;
      if ( !_curRID.isValid() )
      {
         _isEOF = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN_RELOCATERID, "_rtnMemTBScanner::relocateRID" )
   INT32 _rtnMemTBScanner::relocateRID( const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN_RELOCATERID ) ;

      BOOLEAN isFound = FALSE ;
      rc = _relocateRID( rid, isFound ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto error ;
      }

      _savedRID = _curRID ;
      _relocatedRID = _savedRID ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN_RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN__RELOCATERID, "_rtnMemTBScanner::_relocateRID" )
   INT32 _rtnMemTBScanner::_relocateRID( const dmsRecordID &rid, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN__RELOCATERID ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      if ( !_init )
      {
         rc = _firstInit() ;
         if ( SDB_DMS_EOC == rc )
         {
            _isEOF = TRUE ;
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to init scanner, rc: %d", rc ) ;

         _init = TRUE ;
      }

      isFound = _oldVerUnit->isDeletingRID( rid ) ;
      if ( isFound )
      {
         _curRID = rid ;
      }
      else
      {
         _curRID = _oldVerUnit->getNextDeletingRID( rid ) ;
         if ( !_curRID.isValid() )
         {
            _isEOF = TRUE ;
            rc = SDB_DMS_EOC ;
            goto error ;
         }
      }

      _isEOF = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN__RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMEMTBSCAN__INITOLDVERUNIT, "_rtnMemTBScanner::_initOldVerUnit" )
   INT32 _rtnMemTBScanner::_initOldVerUnit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMEMTBSCAN__INITOLDVERUNIT ) ;

      if ( NULL == _transCB || NULL == _transCB->getOldVCB() )
      {
         PD_LOG( PDERROR, "TransCB or OldVerionCB is NULL" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( !_oldVerUnit )
      {
         _oldVerUnit =
               _transCB->getOldVCB()->getOldVersionUnit( _su->CSID(),
                                                         _mbContext->mbID() ) ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMEMTBSCAN__INITOLDVERUNIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
