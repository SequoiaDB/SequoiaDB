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

   Source File Name = rtnMergeTBScanner.cpp

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

#include "rtnMergeTBScanner.hpp"
#include "dmsStorageUnit.hpp"
#include "pdTrace.hpp"
#include "rtnScannerFactory.hpp"
#include "rtnTrace.hpp"

using namespace std ;
using namespace bson ;

namespace engine
{

   /*
      _rtnMergeTBScanner define
    */
   _rtnMergeTBScanner::_rtnMergeTBScanner( dmsStorageUnit *su,
                                           dmsMBContext *mbContext,
                                           const dmsRecordID &startRID,
                                           BOOLEAN isAfterStartRID,
                                           INT32 direction,
                                           pmdEDUCB *cb )
   : _rtnTBScanner( su, mbContext, startRID, isAfterStartRID, direction, FALSE, cb ),
     _fromDir( SCAN_NONE ),
     _savedDir( SCAN_NONE ),
     _leftTBScanner( NULL ),
     _rightTBScanner( NULL ),
     _leftType( SCANNER_TYPE_MEM_TREE ),
     _rightType( SCANNER_TYPE_DISK ),
     _leftEnabled( FALSE ),
     _rightEnabled( FALSE )
   {
   }

   _rtnMergeTBScanner::~_rtnMergeTBScanner()
   {
      rtnScannerFactory f ;
      if ( _leftTBScanner )
      {
         f.releaseScanner( _leftTBScanner ) ;
         _leftTBScanner = NULL ;
      }
      if ( _rightTBScanner )
      {
         f.releaseScanner( _rightTBScanner ) ;
         _rightTBScanner = NULL ;
      }
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_INIT, "_rtnMergeTBScanner::init" )
   INT32 _rtnMergeTBScanner::init()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_INIT ) ;

      rc = _rtnTBScanner::init() ;
      if ( rc )
      {
         goto error ;
      }

      rc = _createScanner( _leftType, _leftTBScanner ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create left scanner, rc: %d", rc ) ;
      _leftEnabled = TRUE ;

      rc = _createScanner( _rightType, _rightTBScanner ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create right scanner, rc: %d", rc ) ;
      _rightEnabled = TRUE ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_INIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_ADVANCE, "_rtnMergeTBScanner::advance" )
   INT32 _rtnMergeTBScanner::advance( dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_ADVANCE ) ;

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
         rc = _advance() ;
         if ( SDB_DMS_EOC == rc )
         {
            goto done ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to advance cursor, rc: %d", rc ) ;
      }
      else
      {
         _fromDir = _savedDir ;
         _savedRID.reset() ;
         _savedDir = SCAN_NONE ;
         _relocatedRID.reset() ;

         if ( SCAN_LEFT == _fromDir )
         {
            _leftTBScanner->resetSavedRID() ;
         }
         else if ( SCAN_RIGHT == _fromDir )
         {
            _rightTBScanner->resetSavedRID() ;
         }
         else
         {
            PD_LOG( PDERROR, "Failed to advance scanner, unkonwn child scanner" ) ;
            rc = SDB_SYS ;
            goto error ;
         }
      }

      rc = getCurrentRID( rid ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;

   done:
      if ( SDB_DMS_EOC == rc && !_isEOF )
      {
         _isEOF = TRUE ;
      }
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_RESUMESCAN, "_rtnMergeTBScanner::resumeScan" )
   INT32 _rtnMergeTBScanner::resumeScan( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_RESUMESCAN ) ;

      BOOLEAN isLeftEOF = FALSE, isRightEOF = FALSE ;
      BOOLEAN isLeftCursorSame = FALSE, isRightCursorSame = FALSE ;

      if ( !_init || !_savedRID.isValid() )
      {
         isCursorSame = TRUE ;
         goto done ;
      }

      SDB_ASSERT( SCAN_NONE != _savedDir, "Invalid saved direction" ) ;

      if ( _leftEnabled )
      {
         rc = _leftTBScanner->resumeScan( isLeftCursorSame ) ;
         if ( SDB_DMS_EOC == rc || _leftTBScanner->isEOF() )
         {
            isLeftEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to resume left scanner, rc: %d", rc ) ;
      }
      else
      {
         isLeftEOF = TRUE ;
      }
      if ( _rightEnabled )
      {
         rc = _rightTBScanner->resumeScan( isRightCursorSame ) ;
         if ( SDB_DMS_EOC == rc || _rightTBScanner->isEOF()  )
         {
            isRightEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to resume right scanner, rc: %d", rc ) ;
      }
      else
      {
         isRightEOF = TRUE ;
      }

      if ( SCAN_LEFT == _savedDir )
      {
         isCursorSame = isLeftCursorSame ;
      }
      else if ( SCAN_RIGHT == _savedDir )
      {
         isCursorSame = isRightCursorSame ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to resume scanner, unkonwn child scanner" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      if ( SCAN_LEFT == _savedDir && _rightEnabled )
      {
         rc = _rightTBScanner->relocateRID( _savedRID ) ;
         if ( SDB_DMS_EOC == rc || _rightTBScanner->isEOF()  )
         {
            isRightEOF = TRUE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            isRightEOF = FALSE ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate right scanner, rc: %d", rc ) ;
      }
      else if ( SCAN_RIGHT == _savedDir && _leftEnabled )
      {
         rc = _leftTBScanner->relocateRID( _savedRID ) ;
         if ( SDB_DMS_EOC == rc || _leftTBScanner->isEOF() )
         {
            isLeftEOF = TRUE ;
            rc = SDB_OK ;
         }
         else if ( SDB_OK == rc )
         {
            isLeftEOF = FALSE ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate left scanner, rc: %d", rc ) ;
      }

      if ( isRightEOF && isLeftEOF )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto done ;
      }

      PD_LOG( PDDEBUG, "Relocate with rid(%u,%u), found(%d)",
              _savedRID._extent, _savedRID._offset, isCursorSame ) ;

      if ( isCursorSame && _relocatedRID != _savedRID )
      {
         _savedRID.reset() ;
         _savedDir = SCAN_NONE ;
      }
      else if ( !isCursorSame )
      {
         rc = _chooseFromDir( isLeftEOF, isRightEOF,
                              FALSE, FALSE,
                              dmsRecordID(), dmsRecordID(),
                              _savedDir ) ;
         if ( SDB_DMS_EOC == rc )
         {
            goto error ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to choose from direction, rc: %d", rc ) ;
      }
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_RESUMESCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_PAUSESCAN, "_rtnMergeTBScanner::pauseScan" )
   INT32 _rtnMergeTBScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_PAUSESCAN ) ;

      INT32 rcl = SDB_OK, rcr = SDB_OK ;

      if ( !_init || _isEOF )
      {
         goto done ;
      }

      if ( _leftEnabled )
      {
         rcl = _leftTBScanner->pauseScan() ;
      }
      if ( _rightTBScanner )
      {
         rcr = _rightTBScanner->pauseScan() ;
      }
      rc = rcl ? rcl : rcr ;
      PD_RC_CHECK( rc, PDERROR, "Failed to pause scan, rc: %d", rc ) ;

      _savedRID = _getSavedRIDFromChild() ;
      _savedDir = _fromDir ;
      PD_LOG( PDDEBUG, "Pause in recordID [extent: %u, offset: %u]",
              _savedRID._extent, _savedRID._offset ) ;
      _relocatedRID.reset() ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_PAUSESCAN, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_CHECKSNAPSHOTID, "_rtnMergeTBScanner::checkSnapshotID" )
   INT32 _rtnMergeTBScanner::checkSnapshotID( BOOLEAN &isCursorSame )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_CHECKSNAPSHOTID ) ;

      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;
      if ( SCAN_LEFT == _fromDir )
      {
         rc = _leftTBScanner->checkSnapshotID( isCursorSame ) ;
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         rc = _rightTBScanner->checkSnapshotID( isCursorSame ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to check snapshot ID, unkonwn child scanner" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_CHECKSNAPSHOTID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_GETCURRID, "_rtnMergeTBScanner::getCurrentRID" )
   INT32 _rtnMergeTBScanner::getCurrentRID( dmsRecordID &nextRID )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_GETCURRID ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;
      if ( SCAN_LEFT == _fromDir )
      {
         rc = _leftTBScanner->getCurrentRID( nextRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         rc = _rightTBScanner->getCurrentRID( nextRID ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get current record ID, unkonwn child scanner" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_GETCURRID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_GETCURREC, "_rtnMergeTBScanner::getCurrentRecord" )
   INT32 _rtnMergeTBScanner::getCurrentRecord( dmsRecordData &recordData )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_GETCURREC ) ;

      if ( _isEOF )
      {
         rc = SDB_DMS_EOC ;
         goto error ;
      }

      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;
      if ( SCAN_LEFT == _fromDir )
      {
         rc = _leftTBScanner->getCurrentRecord( recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get current record, rc: %d", rc ) ;
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         rc = _rightTBScanner->getCurrentRecord( recordData ) ;
         PD_RC_CHECK( rc, PDERROR, "Failed to get current record, rc: %d", rc ) ;
      }
      else
      {
         PD_LOG( PDERROR, "Failed to get current record, unkonwn child scanner" ) ;
         rc = SDB_SYS ;
         goto error ;
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_GETCURREC, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN__FIRSTINIT, "_rtnMergeTBScanner::_firstInit" )
   INT32 _rtnMergeTBScanner::_firstInit()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN__FIRSTINIT ) ;

      BOOLEAN isLeftEOF = FALSE, isRightEOF = FALSE ;
      dmsRecordID leftRID, rightRID ;

      _fromDir = SCAN_NONE ;

      if ( _leftEnabled )
      {
         rc = _leftTBScanner->advance( leftRID ) ;
         if ( SDB_DMS_EOC == rc )
         {
            isLeftEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate left scanner, rc: %d", rc ) ;
      }
      else
      {
         isLeftEOF = TRUE ;
      }
      if ( _rightTBScanner )
      {
         rc = _rightTBScanner->advance( rightRID ) ;
         if ( SDB_DMS_EOC == rc )
         {
            isRightEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate right scanner, rc: %d", rc ) ;
      }
      else
      {
         isRightEOF = TRUE ;
      }

      rc = _chooseFromDir( isLeftEOF, isRightEOF,
                           FALSE, FALSE,
                           leftRID, rightRID,
                           _fromDir ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to choose from direction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN__FIRSTINIT, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN__ADVANCE, "_rtnMergeTBScanner::_advance" )
   INT32 _rtnMergeTBScanner::_advance()
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN__ADVANCE ) ;

      BOOLEAN isLeftEOF = FALSE, isRightEOF = FALSE ;
      dmsRecordID leftRID, rightRID ;

      if ( SCAN_LEFT == _fromDir )
      {
         if ( _leftEnabled )
         {
            rc = _leftTBScanner->advance( leftRID ) ;
            if ( SDB_DMS_EOC == rc )
            {
               isLeftEOF = TRUE ;
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to relocate left scanner, rc: %d", rc ) ;
         }
         else
         {
            isLeftEOF = TRUE ;
         }
         if ( _rightEnabled && !_rightTBScanner->isEOF() )
         {
            rc = _rightTBScanner->getCurrentRID( rightRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;
         }
         else
         {
            isRightEOF = TRUE ;
         }
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         if ( _rightEnabled )
         {
            rc = _rightTBScanner->advance( rightRID ) ;
            if ( SDB_DMS_EOC == rc )
            {
               isRightEOF = TRUE ;
               rc = SDB_OK ;
            }
            PD_RC_CHECK( rc, PDERROR, "Failed to relocate right scanner, rc: %d", rc ) ;
         }
         else
         {
            isRightEOF = TRUE ;
         }
         if ( _leftEnabled && !_leftTBScanner->isEOF() )
         {
            rc = _leftTBScanner->getCurrentRID( leftRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get current record ID, rc: %d", rc ) ;
         }
         else
         {
            isLeftEOF = TRUE ;
         }
      }

      rc = _chooseFromDir( isLeftEOF, isRightEOF,
                           FALSE, FALSE,
                           leftRID, rightRID,
                           _fromDir ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to choose from direction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN__ADVANCE, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN_RELOCATERID, "_rtnMergeTBScanner::relocateRID" )
   INT32 _rtnMergeTBScanner::relocateRID( const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN_RELOCATERID ) ;

      BOOLEAN isFound = FALSE ;
      rc = _relocateRID( rid, isFound ) ;
      if ( SDB_DMS_EOC == rc )
      {
         _isEOF = TRUE ;
         rc = SDB_OK ;
         goto error ;
      }

      _savedRID = _getSavedRIDFromChild() ;
      _relocatedRID = _savedRID ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN_RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN__RELOCATERID, "_rtnMergeTBScanner::_relocateRID" )
   INT32 _rtnMergeTBScanner::_relocateRID( const dmsRecordID &rid, BOOLEAN &isFound )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN__RELOCATERID ) ;

      BOOLEAN isLeftFound = FALSE, isRightFound = FALSE ;
      BOOLEAN isLeftEOF = FALSE, isRightEOF = FALSE ;

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

      if ( _leftEnabled )
      {
         rc = _leftTBScanner->relocateRID( rid, isLeftFound ) ;
         if ( SDB_DMS_EOC == rc || _leftTBScanner->isEOF() )
         {
            isLeftEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate from left scanner, rc: %d", rc ) ;
      }
      else
      {
         isLeftEOF = TRUE ;
      }
      if ( _rightEnabled )
      {
         rc = _rightTBScanner->relocateRID( rid, isRightFound ) ;
         if ( SDB_DMS_EOC == rc || _rightTBScanner->isEOF() )
         {
            isRightEOF = TRUE ;
            rc = SDB_OK ;
         }
         PD_RC_CHECK( rc, PDERROR, "Failed to relocate from right scanner, rc: %d", rc ) ;
      }
      else
      {
         isRightEOF = TRUE ;
      }

      isFound = ( isLeftFound || isRightFound ) ? TRUE : FALSE ;

      rc = _chooseFromDir( isLeftEOF, isRightEOF,
                           isLeftFound, isRightFound,
                           dmsRecordID(), dmsRecordID(),
                           _fromDir ) ;
      if ( SDB_DMS_EOC == rc )
      {
         goto error ;
      }
      PD_RC_CHECK( rc, PDERROR, "Failed to choose from direction, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN__RELOCATERID, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN__CREATESCANNER, "_rtnMergeTBScanner::_createScanner" )
   INT32 _rtnMergeTBScanner::_createScanner( rtnScannerType type,
                                             rtnTBScanner *&scanner )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN__CREATESCANNER ) ;

      rtnScannerFactory f ;

      rc = f.createTBScanner( type, _su, _mbContext, FALSE, _cb, scanner ) ;
      PD_RC_CHECK( rc, PDERROR, "Failed to create scanner, rc: %d", rc ) ;

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN__CREATESCANNER, rc ) ;
      return rc ;

   error:
      goto done ;
   }

   const dmsRecordID &_rtnMergeTBScanner::_getSavedRIDFromChild() const
   {
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;
      if ( SCAN_LEFT == _fromDir )
      {
         return _leftTBScanner->getSavedRID() ;
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         return _rightTBScanner->getSavedRID() ;
      }
      return _savedRID ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGETBSCAN__CHOOSEFROMDIR, "_rtnMergeTBScanner::_chooseFromDir" )
   INT32 _rtnMergeTBScanner::_chooseFromDir( BOOLEAN isLeftEOF,
                                             BOOLEAN isRightEOF,
                                             BOOLEAN isLeftFound,
                                             BOOLEAN isRightFound,
                                             const dmsRecordID &leftRID,
                                             const dmsRecordID &rightRID,
                                             INT32 &fromDir )
   {
      INT32 rc = SDB_OK ;

      PD_TRACE_ENTRY( SDB__RTNMERGETBSCAN__CHOOSEFROMDIR ) ;

      if ( isLeftEOF && isRightEOF )
      {
         _isEOF = TRUE ;
         rc = SDB_DMS_EOC ;
         goto error ;
      }
      else if ( isLeftEOF )
      {
         fromDir = SCAN_RIGHT ;
      }
      else if ( isRightEOF )
      {
         fromDir = SCAN_LEFT ;
      }
      else if ( isLeftFound )
      {
         fromDir = SCAN_LEFT ;
      }
      else if ( isRightFound )
      {
         fromDir = SCAN_RIGHT ;
      }
      else
      {
         dmsRecordID curLeftRID = leftRID, curRightRID = rightRID ;
         if ( !curLeftRID.isValid() )
         {
            rc = _leftTBScanner->getCurrentRID( curLeftRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get record ID from "
                         "left scanner, rc: %d", rc ) ;
         }
         if ( !curRightRID.isValid() )
         {
            rc = _rightTBScanner->getCurrentRID( curRightRID ) ;
            PD_RC_CHECK( rc, PDERROR, "Failed to get record ID from "
                         "right scanner, rc: %d", rc ) ;
         }
         if ( curLeftRID <= curRightRID )
         {
            fromDir = SCAN_LEFT ;
         }
         else
         {
            fromDir = SCAN_RIGHT ;
         }
      }

   done:
      PD_TRACE_EXITRC( SDB__RTNMERGETBSCAN__CHOOSEFROMDIR, rc ) ;
      return rc ;

   error:
      goto done ;
   }

}
