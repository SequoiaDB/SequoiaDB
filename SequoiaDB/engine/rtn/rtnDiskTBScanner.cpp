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

   Source File Name = rtnDiskTBScanner.cpp

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

#include "rtnDiskTBScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "interface/IOperationContext.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnDiskTBScanner define
    */
   _rtnDiskTBScanner::_rtnDiskTBScanner( dmsStorageUnit *su,
                                         dmsMBContext *mbContext,
                                         const dmsRecordID &startRID,
                                         BOOLEAN isAfterStartRID,
                                         INT32 direction,
                                         BOOLEAN isAsync,
                                         pmdEDUCB *cb )
   : _rtnTBScanner( su, mbContext, startRID, isAfterStartRID, direction, isAsync, cb )
   {
   }

   _rtnDiskTBScanner::~_rtnDiskTBScanner()
   {
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_ADVANCE, "_rtnDiskTBScanner::advance" )
   INT32 _rtnDiskTBScanner::advance( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_ADVANCE ) ;

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
         rc = _cursorPtr->advance( _cb ) ;
         if ( SDB_DMS_EOC == rc )
         {
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
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_RESUMESCAN, "_rtnDiskTBScanner::resumeScan" )
   INT32 _rtnDiskTBScanner::resumeScan( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_RESUMESCAN ) ;

      if ( !_init || !_savedRID.isValid() )
      {
         isCursorSame = TRUE ;
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
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_RESUMESCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_PAUSESCAN, "_rtnDiskTBScanner::pauseScan" )
   INT32 _rtnDiskTBScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_PAUSESCAN ) ;

      if ( !_init || _isEOF )
      {
         goto done ;
      }

      if ( !_savedRID.isValid() )
      {
         rc = _cursorPtr->getCurrentRecordID( _savedRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;
      }

      rc = _cursorPtr->pause( _cb ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause cursor, rc: %d", rc ) ;

      PD_LOG( PDDEBUG, "Pause in recordID [extent: %u, offset: %u]",
              _savedRID._extent, _savedRID._offset ) ;
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_PAUSESCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_CHECKSNAPSHOTID, "_rtnDiskTBScanner::checkSnapshotID" )
   INT32 _rtnDiskTBScanner::checkSnapshotID( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_CHECKSNAPSHOTID ) ;

      isCursorSame = _mbContext->mbStat()->_snapshotID.compare(
                                                _cursorPtr->getSnapshotID() ) ;

      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_CHECKSNAPSHOTID, rc ) ;

      return rc ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_GETCURRID, "_rtnDiskTBScanner::getCurrentRID" )
   INT32 _rtnDiskTBScanner::getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_GETCURRID ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get record ID, cursor is clsoed" ) ;

      rc = _cursorPtr->getCurrentRecordID( nextRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record ID, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_GETCURRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_GETCURREC, "_rtnDiskTBScanner::getCurrentRecord" )
   INT32 _rtnDiskTBScanner::getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_GETCURREC ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to get record, cursor is clsoed" ) ;

      rc = _cursorPtr->getCurrentRecord( recordData ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get record, rc: %d", rc ) ;

      DMS_MON_OP_COUNT_INC( _cb->getMonAppCB(), MON_DATA_READ, 1 ) ;
      DMS_MON_OP_COUNT_INC( _cb->getMonAppCB(), MON_READ, 1 ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN__FIRSTINIT, "_rtnDiskTBScanner::_firstInit" )
   INT32 _rtnDiskTBScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN__FIRSTINIT ) ;

      rc = _mbContext->getCollPtr()->createDataCursor( _cursorPtr,
                                                       _startRID,
                                                       _isAfterStartRID,
                                                       _direction > 0 ? TRUE : FALSE,
                                                       _isAsync,
                                                       _cb ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to create data cursor on "
                   "collection [%s.%s], rc: %d", _su->getSUName(),
                   _mbContext->clName(), rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN_RELOCATERID, "_rtnDiskTBScanner::relocateRID" )
   INT32 _rtnDiskTBScanner::relocateRID( const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN_RELOCATERID ) ;

      BOOLEAN isFound = FALSE ;
      rc = _relocateRID( rid, isFound ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto error ;
      }

      rc = _cursorPtr->getCurrentRecordID( _savedRID ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;

      _relocatedRID = _savedRID ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN_RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNDISKTBSCAN__RELOCATERID, "_rtnDiskTBScanner::_relocateRID" )
   INT32 _rtnDiskTBScanner::_relocateRID( const dmsRecordID &rid, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNDISKTBSCAN__RELOCATERID ) ;

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

      PD_CHECK( _cursorPtr, SDB_DMS_CONTEXT_IS_CLOSE, error, PDERROR,
                "Failed to relocate record, cursor is clsoed" ) ;

      rc = _cursorPtr->locate( rid, FALSE, _cb, isFound ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         goto done ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to relocate record, rc: %d", rc ) ;

      _isEOF = FALSE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNDISKTBSCAN__RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
