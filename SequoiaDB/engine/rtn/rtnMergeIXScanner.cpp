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

   Source File Name = rtnMergeIXScanner.cpp

   Descriptive Name = Runtime Index Merge Scanner

   When/how to use: this program may be used on binary and text-formatted
   versions of Runtime component. This file contains code for index traversal,
   including advance, pause, resume operations.

   Dependencies: N/A

   Restrictions: N/A

   Change Activity:
   defect Date        Who Description
   ====== =========== === ==============================================
          11/14/2012  CYX Initial Draft

   Last Changed =

*******************************************************************************/
#include "rtnMergeIXScanner.hpp"
#include "rtnDiskIXScanner.hpp"
#include "rtnMemIXTreeScanner.hpp"
#include "pdTrace.hpp"
#include "rtnTrace.hpp"
#include "dpsTransCB.hpp"

using namespace bson ;

namespace engine
{

   /*
      RTN_SUB_SCAN_TYPE define
   */
   enum RTN_SUB_SCAN_TYPE
   {
      SCAN_NONE,
      SCAN_LEFT,
      SCAN_RIGHT
   } ;

   _rtnMergeIXScanner::_rtnMergeIXScanner( ixmIndexCB *pIndexCB,
                                           rtnPredicateList *predList,
                                           _dmsStorageUnit  *su,
                                           _pmdEDUCB        *cb,
                                           BOOLEAN indexCBOwnned )
   :_rtnIXScanner( pIndexCB, predList, su, cb, indexCBOwnned )
   {
      _fromDir = SCAN_NONE ;
      _savedRID.reset() ;

      _leftIXScanner  = NULL ;
      _rightIXScanner = NULL ;

      _leftType = SCANNER_TYPE_MEM_TREE ;
      _rightType = SCANNER_TYPE_DISK ;

      _leftEnabled = FALSE ;
      _rightEnabled = FALSE ;
   }

   // destructor, do all rtnDiskIXScanner clean up plus free _memIXScanner
   _rtnMergeIXScanner::~_rtnMergeIXScanner()
   {
      if ( _leftIXScanner )
      {
         SDB_OSS_DEL _leftIXScanner ;
      }
      if ( _rightIXScanner )
      {
         SDB_OSS_DEL _rightIXScanner ;
      }
   }

   void _rtnMergeIXScanner::setSubScannerType( IXScannerType leftType,
                                               IXScannerType rightType )
   {
      _leftType = leftType ;
      _rightType = rightType ;
   }

   INT32 _rtnMergeIXScanner::init()
   {
      INT32 rc = SDB_OK ;

      rc = _rtnIXScanner::init() ;
      if ( rc )
      {
         goto error ;
      }

      /// create scanner
      rc = _createScanner( _leftType, _leftIXScanner ) ;
      if ( rc )
      {
         goto error ;
      }
      _leftEnabled = TRUE ;

      rc = _createScanner( _rightType, _rightIXScanner ) ;
      if ( rc )
      {
         goto error ;
      }
      _rightEnabled = TRUE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   INT32 _rtnMergeIXScanner::_createScanner( IXScannerType type,
                                             _rtnIXScanner *&pScanner )
   {
      INT32 rc = SDB_OK ;

      switch( type )
      {
         case SCANNER_TYPE_DISK :
            pScanner = SDB_OSS_NEW _rtnDiskIXScanner( getIndexCB(),
                                                      getPredicateList(),
                                                      getSu(),
                                                      getEDUCB(),
                                                      getIndexCBOwned() ) ;
            break ;
         case SCANNER_TYPE_MEM_TREE :
            pScanner = SDB_OSS_NEW _rtnMemIXTreeScanner( getIndexCB(),
                                                         getPredicateList(),
                                                         getSu(),
                                                         getEDUCB(),
                                                         getIndexCBOwned() ) ;
            break ;
         default :
            rc = SDB_SYS ;
            PD_LOG( PDERROR, "Invalid scanner type[%d]", type ) ;
            goto error ;
            break ;
      }

      if ( !pScanner )
      {
         rc = SDB_OOM ;
         PD_LOG( PDERROR, "Allocate scanner by type[%d] failed", type ) ;
         goto error ;
      }

      /// init
      rc = pScanner->init() ;
      if ( rc )
      {
         PD_LOG( PDERROR, "Init scanner[Type:%d] failed, rc: %d", type, rc ) ;
         goto error ;
      }

      /// set sub scan shared info to NULL
      pScanner->setShareInfo( NULL ) ;

   done:
      return rc ;
   error:
      goto done ;
   }

   void _rtnMergeIXScanner::setReadonly( BOOLEAN isReadonly )
   {
      _rtnIXScanner::setReadonly( isReadonly ) ;
      SDB_ASSERT( isAvailable(), "Must be available" ) ;

      if ( _leftIXScanner )
      {
         _leftIXScanner->setReadonly( isReadonly ) ;
      }
      if ( _rightIXScanner )
      {
         _rightIXScanner->setReadonly( isReadonly ) ;
      }
   }

   // Description:
   //    This is a merge advance specifically for MemIXTree and Disk scan
   // Dependency:
   //
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEIXSCAN_ADVANCE, "_rtnMergeIXScanner::advance" )
   INT32 _rtnMergeIXScanner::advance( dmsRecordID &rid )
   {
      PD_TRACE_ENTRY ( SDB__RTNMERGEIXSCAN_ADVANCE ) ;

      INT32     rc        = SDB_OK ;   // return rc
      INT32     rcl       = SDB_OK ;   // rc from left scanner
      INT32     rcr       = SDB_OK ;   // rc from right scanner
      BOOLEAN   leftDone  = FALSE ;
      BOOLEAN   rightDone = FALSE ;

      const BSONObj *lObj = NULL ;
      const BSONObj *rObj = NULL ;
      INT32 result        = 0 ;

      if ( !isAvailable() )
      {
         rc = SDB_SYS ;
         goto error ;
      }

   begin:
      if ( !_leftEnabled || _leftIXScanner->eof() ||
           !_leftIXScanner->isAvailable() )
      {
         _lrid.reset() ;
         leftDone = TRUE ;
      }
      if ( !_rightEnabled || _rightIXScanner->eof() ||
           !_rightIXScanner->isAvailable() )
      {
         _rrid.reset() ;
         rightDone = TRUE ;
      }

      if ( leftDone && rightDone )
      {
         rc = SDB_IXM_EOC ;
         goto done ;
      }
      else if ( leftDone )
      {
         rcl = SDB_IXM_EOC ;
         if ( SCAN_LEFT != _fromDir )
         {
            rcr = _rightIXScanner->advance( _rrid ) ;
         }
         _fromDir = SCAN_RIGHT ;
      }
      else if ( rightDone )
      {
         rcr = SDB_IXM_EOC ;
         if ( SCAN_RIGHT != _fromDir )
         {
            rcl = _leftIXScanner->advance( _lrid ) ;
         }
         _fromDir = SCAN_LEFT ;
      }
      // first time advance both
      else if( SCAN_NONE == _fromDir )
      {
         rcl = _leftIXScanner->advance( _lrid ) ;
         rcr = _rightIXScanner->advance( _rrid ) ;
      }
      // otherwise
      else
      {
         if ( SCAN_LEFT == _fromDir )
         {
            // last index used was from left scanner, or left became invalid
            // after resume, we need to advance left
            rcl = _leftIXScanner->advance( _lrid ) ;
         }
         else
         {
            // last index used was from right scanner, or right became invalid
            // after resume, we need to advance right
            rcr = _rightIXScanner->advance( _rrid ) ;
         }
      }

      if ( rcl && SDB_IXM_EOC != rcl )
      {
         rc = rcl ;
         goto error ;
      }
      else if ( rcr && SDB_IXM_EOC != rcr )
      {
         rc = rcr ;
         goto error ;
      }

      if ( rcl && rcr )
      {
         rc = SDB_IXM_EOC ;
         goto done ;
      }
      else if ( rcl )
      {
         rid = _rrid ;
         _fromDir = SCAN_RIGHT ;
         goto found ;
      }
      else if ( rcr )
      {
         rid = _lrid ;
         _fromDir = SCAN_LEFT ;
         goto found ;
      }

      // if we reach here, both scanners did return valid rid,
      // now compare index value from both scanners and return the one
      // come to the front.
      lObj = _leftIXScanner->getCurKeyObj() ;
      rObj = _rightIXScanner->getCurKeyObj() ;
      result = lObj->woCompare( *rObj, _order, false ) * _direction ;
      if ( result < 0 )
      {
         _fromDir = SCAN_LEFT ;
      }
      else if ( result > 0 )
      {
         _fromDir = SCAN_RIGHT ;
      }
      else
      {
         if ( _lrid <= _rrid )
         {
            _fromDir = _direction > 0 ? SCAN_LEFT : SCAN_RIGHT ;
         }
         else
         {
            _fromDir = _direction > 0 ? SCAN_RIGHT : SCAN_LEFT ;
         }
      }

      if ( SCAN_LEFT == _fromDir )
      {
         rid = _lrid ;
      }
      else
      {
         rid = _rrid ;
      }

   found :
      SDB_ASSERT( rid.isValid(), "RID must be valid" ) ;
      try
      {
         if ( !_insert2Dup( rid ) )
         {
            rid.reset() ;
            /// already exist
            goto begin ;
         }
      }
      catch( std::bad_alloc &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_OOM ;
         goto error ;
      }
      catch( std::exception &e )
      {
         PD_LOG( PDERROR, "Occur exception: %s", e.what() ) ;
         rc = SDB_SYS ;
         goto error ;
      }

      // make sure we don't hit maximum size of dedup buffer
      /*if ( _pInfo && _pInfo->isUpToLimit() )
      {
         rc = SDB_IXM_DEDUP_BUF_MAX ;
         goto error ;
      }*/

   done :
      if ( SDB_IXM_EOC == rc )
      {
         _eof = TRUE ;
      }
      PD_TRACE_EXITRC ( SDB__RTNMERGEIXSCAN_ADVANCE, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // restoring the bson key and rid for the current index rid. This is done by
   // comparing the current key/rid pointed by _curIndexRID still same as
   // previous saved one. If so it means there's no change in this key, and we
   // can move on the from the current index rid. Otherwise we have to locate
   // the new position for the saved key+rid
   // this is used in query scan only
   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEIXSCAN_RESUMESCAN, "_rtnMergeIXScanner::resumeScan" )
   INT32 _rtnMergeIXScanner::resumeScan( BOOLEAN *pIsCursorSame )
   {
      SINT32 rc = SDB_OK ;
      SINT32 rcl = SDB_OK ;
      SINT32 rcr = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNMERGEIXSCAN_RESUMESCAN ) ;

      BOOLEAN isSame  = TRUE ;
      BOOLEAN lIsSame = TRUE ;
      BOOLEAN rIsSame = TRUE ;

      if ( _leftEnabled )
      {
         rcl = _leftIXScanner->resumeScan( &lIsSame ) ;
      }
      if ( _rightEnabled )
      {
         rcr = _rightIXScanner->resumeScan( &rIsSame ) ;
      }

      rc = rcl ? rcl : rcr ;
      if ( rc )
      {
         goto error ;
      }

      if ( SCAN_LEFT == _fromDir )
      {
         isSame = lIsSame ;
      }
      else if ( SCAN_RIGHT == _fromDir )
      {
         isSame = rIsSame ;
      }

      /// sync left to right
      if ( SCAN_LEFT == _fromDir && _rightEnabled )
      {
         rc = _rightIXScanner->syncPredStatus( _leftIXScanner ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync left scanner predicate status to right "
                    "failed, rc: %d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Relocate right scanner to obj(%s) with rid(%d,%d)",
                 _savedObj.toString().c_str(),
                 _savedRID._extent, _savedRID._offset ) ;

         rc = _rightIXScanner->relocateRID( _savedObj, _savedRID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Relocate to obj(%s) and rid(%d,%d) faild in "
                    "right scan, rc: %d", _savedObj.toString().c_str(),
                    _savedRID._extent, _savedRID._offset, rc ) ;
            goto error ;
         }
         _rrid.reset() ;
      }
      /// sync right to left
      else if ( SCAN_RIGHT == _fromDir && _leftEnabled )
      {
         rc = _leftIXScanner->syncPredStatus( _rightIXScanner ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Sync right scanner predicate status to left "
                    "failed, rc: %d", rc ) ;
            goto error ;
         }

         PD_LOG( PDDEBUG, "Relocate left scanner to obj(%s) with rid(%d,%d)",
                 _savedObj.toString().c_str(),
                 _savedRID._extent, _savedRID._offset ) ;

         rc = _leftIXScanner->relocateRID( _savedObj, _savedRID ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Relocate to obj(%s) and rid(%d,%d) faild in "
                    "left scan, rc: %d", _savedObj.toString().c_str(),
                    _savedRID._extent, _savedRID._offset, rc ) ;
            goto error ;
         }
         _lrid.reset() ;
      }

      /// when left has changed, but last from right
      if ( SCAN_RIGHT == _fromDir && _leftEnabled &&
           !_leftIXScanner->eof() &&
           ( !lIsSame || _lrid.isNull() ) )
      {
         rc = _leftIXScanner->advance( _lrid ) ;
         if ( rc && SDB_IXM_EOC != rc )
         {
            PD_LOG( PDERROR, "Left scan advance failed, rc: %d", rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "Left scanner advance to obj(%s) with rid(%d,%d)",
                 _leftIXScanner->getCurKeyObj()->toString().c_str(),
                 _lrid._extent, _lrid._offset ) ;
         rc = SDB_OK ;
      }
      /// when right has changed, but last from left
      if ( SCAN_LEFT == _fromDir && _rightEnabled &&
           !_rightIXScanner->eof() &&
           ( !rIsSame || _rrid.isNull() ) )
      {
         rc = _rightIXScanner->advance( _rrid ) ;
         if ( rc && SDB_IXM_EOC != rc )
         {
            PD_LOG( PDERROR, "Right scan advance failed, rc: %d", rc ) ;
            goto error ;
         }
         PD_LOG( PDDEBUG, "Right scanner advance to obj(%s) with rid(%d,%d)",
                 _rightIXScanner->getCurKeyObj()->toString().c_str(),
                 _rrid._extent, _rrid._offset ) ;
         rc = SDB_OK ;
      }

   done :
      if ( pIsCursorSame )
      {
         *pIsCursorSame = isSame ;
      }
      PD_TRACE_EXITRC ( SDB__RTNMERGEIXSCAN_RESUMESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   // PD_TRACE_DECLARE_FUNCTION ( SDB__RTNMERGEIXSCAN_PAUSESCAN, "_rtnMergeIXScanner::pauseScan" )
   INT32 _rtnMergeIXScanner::pauseScan()
   {
      INT32 rc = SDB_OK ;
      PD_TRACE_ENTRY ( SDB__RTNMERGEIXSCAN_PAUSESCAN ) ;
      INT32 rcl = SDB_OK ;
      INT32 rcr = SDB_OK ;

      if ( _leftEnabled )
      {
         rcl = _leftIXScanner->pauseScan() ;
      }
      if ( _rightEnabled )
      {
         rcr = _rightIXScanner->pauseScan() ;
      }

      rc = rcl ? rcl : rcr ;
      if ( rc )
      {
         goto error ;
      }

      // globally save the obj and rid incase things changed after resume,
      // at which time we need to do relocateRID based on these saved value
      if ( SCAN_NONE != _fromDir )
      {
         _savedRID = getSavedRIDFromChild() ;
         _savedObj = getSavedObjFromChild()->getOwned() ;

         PD_LOG( PDDEBUG, "Paused in obj(%s) with rid(%d,%d), From(%s)",
                 _savedObj.toString().c_str(), _savedRID._extent,
                 _savedRID._offset,
                 ( SCAN_LEFT == _fromDir ? "LEFT" : "RIGHT" ) ) ;
      }

   done:
      PD_TRACE_EXITRC ( SDB__RTNMERGEIXSCAN_PAUSESCAN, rc ) ;
      return rc ;
   error :
      goto done ;
   }

   const dmsRecordID& _rtnMergeIXScanner::getSavedRIDFromChild() const
   {
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;

      return ( SCAN_LEFT == _fromDir ) ? _leftIXScanner->getSavedRID() :
                                         _rightIXScanner->getSavedRID() ;
   }

   const BSONObj* _rtnMergeIXScanner::getSavedObjFromChild() const
   {
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;

      return ( SCAN_LEFT ==_fromDir ) ? _leftIXScanner->getSavedObj() :
                                        _rightIXScanner->getSavedObj() ;
   }

   INT32 _rtnMergeIXScanner::relocateRID( BOOLEAN &found )
   {
      SDB_ASSERT( FALSE, "Can't call the function" ) ;
      return SDB_SYS ;
   }

   INT32 _rtnMergeIXScanner::relocateRID( const BSONObj &keyObj,
                                          const dmsRecordID &rid )
   {
      INT32 rc = SDB_OK ;
      BOOLEAN leftEOF = TRUE, rightEOF = TRUE ;

      _savedObj = keyObj.getOwned() ;
      _savedRID = rid ;

      if ( _leftEnabled && _leftIXScanner )
      {
         rc = _leftIXScanner->relocateRID( keyObj, rid ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Relocate rid in left scan failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         leftEOF = _leftIXScanner->eof() ;
      }

      if ( _rightEnabled && _rightIXScanner )
      {
         rc = _rightIXScanner->relocateRID( keyObj, rid ) ;
         if ( rc )
         {
            PD_LOG( PDERROR, "Relocate rid in right scan failed, rc: %d",
                    rc ) ;
            goto error ;
         }
         rightEOF = _rightIXScanner->eof() ;
      }

      _eof = ( leftEOF && rightEOF ) ? TRUE : FALSE ;
      _fromDir = SCAN_NONE ;

   done:
      return rc ;
   error:
      goto done ;
   }

   BOOLEAN _rtnMergeIXScanner::isAvailable() const
   {
      return ( _leftIXScanner && _rightIXScanner ) ? TRUE : FALSE ;
   }

   IXScannerType _rtnMergeIXScanner::getType() const
   {
      return SCANNER_TYPE_MERGE ;
   }

   IXScannerType _rtnMergeIXScanner::getCurScanType() const
   {
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;

      return ( SCAN_LEFT == _fromDir ) ? _leftType : _rightType ;
   }

   void _rtnMergeIXScanner::disableByType( IXScannerType type )
   {
      if ( _leftType == type )
      {
         _leftEnabled = FALSE ;
      }
      else if ( _rightType == type )
      {
         _rightEnabled = FALSE ;
      }
   }

   INT32 _rtnMergeIXScanner::getLockModeByType( IXScannerType type ) const
   {
      if ( _leftType == type && _leftIXScanner )
      {
         return _leftIXScanner->getLockModeByType( type ) ;
      }
      else if ( _rightType == type && _rightIXScanner )
      {
         return _rightIXScanner->getLockModeByType( type ) ;
      }
      return -1 ;
   }

   rtnPredicateListIterator*  _rtnMergeIXScanner::getPredicateListInterator()
   {
      SDB_ASSERT( FALSE, "Can't call the function" ) ;
      return NULL ;
   }

   INT32 _rtnMergeIXScanner::isCursorSame( const BSONObj &saveObj,
                                           const dmsRecordID &saveRID,
                                           BOOLEAN &isSame )
   {
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;

      return ( SCAN_LEFT == _fromDir ) ?
             _leftIXScanner->isCursorSame( saveObj, saveRID, isSame ) :
             _rightIXScanner->isCursorSame( saveObj, saveRID, isSame ) ;
   }

   const BSONObj* _rtnMergeIXScanner::getCurKeyObj() const
   {
      SDB_ASSERT( isAvailable(), "Must be available" ) ;
      SDB_ASSERT( SCAN_NONE != _fromDir, "Invalid scann from" ) ;

      return  ( SCAN_LEFT == _fromDir ) ? _leftIXScanner->getCurKeyObj() :
                                          _rightIXScanner->getCurKeyObj() ;
   }


}

